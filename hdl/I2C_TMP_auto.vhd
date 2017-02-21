------------------------------------------------------------------------------
--                       Paul Scherrer Institute (PSI)
------------------------------------------------------------------------------
-- File    : I2C_auto.vhd    
-- Author  : Alessandro Malatesta, Section Diagnostic
-- Version : $Revision: 1.0 $
------------------------------------------------------------------------------
-- Copyright© PSI, Section Diagnostic
------------------------------------------------------------------------------
-- Description:
-- This module implements an APB peripheral that periodically reads from a 
-- set of I2C targets. The results are made available on a register interface.
-- The module integrates the I2C core developed by Goran Marinkovic.
-- All the readings are repeated every 50M clock cycles. Extra commands can
-- be issued by writing to the command register.
--
-- APB Memory map
-- Read-only:
--   0x00: T112 temperature sensor, MBU's front air outlet
--   0x04: T112 temperature sensor, MBU's rear air outlet
--   0x08: T112 temperature sensor, MBU's power board
--   0x0C: T112 temperature sensor, MBU's air inlet
--   0x10: TC74 temperature sensor, MBU's heatsink
--   0x14: AD5321 DAC, MBU's heaters
-- Write-only:
--   0x00: Command register: when this register is written, a command is 
--         pushed to the input FIFO. If the FIFO is full, the command is ignored.
--         Parameters are taken from the register as follows:
--           bit 10 : READ-notWRITE
--           bit 9:0: ROM index (same as read registers' addresses)
--   0x04: For write commands the data to be written is read from this register.
------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library synplify;
use synplify.attributes.all;

use work.APB3_regbank_pkg.all;

--Core source: G\XFEL\14_Firmware\13_Prototype\Cavity_BPM\02_Design\BPM_FPGA\06_EDK\pcores\plb46_to_iic_mmap_v1_00_a\hdl\vhdl
--library plb46_to_iic_mmap_v1_00_a;
use work.mmap_package.all;

entity I2C_TMP_auto is
port(
    PCLK    : in  std_logic;
    --APB3 SLAVE (register interface)
    s_PRESETn : in  std_logic;
    s_PSELx   : in  std_logic;
    s_PENABLE : in  std_logic;
    s_PWRITE  : in  std_logic;
    s_PADDR   : in  std_logic_vector(31 downto 0); --byte addressing, 32bit registers
    s_PWDATA  : in  std_logic_vector(31 downto 0);
    s_PRDATA  : out std_logic_vector(31 downto 0) := X"00000000";
    s_PREADY  : out std_logic := '1';
    s_PSLVERR : out std_logic := '0';
    --i2c
    o_i2c_scl_tx : out std_logic;
    i_i2c_scl_rx : in  std_logic;
    o_i2c_sda_tx : out std_logic;
    i_i2c_sda_rx : in  std_logic
);
end entity I2C_TMP_auto;

architecture rtl of I2C_TMP_auto is

    attribute syn_radhardlevel : string;
--    attribute syn_radhardlevel of rtl : architecture is "tmr";

    constant REGBANK_ADDR_W : natural := 4; --16registers
    constant REGBANK_NREG : natural := 2**REGBANK_ADDR_W;
    constant REGBANK_CMD_ADDR : std_logic_vector(REGBANK_ADDR_W+2-1 downto 0) := (others => '0');

    signal fabric2cpu_reg : slv32_array(0 to 2**REGBANK_ADDR_W-1);
    signal cpu2fabric_reg : slv32_array(0 to 2**REGBANK_ADDR_W-1);

    signal rst, s_PRESETn_reg, regbank_write, cmd_reg_write : std_logic;
    signal cmd_valid, cmd_rnw, fifo_empty, fifo_full, i_update, o_busy, o_periodic : std_logic;
    signal rd_index  : unsigned(9 downto 0);
    signal cmd_index : std_logic_vector(9 downto 0);
    signal cmd_data, rd_data : std_logic_vector(31 downto 0);

    signal timer : natural; --let it be 32 bits
    signal s : natural range 0 to 7; --3 bits 

begin

--sync reset for speed
sRST_REG_P : process(PCLK)
begin 
    if rising_edge(PCLK) then
        s_PRESETn_reg <= s_PRESETn;
    end if;
end process;

--slave register bank (for data exchange with CPU)
REGBANK : APB3_Regbank
generic map(
    ADDR_W => REGBANK_ADDR_W --actual address is 2 bit more (byte)
)
port map(
    --APB3
    PCLK    => PCLK,
    PRESETn => s_PRESETn_reg,
    PSEL    => s_PSELx, 
    PENABLE => s_PENABLE,
    PWRITE  => s_PWRITE,
    PADDR   => s_PADDR(REGBANK_ADDR_W+2-1 downto 0),
    PWDATA  => s_PWDATA,
    PRDATA  => s_PRDATA,
    PREADY  => s_PREADY,
    PSLVERR => s_PSLVERR,
    --Register bank
    rd_en_o     => open,
    wr_en_o     => regbank_write,
    regbank_in  => fabric2cpu_reg,
    regbank_out => cpu2fabric_reg
);

--monitor writes to register 0
COMMAND_HANDLER_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if (s_PRESETn_reg = '0') then
            cmd_valid     <= '0';
            cmd_reg_write <= '0';
        else
            if s_PSELx = '1' and (s_PADDR(REGBANK_ADDR_W+2-1 downto 0) = REGBANK_CMD_ADDR) then
                cmd_reg_write <= '1';
            else
                cmd_reg_write <= '0';
            end if;

            if (regbank_write = '1') and 
               (cmd_reg_write = '1') and
               (fifo_full = '0') then
                cmd_valid <= '1';
            else
                cmd_valid <= '0';
            end if;
        end if;
    end if;
end process; --command_handler_p
                
cmd_rnw   <= cpu2fabric_reg(0)(10);
cmd_index <= cpu2fabric_reg(0)(9 downto 0);
cmd_data  <= cpu2fabric_reg(1);

--wait timeout, start update, wait update to be over, read all registers, restart.
AUTO_UPDATE_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if (s_PRESETn_reg = '0') then
            i_update  <= '0';
            s         <= 0;
            timer     <= 0;
        else
            case s is
            when 0 =>
                --wait for timer
                if timer = 5000000 then
                    timer <= 0;
                    s <= 1;
                else
                    timer <= timer+1;
                end if;
            when 1 =>
                --wait for idle then start update
                if o_busy = '0' then
                    i_update <= '1';
                    s <= 2;
                end if;
            when 2 =>
                --wait for update to start then reset start signal
                if o_busy = '1' and o_periodic = '1' then
                    i_update <= '0';
                    s <= 3;
                end if;
            when 3 =>
                --wait for update to finish
                if o_busy = '0' then
                    rd_index <= (others => '0');
                    s <= 4;
                end if;
            when 4 =>
                --update registers
                if rd_index < ("00"&X"06") then --more registers to read
                    if timer < 4 then --wait a bit
                        timer <= timer+1;
                    else --store data and increment counter
                        timer <= 0;
                        fabric2cpu_reg(to_integer(rd_index)) <= rd_data;
                        rd_index <= rd_index+1;
                    end if;
                else --nothing left to read
                    s <= 0;
                end if; 
            when others =>
                i_update  <= '0';
                s         <= 0;
                timer     <= 0;
            end case;
        end if;
    end if;
end process; --auto_update_p




--Core from Goran ------------------------------------------------------------------------
--G\XFEL\14_Firmware\13_Prototype\Cavity_BPM\02_Design\BPM_FPGA\06_EDK\pcores\plb46_to_iic_mmap_v1_00_a\hdl\vhdl
rst <= not s_PRESETn_reg;
--------------------------
mmap_inst: mmap
generic map(
    c_fpga    => "smartfusion",
    c_num_to_read => X"006", --ML84: number of fields to read (periodic mode)
    c_i2c_clk_div => 160, --ML84
    c_storage =>  --ML84: maximum 32 entries
      (
         0 => (dev => X"48", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- T112 temp sensor (Front Air outlet)
         1 => (dev => X"49", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- T112 temp sensor (Rear air outlet) 
         2 => (dev => X"4A", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- T112 temp sensor (Power Board)
         3 => (dev => X"4B", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- T112 temp sensor (Air Inlet)
         4 => (dev => X"4D", reg_len => "01", reg => X"0000", data_len => "001", rep => '1'), -- TC74 temp sensor (Heatsink)
         5 => (dev => X"0D", reg_len => "00", reg => X"0000", data_len => "010", rep => '1'), -- AD5321 DAC (Heater control)
         6 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- free
         7 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0')  -- free
      )
)
port map(
    ------------------------------------------------------------------------
    -- Debug interface
    ------------------------------------------------------------------------
    debug                       => open,
    ------------------------------------------------------------------------
    -- System
    ------------------------------------------------------------------------
    i_rst                       => rst,
    i_clk                       => PCLK,
    ------------------------------------------------------------------------
    -- Data Interface
    ------------------------------------------------------------------------
    i_trig                      => cmd_valid,     --write to input FIFO
    i_irq                       => "0000",   --irq loaded into FIFO (not used here)
    i_rw                        => cmd_rnw,      --read/write#
    i_wr_index                  => cmd_index, --ROM entry
    i_wr_data                   => cmd_data,  --data to be written
    o_wr_empty                  => fifo_empty, --input fifo status
    o_wr_full                   => fifo_full,  --input fifo status
    i_rd_index                  => std_logic_vector(rd_index), --RAM read address
    o_rd_data                   => rd_data,  --RAM read data
    ------------------------------------------------------------------------
    -- Interrupt Interface
    ------------------------------------------------------------------------
    o_int                       => open, --not used
    ------------------------------------------------------------------------
    -- Periodic read Interface
    ------------------------------------------------------------------------
    i_update                    => i_update, --start update
    o_busy                      => o_busy,
    o_periodic                  => o_periodic,
    ------------------------------------------------------------------------
    -- Serial link interface
    ------------------------------------------------------------------------
    o_i2c_scl_tx                => o_i2c_scl_tx,
    i_i2c_scl_rx                => i_i2c_scl_rx,
    o_i2c_sda_tx                => o_i2c_sda_tx,
    i_i2c_sda_rx                => i_i2c_sda_rx
   );

end architecture rtl; --of I2C_auto

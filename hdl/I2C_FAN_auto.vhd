------------------------------------------------------------------------------
--                       Paul Scherrer Institute (PSI)
------------------------------------------------------------------------------
-- File    : I2C_FAN_auto.vhd    
-- Author  : Alessandro Malatesta, Section Diagnostic
-- Version : $Revision: 1.0 $
------------------------------------------------------------------------------
-- Copyright© PSI, Section Diagnostic
------------------------------------------------------------------------------
-- Description:
-- This module implements an APB peripheral that periodically reads from a 
-- set of I2C targets. The results are made available on a register interface.
-- The module integrates the I2C core developed by Goran Marinkovic.
-- All the readings are repeated periodically. Extra commands can
-- be issued by writing to the command register.
--
-- ROM Codes (mirrored registers can be read directly from register map):
-- Mirrored:
--   0x00: MAX31785 fan controller 1 - page register
--   0x01: MAX31785 fan controller 1 - FAN_CONFIG_1_2: 7=fan_en, 6=RPM/PWM#, 5:4=TACH pulses (pages 0:5)  
--   0x02: MAX31785 fan controller 1 - FAN_COMMAND_1: 0xFFFF=auto_ctrl, others=PWM/RPM value
--   0x03: MAX31785 fan controller 1 - STATUS_WORD
--   0x04: MAX31785 fan controller 1 - STATUS_CML (communication status)
--   0x05: MAX31785 fan controller 1 - STATUS_MFR_SPECIFIC (pages 6:15) 
--   0x06: MAX31785 fan controller 1 - STATUS_FANS_1_2     (pages 0:5)  
--   0x07: MAX31785 fan controller 1 - FAN_SPEED           (pages 0:5)  
--   0x08: MAX31785 fan controller 1 - MFR_MODE
--   0x09: MAX31785 fan controller 1 - MFR_FAULT_RESPONSE
--   0x0A: MAX31785 fan controller 1 - MFR_TIME_COUNT (device lifetime)
--   0x0B: MAX31785 fan controller 1 - MFR_FAN_CONFIG (pages 0:5)                 
--   0x0C: MAX31785 fan controller 1 - MFR_FAN_PWM (pages 0:5)                 
--   0x0D: MAX31785 fan controller 1 - MFR_FAN_FAULT_LIMIT (pages 0:5)                 
--   0x0E: MAX31785 fan controller 1 - MFR_FAN_WARN_LIMIT (pages 0:5)                 
--   0x0F: MAX31785 fan controller 1 - MFR_FAN_RUN_TIME (pages 0:5)                 
--   0x10: MAX31785 fan controller 1 - MFR_FAN_PWM_AVG (pages 0:5)                 

--   0x11: MAX31785 fan controller 2 - page register
--   0x12: MAX31785 fan controller 2 - FAN_CONFIG_1_2: 7=fan_en, 6=RPM/PWM#, 5:4=TACH pulses (pages 0:5)  
--   0x13: MAX31785 fan controller 2 - FAN_COMMAND_1: 0xFFFF=auto_ctrl, others=PWM/RPM value
--   0x14: MAX31785 fan controller 2 - STATUS_WORD
--   0x15: MAX31785 fan controller 2 - STATUS_CML (communication status)
--   0x16: MAX31785 fan controller 2 - STATUS_MFR_SPECIFIC (pages 6:15) 
--   0x17: MAX31785 fan controller 2 - STATUS_FANS_1_2     (pages 0:5)  
--   0x18: MAX31785 fan controller 2 - FAN_SPEED           (pages 0:5)  
--   0x19: MAX31785 fan controller 2 - MFR_MODE
--   0x1A: MAX31785 fan controller 2 - MFR_FAULT_RESPONSE
--   0x1B: MAX31785 fan controller 2 - MFR_TIME_COUNT (device lifetime)
--   0x1C: MAX31785 fan controller 2 - MFR_FAN_CONFIG (pages 0:5)                 
--   0x1D: MAX31785 fan controller 2 - MFR_FAN_PWM (pages 0:5)                 
--   0x1E: MAX31785 fan controller 2 - MFR_FAN_FAULT_LIMIT (pages 0:5)                 
--   0x1F: MAX31785 fan controller 2 - MFR_FAN_WARN_LIMIT (pages 0:5)                 
--   0x20: MAX31785 fan controller 2 - MFR_FAN_RUN_TIME (pages 0:5)                 
--   0x21: MAX31785 fan controller 2 - MFR_FAN_PWM_AVG (pages 0:5)    

-- Write-only:
--   0x22: MAX31785 fan controller 1 - clear faults (write 0 to clear) TODO:check
--   0x23: MAX31785 fan controller 1 - write protect (0x0=no, 0x40=allow PAGE & WP, 0x80=allow only WP)
--   0x24: MAX31785 fan controller 1 - store configuration data (wait 250ms before next command)                 
--   0x25: MAX31785 fan controller 1 - restore configuration data
--   0x26: MAX31785 fan controller 2 - clear faults (write 0 to clear) TODO:check
--   0x27: MAX31785 fan controller 2 - write protect (0x0=no, 0x40=allow PAGE & WP, 0x80=allow only WP)
--   0x28: MAX31785 fan controller 2 - store configuration data (wait 250ms before next command)                 
--   0x29: MAX31785 fan controller 2 - restore configuration data

-- Write interface:
--   0x00: Command register: when this register is written, a command is 
--         pushed to the input FIFO. If the FIFO is full, the command is ignored.
--         Parameters are taken from the register as follows:
--           bit 10 : READ-notWRITE
--           bit 9:0: ROM code
--   0x04: For write commands only. The data to be written
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

entity I2C_FAN_auto is
generic(
    AUTO_READ_PERIOD : natural := 5000000 --100ms
);
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
end entity I2C_FAN_auto;

architecture rtl of I2C_FAN_auto is

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

    constant REGS_TO_READ : std_logic_vector(11 downto 0) := X"000";

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
                if timer = AUTO_READ_PERIOD then
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
                if rd_index < unsigned(REGS_TO_READ(9 downto 0)) then --more registers to read
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
    c_num_to_read => REGS_TO_READ, --ML84: number of fields to read (periodic mode)
    c_i2c_clk_div => 160, --ML84
    c_storage =>  --ML84: maximum 64 entries
    (
      --ID     I2C address   Reg addr bytes   Register addr   Data bytes         Auto read
      --Fan controller 1 (mirrored to register map)
         0 => (dev => X"52", reg_len => "01", reg => X"0000", data_len => "001", rep => '1'), -- MAX31785 fan controller 1 - page register
         1 => (dev => X"52", reg_len => "01", reg => X"003A", data_len => "001", rep => '1'), -- MAX31785 fan controller 1 - FAN_CONFIG_1_2: 7=fan_en, 6=RPM/PWM#, 5:4=TACH pulses (pages 0:5)  
         2 => (dev => X"52", reg_len => "01", reg => X"003B", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - FAN_COMMAND_1: 0xFFFF=auto_ctrl, others=PWM/RPM value
         3 => (dev => X"52", reg_len => "01", reg => X"0079", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - STATUS_WORD
         4 => (dev => X"52", reg_len => "01", reg => X"007E", data_len => "001", rep => '1'), -- MAX31785 fan controller 1 - STATUS_CML (communication status)
         5 => (dev => X"52", reg_len => "01", reg => X"0080", data_len => "001", rep => '1'), -- MAX31785 fan controller 1 - STATUS_MFR_SPECIFIC (pages 6:15) 
         6 => (dev => X"52", reg_len => "01", reg => X"0081", data_len => "001", rep => '1'), -- MAX31785 fan controller 1 - STATUS_FANS_1_2     (pages 0:5)  
         7 => (dev => X"52", reg_len => "01", reg => X"0090", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - FAN_SPEED           (pages 0:5)  
         8 => (dev => X"52", reg_len => "01", reg => X"00D1", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - MFR_MODE
         9 => (dev => X"52", reg_len => "01", reg => X"00D9", data_len => "001", rep => '1'), -- MAX31785 fan controller 1 - MFR_FAULT_RESPONSE
        10 => (dev => X"52", reg_len => "01", reg => X"00DD", data_len => "100", rep => '1'), -- MAX31785 fan controller 1 - MFR_TIME_COUNT (device lifetime)
        11 => (dev => X"52", reg_len => "01", reg => X"00F1", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - MFR_FAN_CONFIG (pages 0:5)                 
        12 => (dev => X"52", reg_len => "01", reg => X"00F3", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - MFR_FAN_PWM (pages 0:5)                 
        13 => (dev => X"52", reg_len => "01", reg => X"00F5", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - MFR_FAN_FAULT_LIMIT (pages 0:5)                 
        14 => (dev => X"52", reg_len => "01", reg => X"00F6", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - MFR_FAN_WARN_LIMIT (pages 0:5)                 
        15 => (dev => X"52", reg_len => "01", reg => X"00F7", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - MFR_FAN_RUN_TIME (pages 0:5)                 
        16 => (dev => X"52", reg_len => "01", reg => X"00F8", data_len => "010", rep => '1'), -- MAX31785 fan controller 1 - MFR_FAN_PWM_AVG (pages 0:5)                 
      --Fan controller 2 (mirrored to register map)
        17 => (dev => X"52", reg_len => "01", reg => X"0000", data_len => "001", rep => '1'), -- MAX31785 fan controller 2 - page register
        18 => (dev => X"52", reg_len => "01", reg => X"003A", data_len => "001", rep => '1'), -- MAX31785 fan controller 2 - FAN_CONFIG_1_2: 7=fan_en, 6=RPM/PWM#, 5:4=TACH pulses (pages 0:5)  
        19 => (dev => X"52", reg_len => "01", reg => X"003B", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - FAN_COMMAND_1: 0xFFFF=auto_ctrl, others=PWM/RPM value
        20 => (dev => X"52", reg_len => "01", reg => X"0079", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - STATUS_WORD
        21 => (dev => X"52", reg_len => "01", reg => X"007E", data_len => "001", rep => '1'), -- MAX31785 fan controller 2 - STATUS_CML (communication status)
        22 => (dev => X"52", reg_len => "01", reg => X"0080", data_len => "001", rep => '1'), -- MAX31785 fan controller 2 - STATUS_MFR_SPECIFIC (pages 6:15) 
        23 => (dev => X"52", reg_len => "01", reg => X"0081", data_len => "001", rep => '1'), -- MAX31785 fan controller 2 - STATUS_FANS_1_2     (pages 0:5)  
        24 => (dev => X"52", reg_len => "01", reg => X"0090", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - FAN_SPEED           (pages 0:5)  
        25 => (dev => X"52", reg_len => "01", reg => X"00D1", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - MFR_MODE
        26 => (dev => X"52", reg_len => "01", reg => X"00D9", data_len => "001", rep => '1'), -- MAX31785 fan controller 2 - MFR_FAULT_RESPONSE
        27 => (dev => X"52", reg_len => "01", reg => X"00DD", data_len => "100", rep => '1'), -- MAX31785 fan controller 2 - MFR_TIME_COUNT (device lifetime)
        28 => (dev => X"52", reg_len => "01", reg => X"00F1", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - MFR_FAN_CONFIG (pages 0:5)                 
        29 => (dev => X"52", reg_len => "01", reg => X"00F3", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - MFR_FAN_PWM (pages 0:5)                 
        30 => (dev => X"52", reg_len => "01", reg => X"00F5", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - MFR_FAN_FAULT_LIMIT (pages 0:5)                 
        31 => (dev => X"52", reg_len => "01", reg => X"00F6", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - MFR_FAN_WARN_LIMIT (pages 0:5)                 
        32 => (dev => X"52", reg_len => "01", reg => X"00F7", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - MFR_FAN_RUN_TIME (pages 0:5)                 
        33 => (dev => X"52", reg_len => "01", reg => X"00F8", data_len => "010", rep => '1'), -- MAX31785 fan controller 2 - MFR_FAN_PWM_AVG (pages 0:5)            
      --Fan controller 1 write only
        34 => (dev => X"52", reg_len => "01", reg => X"0003", data_len => "000", rep => '0'), -- MAX31785 fan controller 1 - clear faults (write 0 to clear) TODO:check
        35 => (dev => X"52", reg_len => "01", reg => X"0010", data_len => "001", rep => '0'), -- MAX31785 fan controller 1 - write protect (0x0=no, 0x40=allow PAGE & WP, 0x80=allow only WP)
        36 => (dev => X"52", reg_len => "01", reg => X"0011", data_len => "000", rep => '0'), -- MAX31785 fan controller 1 - store configuration data (wait 250ms before next command)                 
        37 => (dev => X"52", reg_len => "01", reg => X"0012", data_len => "000", rep => '0'), -- MAX31785 fan controller 1 - restore configuration data
      --Fan controller 1 write only
        38 => (dev => X"52", reg_len => "01", reg => X"0003", data_len => "000", rep => '0'), -- MAX31785 fan controller 2 - clear faults (write 0 to clear) TODO:check
        39 => (dev => X"52", reg_len => "01", reg => X"0010", data_len => "001", rep => '0'), -- MAX31785 fan controller 2 - write protect (0x0=no, 0x40=allow PAGE & WP, 0x80=allow only WP)
        40 => (dev => X"52", reg_len => "01", reg => X"0011", data_len => "000", rep => '0'), -- MAX31785 fan controller 2 - store configuration data (wait 250ms before next command)                 
        41 => (dev => X"52", reg_len => "01", reg => X"0012", data_len => "000", rep => '0'), -- MAX31785 fan controller 2 - restore configuration data
      --Available
        42 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        43 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        44 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        45 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        46 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        47 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        48 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        49 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        50 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        51 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        52 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        53 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        54 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        55 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        56 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        57 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        58 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        59 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        60 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        61 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        62 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0'), -- empty
        63 => (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0')  -- empty
      --ADD_IF_NEEDED: 21 => (dev => X"52", reg_len => "01", reg => X"002A", data_len => "010", rep => '0'), -- MAX31785 fan controller 1 - set vout scale monitor (pages 17:22)
      --ADD IF NEEDED: Overvoltage/Undervoltage Fault/Failure limits (4 registers)
      --ADD IF NEEDED: Voltage monitors status 0x7A, 1byte
      --ADD IF NEEDED: Voltage value 0x8B, 2byte
      --ADD IF NEEDED: temperature value 0x8D, 2byte
      --ADD IF NEEDED: max/min monitors
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

end architecture rtl; --of I2C_FAN_auto

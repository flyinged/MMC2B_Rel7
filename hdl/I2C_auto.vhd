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
--   0x18: PCA9555, GPO current outputs
--   0x1C: PCA9555, GPI current inputs
--   0x20: RTC time
--   0x24: RTC date
--   0x28: ETC cfg 
--   0x2C: ETC alarm cnt 
--   0x30: ETC elapsed time cnt
--   0x34: ETC event cnt
--   0x38: GPO dir (settings)
--   0x3C: GPI dir (settings)
--   0x40: RTC ctrl
--   0x1FC : debug
--   
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
use work.mmap_package_multi.all;

entity I2C_auto is
generic(
    INIT_WAIT : integer := 25000000 --clock cycles to wait before start
);
port(
    dbg_data0 : out std_logic_vector(31 downto 0); 
    --dbg_data1 : out std_logic_vector(31 downto 0); 
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
    --GPIO
    gpi_data : out std_logic_vector(15 downto 0); 
    gpo_data : in  std_logic_vector(15 downto 0);
    --i2c
    o_i2c_scl_tx : out std_logic_vector(1 downto 0);
    i_i2c_scl_rx : in  std_logic_vector(1 downto 0);
    o_i2c_sda_tx : out std_logic_vector(1 downto 0);
    i_i2c_sda_rx : in  std_logic_vector(1 downto 0)
);
end entity I2C_auto;

architecture rtl of I2C_auto is

component ram_128x32 is
port( 
    dina  : in    std_logic_vector(31 downto 0);
    douta : out   std_logic_vector(31 downto 0);
    dinb  : in    std_logic_vector(31 downto 0);
    doutb : out   std_logic_vector(31 downto 0);
    addra : in    std_logic_vector(6 downto 0);
    addrb : in    std_logic_vector(6 downto 0);
    wea_n : in    std_logic;
    web_n : in    std_logic;
    ena   : in    std_logic;
    enb   : in    std_logic;
    clk   : in    std_logic
);
end component ram_128x32;

    attribute syn_radhardlevel : string;
    --attribute syn_radhardlevel of rtl : architecture is "tmr";

    constant REGBANK_ADDR_W : natural := 7; --128 RAM DWORDS
    constant REGBANK_NREG : natural := 2**REGBANK_ADDR_W;
    
    constant RAM_CMD_ADDR : std_logic_vector(REGBANK_ADDR_W-1 downto 0) := "0100000"; --0x20 (byte addr 0x80)
    constant RAM_CDA_ADDR : std_logic_vector(REGBANK_ADDR_W-1 downto 0) := "0100001"; --0x21 (byte addr 0x84)
    constant RAM_ENA_ADDR : std_logic_vector(REGBANK_ADDR_W-1 downto 0) := "0100010"; --0x22 (byte addr 0x88)
    constant RAM_DIS_ADDR : std_logic_vector(REGBANK_ADDR_W-1 downto 0) := "0100011"; --0x23 (byte addr 0x8C)

    signal rst, s_PRESETn_reg, regbank_write, cmd_sent : std_logic;
    signal cmd_reg_write : std_logic_Vector(3 downto 0);
    signal cmd_valid,   cmd_rnw   : std_logic;
    signal cmd_valid_x, cmd_rnw_x : std_logic;
    signal cmd_valid_a, cmd_rnw_a : std_logic;
    signal fifo_empty, fifo_full, i_update, o_busy, o_periodic : std_logic;
    signal rd_index, rd_index_r : unsigned(9 downto 0);
    signal cmd_index : std_logic_vector(9 downto 0);
    signal cmd_index_a : std_logic_vector(9 downto 0);
    signal cmd_index_x : std_logic_vector(9 downto 0);
    signal rd_data : std_logic_vector(31 downto 0) := (others => '1');
    signal cmd_data, cmd_data_x, cmd_data_a : std_logic_vector(31 downto 0);

    signal timer : unsigned(31 downto 0); --let it be 32 bits
    signal INIT_WAIT_V : unsigned(31 downto 0);
    constant UPDATE_WAIT : unsigned(31 downto 0) := X"002625A0"; --2.5e6 (100ms)
    --signal s, s_r : natural range 0 to 15; --4 bits 
    signal s, s_r : unsigned(3 downto 0);

    constant N_TO_READ  : std_logic_vector(11 downto 0) := X"00E";

    --ADDED
    signal apb_ram_wdata, apb_ram_rdata, ram_wdata, ram_rdata : std_logic_vector(31 downto 0);
    signal apb_ram_wen, ram_wen, ram_lock : std_logic;
    signal apb_ram_addr, ram_addr : std_logic_vector(REGBANK_ADDR_W-1 downto 0); 

    signal wd_timer : natural := 25000000;
    signal wd_rst : std_logic := '0';
    signal enable : std_logic;
    signal mmap_dbg : std_logic_vector(127 downto 0);

begin

INIT_WAIT_V <= to_unsigned(INIT_WAIT,32);

--reset watchdog on timeout or on FSM change
--if FSM gets stuck, a reset is triggered
--WATCHDOG_P : process(PCLK)
--begin
--    if rising_edge(PCLK) then
--        s_r <= s;
--
--        if (wd_timer = 0) or (s_r /= s) then --timeout
--            wd_timer <= 25000000;
--        else
--           wd_timer <= wd_timer-1;
--        end if;
--
--        if (wd_timer = 0) then
--            wd_rst <= '1';
--        else
--            wd_rst <= '0';
--        end if;
--    end if;
--end process;

--sync reset for speed
sRST_REG_P : process(PCLK)
begin 
    if rising_edge(PCLK) then
        s_PRESETn_reg <= s_PRESETn;
    end if;
end process;

DBG_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        dbg_data0 <= std_logic_vector(timer(11 downto 0)) &
                     "000" & mmap_dbg(78 downto 74) & --19:12
                     enable & i_update & o_busy & o_periodic & --11:8
                     fifo_full & fifo_empty & cmd_valid_x & cmd_valid_a &   --7:4
                     std_logic_vector(s); --3:0 
        --dbg_data1 <= std_logic_vector(TO_UNSIGNED(timer,32));
    end if;
end process;

REGBANK : APB3_Regbank_ram
generic map(
    DEBUG => '0',
    ALLOW_WRITE => '1',
    ADDR_W => REGBANK_ADDR_W,  --dword address
    DEBUG_ADDR => X"00000000" --byte address
)
port map(
    DBG_DATA => X"00000000",
    --APB3
    PCLK    => PCLK,
    PRESETn => s_PRESETn_reg,
    PSEL    => s_PSELx,
    PENABLE => s_PENABLE,
    PWRITE  => s_PWRITE,
    PADDR   => s_PADDR(REGBANK_ADDR_W+2-1 downto 0), --BYTE address
    PWDATA  => s_PWDATA,
    PRDATA  => s_PRDATA,
    PREADY  => s_PREADY,
    PSLVERR => s_PSLVERR,
    --APB RAM port
    we_n  => apb_ram_wen,
    addr  => apb_ram_addr,
    wdata => apb_ram_wdata,
    rdata => apb_ram_rdata
);

REGBANK_RAM : ram_128x32
port map( 
    --connected to APB interface
    ena   => '1',
    dina  => apb_ram_wdata,
    douta => apb_ram_rdata,
    addra => apb_ram_addr,
    wea_n => apb_ram_wen,
    --USER port
    enb   => '1',
    dinb  => ram_wdata,
    doutb => ram_rdata,
    addrb => ram_addr,
    web_n => ram_wen,

    clk   => PCLK
);

--monitor writes to location command addresses
COMMAND_HANDLER_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if (s_PRESETn_reg = '0') then
            cmd_valid_x <= '0';
            enable <= '0';--start disabled
        else
            --if s_PSELx = '1' and (s_PADDR(REGBANK_ADDR_W+2-1 downto 0) = REGBANK_CMD_ADDR) then
            if apb_ram_wen = '0' then --write to RAM
                case apb_ram_addr is
                    when RAM_CDA_ADDR =>
                    --if (apb_ram_addr = RAM_CDA_ADDR) then --write to data address
                        cmd_data_x <= apb_ram_wdata;
                        cmd_valid_x <= '0';
                    when RAM_CMD_ADDR =>
                    --elsif (apb_ram_addr = RAM_CMD_ADDR) then --write to command address
                        cmd_rnw_x <= apb_ram_wdata(10);
                        cmd_index_x <= apb_ram_wdata(9 downto 0);
                        cmd_valid_x <= '1';
                    when RAM_ENA_ADDR =>
                        enable <= '1';
                    when RAM_DIS_ADDR =>
                        enable <= '0';
                    when others =>
                        cmd_valid_x <= '0';
                    --end if;
                end case;
            else
                cmd_valid_x <= '0';
            end if;
        end if;

    end if;
end process; --command_handler_p

--ram read address
ram_addr <= std_logic_vector(rd_index(ram_addr'left downto 0));

--cmd_rnw_x   <= cpu2fabric_reg(0)(10);
--cmd_index_x <= cpu2fabric_reg(0)(9 downto 0);
--cmd_data_x  <= cpu2fabric_reg(1);

--wait timeout, start update, wait update to be over, read all registers, restart.
AUTO_UPDATE_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if (s_PRESETn_reg = '0') then
            i_update    <= '0';
            s           <= X"0";
            timer       <= (others => '0');
            cmd_valid_a <= '0';
            ram_wen     <= '1';
            cmd_sent    <= '0';
        else

            --write to GPI
            if ram_wen = '0' and ram_addr = "0000111" then --GPI 
                gpi_data <= rd_data(7 downto 0) & rd_data(15 downto 8); --swap bytes
            end if;

            case s is
            --INITIALIZATION
            when X"0" =>
                cmd_valid_a <= '0';
                --wait for timer
                --if (enable = '1') then
                    if (timer = INIT_WAIT_V) then --wait on start
                        timer <= (others => '0');
                        s <= s+1;
                    else
                        timer <= timer+1;
                    end if;
                --end if;
            when X"1" =>
                --Set GPO direction to output (0x0000)
                if fifo_full = '0' then
                    cmd_valid_a <= '1'; --write to FIFO
                    cmd_rnw_a   <= '0'; --write
                    cmd_index_a <= ("00" & X"0E"); --GPO DIR
                    cmd_data_a(15 downto 0)  <= X"0000";
                    s <= s+1; --don't go on if operation is not done
                end if;
            when X"2" =>
                --Set GPI direction to input (0xFFFF)
                if fifo_full = '0' then
                    cmd_valid_a <= '1'; --write to FIFO
                    cmd_rnw_a   <= '0'; --write
                    cmd_index_a <= ("00" & X"0F"); --GPI DIR
                    cmd_data_a(15 downto 0)  <= X"FFFF";
                    s <= s+1; --don't go on if operation is not done
                end if;
            --MAIN CYCLE
            when X"3" =>
                cmd_valid_a <= '0';
                --wait for timer
                if timer = UPDATE_WAIT then --wait 100 ms between update cycles
                    timer <= (others => '0');
                    s <= s+1;
                else
                    timer <= timer+1;
                end if;
            when X"4" =>
                --wait for idle then start update
                if o_busy = '0' then
                    i_update <= '1';
                    s <= s+1;
                end if;
            when X"5" =>
                --wait for update to start then reset start signal
                if o_busy = '1' and o_periodic = '1' then
                    i_update <= '0';
                    s <= s+1;
                end if;
            when X"6" =>
                --wait for update to finish
                if o_busy = '0' then
                    rd_index <= (others => '0'); --rx_index = table position: start from begin.
                    s <= s+1;
                end if;
            when X"7" =>
                --update RAM (read all data from I2C core RAM INTO APB ram
                if rd_index < unsigned(N_TO_READ(rd_index'left downto 0)) then --more registers to read
                    if timer < X"00000003" then --wait a bit (RAM in I2C module)
                        ram_wen <= '1';
                        timer <= timer+1;
                    elsif timer = X"00000003" then --write data
                        ram_wen   <= '0';
                        ram_wdata <= rd_data; --data from I2C core
                        timer <= timer+1;
                    else --increment address
                        ram_wen <= '1';
                        rd_index <= rd_index+1;
                        timer <= (others => '0');
                    end if;
                else --nothing left to read
                    --s <= 0;
                    --rd_index(ram_addr'left downto 0) <= unsigned(RAM_CDA_ADDR); --prepare to read CMD DATA
                    ram_wen <= '1';
                    s <= s+1;
                end if; 
            --serve I2c commands
            --when 8 => --DATA requested (available next cycle), request CMD
            --    if cmd_reg_write(3) = '1' then --data available 2 cycles later (microsemi's RAM)
            --        rd_index(ram_addr'left downto 0) <= unsigned(RAM_CMD_ADDR); --request CMD register
            --        s <= s+1;
            --    else
            --        s <= 12; --automatic commands
            --    end if;
            --when 9 => --just wait for data to show up on RAM port
            --    s <= s+1;
            --when 10 => --DATA available
            --    cmd_data_a <= ram_rdata;
            --    s <= s+1;
            --when 11 => --CMD available
            --    cmd_rnw_a   <= ram_rdata(10);
            --    cmd_index_a <= ram_rdata(9 downto 0);
            --    cmd_valid_a <= '1';
            --    cmd_sent    <= '1';
            --    s <= s+1;

            when X"8" => --automatic
                if fifo_full = '0' then
                    cmd_valid_a <= '1'; --write to FIFO
                    cmd_rnw_a   <= '0'; --write
                    cmd_index_a <= ("00" & X"06"); --GPO
                    cmd_data_a(15 downto 8)  <= gpo_data( 7 downto 0); --SWAP BYTES
                    cmd_data_a( 7 downto 0)  <= gpo_data(15 downto 8);
                else
                    cmd_valid_a <= '0';
                end if;
                cmd_sent <= '0';
                s        <= X"3"; --if fifo full, command will be executed on next cycle

            when others =>
                i_update    <= '0';
                s           <= X"0";
                timer       <= (others => '0');
                cmd_valid_a <= '0';
                ram_wen     <= '1';
                cmd_sent    <= '0';
            end case;
        end if;
    end if;
end process; --auto_update_p


FIFO_WRITE_ARB_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        --give priority to external commands
        if cmd_valid_x = '1' then
            cmd_valid <= cmd_valid_x;
            cmd_rnw   <= cmd_rnw_x;
            cmd_index <= cmd_index_x;
            cmd_data  <= cmd_data_x;
        else
            cmd_valid <= cmd_valid_a;
            cmd_rnw   <= cmd_rnw_a;
            cmd_index <= cmd_index_a;
            cmd_data  <= cmd_data_a;
        end if;
    end if;
end process;

--Core from Goran ------------------------------------------------------------------------
--G\XFEL\14_Firmware\13_Prototype\Cavity_BPM\02_Design\BPM_FPGA\06_EDK\pcores\plb46_to_iic_mmap_v1_00_a\hdl\vhdl
rst <= not s_PRESETn_reg;
--------------------------
mmap_inst: mmap
generic map(
    c_fpga    => "smartfusion",
    c_num_to_read => N_TO_READ, --ML84: number of fields to read (periodic mode)
    c_i2c_clk_div => 250, --ML84
    c_storage =>  --ML84: maximum 32 entries
      (
         0 => (dev => X"48", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- 0 T112 temp sensor (Front Air outlet)
         1 => (dev => X"49", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- 1 T112 temp sensor (Rear air outlet) 
         2 => (dev => X"4A", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- 2 T112 temp sensor (Power Board)
         3 => (dev => X"4B", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- 3 T112 temp sensor (Air Inlet)
         4 => (dev => X"4D", reg_len => "01", reg => X"0000", data_len => "001", rep => '1'), -- 4 TC74 temp sensor (Heatsink)
         5 => (dev => X"0D", reg_len => "00", reg => X"0000", data_len => "010", rep => '1'), -- 5 AD5321 DAC (Heater control)
         --SECOND I2C BUS (i2c_start_id1)
         6 => (dev => X"20", reg_len => "01", reg => X"0002", data_len => "010", rep => '1'), -- 6 GPO output, write 
         7 => (dev => X"21", reg_len => "01", reg => X"0000", data_len => "010", rep => '1'), -- 7 GPI inputs, read
         8 => (dev => X"51", reg_len => "01", reg => X"0003", data_len => "011", rep => '1'), -- 8 RTC time
         9 => (dev => X"51", reg_len => "01", reg => X"0006", data_len => "100", rep => '1'), -- 9 RTC date
        10 => (dev => X"6B", reg_len => "01", reg => X"0000", data_len => "001", rep => '1'), -- A ETC cfg
        11 => (dev => X"6B", reg_len => "01", reg => X"0001", data_len => "100", rep => '1'), -- B ETC alarm cnt
        12 => (dev => X"6B", reg_len => "01", reg => X"0005", data_len => "100", rep => '1'), -- C ETC et-count
        13 => (dev => X"6B", reg_len => "01", reg => X"0009", data_len => "010", rep => '1'), -- D ETC event-count
        --write only
        14 => (dev => X"20", reg_len => "01", reg => X"0006", data_len => "010", rep => '0'), -- E GPO direction, write 0x00
        15 => (dev => X"21", reg_len => "01", reg => X"0006", data_len => "010", rep => '0'), -- F GPI direction, write 0xFF
        16 => (dev => X"51", reg_len => "01", reg => X"0000", data_len => "001", rep => '0')  -- 10 RTC control reg
      )
)
port map(
    i2c_start_id1               => "0000000110", --6
    ------------------------------------------------------------------------
    -- Debug interface
    ------------------------------------------------------------------------
    debug                       => mmap_dbg,
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

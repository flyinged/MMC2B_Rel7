--------------------------------------------------------------------------------
-- Company: PSI
--
-- File: FAN_Controller.vhd
-- File history:
--      1.0 : 10/11/2015 : Creation
--
-- Description: 
-- PID controller for FAN control.
-- APB master port used to read temperatures from TMP_I2C bus (Core I2C, Channel 1) and
-- control FAN speed via FAN_I2C bus (MMC I2C, Channel 1).
-- Read and set values are available via register interface on APB Slave port.
--
-- Targeted device: <Family::SmartFusion> <Die::A2F500M3G> <Package::256 FBGA>
-- Author: Alessandro Malatesta
--
--------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--library USER;
use work.APB3_regbank_pkg.all;

library synplify;
use synplify.attributes.all;

--REGISTER MAP
--0x0:  READ  : TBD
--      WRITE : TBD
--                  (2) TBD
--                  (1) TBD
--                  (0) TBD

entity FAN_Controller is
generic(
    CLK_FREQ : natural := 50000000
    --Microsemi not accepting std_logic_vector generics. Using constants. Yes, I know: it sucks.
    --TMP112_I2C_ADDR_0 : std_logic_vector(6 downto 0) --7bit I2C address of TMP112 sensor
    --I2C_CORE_BASE : std_logic_vector(31 downto 0) := X"40050020"; --BASE + (CHAN<<5)
);
port (
    --
    PCLK    : in  std_logic;
    core_i2c_irq : in std_logic;
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
    --APB3 MASTER (I2C access)
    m_PRESETn : in  std_logic;
    m_PSELx   : out std_logic := '0';
    m_PENABLE : out std_logic := '0';
    m_PWRITE  : out std_logic := '0';
    m_PADDR   : out std_logic_vector(31 downto 0) := X"00000000";
    m_PWDATA  : out std_logic_vector(31 downto 0) := X"00000000";
    m_PRDATA  : in  std_logic_vector(31 downto 0);
    m_PREADY  : in  std_logic;
    m_PSLVERR : in  std_logic
);
end entity FAN_Controller;

architecture rtl of FAN_Controller is

--these constants replace generics (slv generics produce an error when launching the command "create core from HDL"
constant I2C_CORE_BASE : std_logic_vector(31 downto 0)    := X"00000000";
constant TMP112_I2C_ADDR_0 : std_logic_vector(6 downto 0) := "1111111"; --7bit I2C address of TMP112 sensor

----------------- COMPONENTS --------------------------------------------------------
component APB_master is
port(
    --control interface
    master_start : in  std_logic;
    master_write : in  std_logic;
    master_addr  : in  std_logic_vector(31 downto 0);
    master_wdata : in  std_logic_vector(31 downto 0);
    master_rdata : out std_logic_vector(31 downto 0);
    master_busy  : out std_logic;
    master_done  : out std_logic;
    --APB3 MASTER (I2C access)
    PCLK      : in  std_logic;
    m_PRESETn : in  std_logic;
    m_PSELx   : out std_logic := '0';
    m_PENABLE : out std_logic := '0';
    m_PWRITE  : out std_logic := '0';
    m_PADDR   : out std_logic_vector(31 downto 0) := X"00000000";
    m_PWDATA  : out std_logic_vector(31 downto 0) := X"00000000";
    m_PRDATA  : in  std_logic_vector(31 downto 0);
    m_PREADY  : in  std_logic;
    m_PSLVERR : in  std_logic
);
end component APB_master;

component I2C_core_controller is
generic(
    I2C_CORE_BASE : std_logic_vector(31 downto 0) := X"40050020" --BASE + (CHAN<<5)
);
port(
    PCLK      : in  std_logic;
    s_PRESETn : in  std_logic;
    --control interface
    execute   : in  std_logic;
    command   : in  std_logic_vector(3 downto 0);
    wdata     : in  std_logic_vector(7 downto 0);
    rdata     : out std_logic_vector(7 downto 0);
    done      : out std_logic;
    busy      : out std_logic;
    --APB_master interface
    master_start : out std_logic;
    master_write : out std_logic;
    master_addr  : out std_logic_vector(31 downto 0);
    master_wdata : out std_logic_vector(31 downto 0);
    master_rdata : in  std_logic_vector(31 downto 0);
    master_busy  : in  std_logic;
    master_done  : in  std_logic
);
end component I2C_core_controller;

---------- SIGNALS -----------------------------------------------------------------
attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of rtl : architecture is "tmr";

constant REGBANK_ADDR_W : natural := 4; --16registers
constant REGBANK_NREG : natural := 2**REGBANK_ADDR_W;

signal fabric2cpu_reg : slv32_array(0 to 2**REGBANK_ADDR_W-1);
signal cpu2fabric_reg : slv32_array(0 to 2**REGBANK_ADDR_W-1);

signal m_PRESETn_reg, s_PRESETn_reg : std_logic;

signal master_start, master_done, master_write, master_busy : std_logic;
signal master_wdata, master_rdata, master_addr : std_logic_vector(31 downto 0);

signal i2c_execute, i2c_done, i2c_busy : std_logic;
signal i2c_cmd : std_logic_vector(3 downto 0);
signal i2c_wdata, i2c_rdata : std_logic_vector(7 downto 0);

--I2C controller command codes
constant CMD_RESET_CTRL    : std_logic_vector(3 downto 0) := X"0";
constant CMD_WRITE_ENS1    : std_logic_vector(3 downto 0) := X"1";
constant CMD_WRITE_STA     : std_logic_vector(3 downto 0) := X"2";
constant CMD_WRITE_STO     : std_logic_vector(3 downto 0) := X"3";
constant CMD_WRITE_SI      : std_logic_vector(3 downto 0) := X"4";
constant CMD_WRITE_AA      : std_logic_vector(3 downto 0) := X"5";
constant CMD_WRITE_CLK_DIV : std_logic_vector(3 downto 0) := X"6";
constant CMD_WRITE_DATA    : std_logic_vector(3 downto 0) := X"7";
constant CMD_READ_DATA     : std_logic_vector(3 downto 0) := X"8";
constant CMD_READ_STAT     : std_logic_vector(3 downto 0) := X"9";

--main process
type m_s_t is (INIT_0, INIT_1, INIT_2, 
               IDLE, WAIT_DONE, INIT_STATUS, READ_STATUS, SAVE_STATUS, CHECK_IRQ, RESET_IRQ, 
               ISR, WRITE_START, WRITE_STOP, WRITE_AA, SEND_DATA, READ_DATA, SAVE_DATA,
               CHECK_RXCNT, STOP_AND_OK, FAIL, SUCCESS);
signal m_s, m_ns : m_s_t;
signal i2c_s : std_logic_vector(7 downto 0);
signal m_s_enc : std_logic_vector(4 downto 0);

--I2C controller status encoding
constant ST_START   : std_logic_vector(7 downto 0) := X"08"; --start has been sent
constant ST_RESTART : std_logic_vector(7 downto 0) := X"10"; --repeated start has been sent
constant ST_STOP    : std_logic_vector(7 downto 0) := X"E0"; --stop has been sent
constant ST_SLAW_A  : std_logic_vector(7 downto 0) := X"18"; --slave write addr sent, ack received
constant ST_SLAW_NA : std_logic_vector(7 downto 0) := X"20"; --slave write addr sent, nack received
constant ST_TXD_A   : std_logic_vector(7 downto 0) := X"28"; --data sent, ack received
constant ST_TXD_NA  : std_logic_vector(7 downto 0) := X"30"; --data sent, nack received
constant ST_ARB_LST : std_logic_vector(7 downto 0) := X"38"; --arbitration lost in SLAW/SLAR/TXD
constant ST_SLAR_A  : std_logic_vector(7 downto 0) := X"40"; --slave read addr sent, ack received
constant ST_SLAR_NA : std_logic_vector(7 downto 0) := X"48"; --slave read addr sent, nack received
constant ST_RXD_A   : std_logic_vector(7 downto 0) := X"50"; --data received, ack returned         
constant ST_RXD_NA  : std_logic_vector(7 downto 0) := X"58"; --data received, nack returned         

--following values are set before issuing a slave_start
constant MAX_BUF_SIZE : natural := 8;
signal slave_read : std_logic; --specifies a read transaction (0=write)
signal slave_start : std_logic; --starts transaction (FLS SHALL BE IDLE)
signal slave_addr : std_logic_vector(6 downto 0); --serial address of I2C targeted device
signal tx_count, tx_size, rx_count, rx_size : natural range 0 to MAX_BUF_SIZE-1;
type   byte_buf_t is array (0 to MAX_BUF_SIZE-1) of std_logic_vector(7 downto 0);
signal tx_buf, rx_buf : byte_buf_t;

signal transaction_dir : std_logic; --store current transaction direction
signal transaction_buf : std_logic_vector(7 downto 0);
signal transaction_status : std_logic_vector(3 downto 0); --0=nd, 1=ok, 2=fail, 3=timeout

--slave_addr
constant TC74_HEATSINK_ADDR : std_logic_vector(6 downto 0) := "1001101"; --0x4D
constant T112_OUTLET_F_ADDR : std_logic_vector(6 downto 0) := "1001000"; --0x48
constant T112_OUTLET_R_ADDR : std_logic_vector(6 downto 0) := "1001001"; --0x49
constant T112_BOARD_ADDR    : std_logic_vector(6 downto 0) := "1001010"; --0x4A
constant T112_INLET_ADDR    : std_logic_vector(6 downto 0) := "1001011"; --0x4B

--transaction_status encoding
constant TS_UNKNOWN : std_logic_vector(3 downto 0) := X"0";
constant TS_RUNNING : std_logic_vector(3 downto 0) := X"1";
constant TS_SUCCESS : std_logic_vector(3 downto 0) := X"2";
constant TS_FAILURE : std_logic_vector(3 downto 0) := X"3";
constant TS_TIMEOUT : std_logic_vector(3 downto 0) := X"4"; 

signal a_s : natural range 0 to 31;
signal temp_board : std_logic_vector(15 downto 0);
signal seq : unsigned(7 downto 0);
signal status_word, sw_r : std_logic_vector(31 downto 0);

begin

--sync reset for speed
sRST_REG_P : process(PCLK)
begin 
    if rising_edge(PCLK) then
        s_PRESETn_reg <= s_PRESETn;
    end if;
end process;

mRST_REG_P : process(PCLK)
begin 
    if rising_edge(PCLK) then
        m_PRESETn_reg <= m_PRESETn;
    end if;
end process;

--slave register bank (for data exchange with CPU)
REGBANK : APB3_Regbank
generic map(
    ADDR_W => REGBANK_ADDR_W
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
    regbank_in  => fabric2cpu_reg,
    regbank_out => cpu2fabric_reg
);

--ASSIGN REGISTERS FROM/TO APB REGISTER BANK ****************

--CPU to Fabric (commands)
-- <= cpu2fabric_reg(0);
-- <= cpu2fabric_reg(1);
-- <= cpu2fabric_reg(2);
-- <= cpu2fabric_reg(3);
-- <= cpu2fabric_reg(4);
-- <= cpu2fabric_reg(5);
-- <= cpu2fabric_reg(6);
-- <= cpu2fabric_reg(7);

--Fabric 2 CPU (status)
--fabric2cpu_reg(0)  <= X"00" & "00000" & "000" & X"08" & X"1A";
--fabric2cpu_reg(1)  <= X"01" & "00001" & "001" & X"10" & X"1B";
--fabric2cpu_reg(2)  <= X"02" & "00010" & "010" & X"18" & X"1C";
--fabric2cpu_reg(3)  <= X"03" & "00011" & "011" & X"20" & X"1D";
--fabric2cpu_reg(4)  <= X"04" & "00100" & "100" & X"28" & X"1E";
--fabric2cpu_reg(5)  <= X"05" & "00101" & "000" & X"30" & X"1F";
--fabric2cpu_reg(6)  <= X"06" & "00110" & "001" & X"38" & X"20";
--fabric2cpu_reg(7)  <= X"07" & "00111" & "010" & X"40" & X"21";
--fabric2cpu_reg(8)  <= X"08" & "01000" & "011" & X"48" & X"22";
--fabric2cpu_reg(9)  <= X"09" & "01001" & "100" & X"50" & X"23";
--fabric2cpu_reg(10) <= X"0A" & "01010" & "000" & X"58" & X"24";
--fabric2cpu_reg(11) <= X"0B" & "01011" & "001" & X"E0" & X"25";
--fabric2cpu_reg(12) <= X"0C" & "01100" & "010" & X"08" & X"26";
--fabric2cpu_reg(13) <= X"0D" & "01101" & "011" & X"10" & X"27";
--fabric2cpu_reg(14) <= X"0E" & "01110" & "100" & X"18" & X"28";
--fabric2cpu_reg(15) <= X"0F" & "01111" & "101" & X"20" & X"29";

-------------------------------------------------------------------------------------
---DEBUG STATUS MONITOR--------------------------------------------------------------
-- seq(8) | ms_enc(5) | ts(3) | i2c_s(8) | temp(15:8)
status_word <= std_logic_vector(seq) & m_s_enc & transaction_status(2 downto 0) & i2c_s & temp_board(15 downto 8);

with m_s select m_s_enc <=
    '0'&X"0" when INIT_0,
    '0'&X"1" when INIT_1,
    '0'&X"2" when INIT_2,
    '0'&X"3" when IDLE,
    '0'&X"4" when WAIT_DONE,
    '0'&X"5" when INIT_STATUS,
    '0'&X"6" when READ_STATUS,
    '0'&X"7" when SAVE_STATUS,
    '0'&X"8" when CHECK_IRQ,
    '0'&X"9" when RESET_IRQ,
    '0'&X"A" when ISR,
    '0'&X"B" when WRITE_START,
    '0'&X"C" when WRITE_STOP,
    '0'&X"D" when WRITE_AA,
    '0'&X"E" when SEND_DATA,
    '0'&X"F" when READ_DATA,
    '1'&X"0" when SAVE_DATA,
    '1'&X"1" when CHECK_RXCNT,
    '1'&X"2" when STOP_AND_OK,
    '1'&X"3" when FAIL,
    '1'&X"4" when SUCCESS;

DEBUG : process(PCLK)
begin
    if rising_edge(PCLK) then
        if s_PRESETn_reg = '0' then
            seq <= (others => '0');
        else
            sw_r <= status_word;
            if sw_r(23 downto 0) /= status_word(23 downto 0) then
                fabric2cpu_reg(to_integer(seq(REGBANK_ADDR_W-1 downto 0))) <= status_word;
                seq <= seq+1;
            end if;
        end if;
    end if;
end process;


-------------------------------------------------------------------------------------


--This process performs a single APB transaction.
--To do a WRITE transaction:
--  write address to master_addr
--  write data to master_wdata
--  write 1 to master_write
--  write 1 to master_start
--  wait until master_done = 1 (lasts only 1 clock cycle)
--To do a READ transaction:
--  write address to master_addr
--  write 0 to master_write
--  write 1 to master_start
--  read data from master_rdata when master_done = 1 (lasts only 1 clock cycle)
--When busy = 1 inputs shall not be changed and no start is accepted.
APB_MASTER_I : APB_master
port map(
    --control interface
    master_start => master_start,
    master_write => master_write,
    master_addr  => master_addr,
    master_wdata => master_wdata,
    master_rdata => master_rdata,
    master_done  => master_done,
    master_busy  => master_busy,
    --APB3 MASTER (I2C access)
    PCLK      => PCLK,
    m_PRESETn => m_PRESETn_reg,
    m_PSELx   => m_PSELx,
    m_PENABLE => m_PENABLE,
    m_PWRITE  => m_PWRITE,
    m_PADDR   => m_PADDR,
    m_PWDATA  => m_PWDATA,
    m_PRDATA  => m_PRDATA,
    m_PREADY  => m_PREADY,
    m_PSLVERR => m_PSLVERR
);

--this provides a command-based interface to control the I2C core
I2C_CTRL_I : I2C_core_controller
generic map(
    I2C_CORE_BASE => I2C_CORE_BASE
)
port map(
    PCLK      => PCLK,
    s_PRESETn => s_PRESETn_reg,
    --control interface
    execute   => i2c_execute,
    command   => i2c_cmd,
    wdata     => i2c_wdata,
    rdata     => i2c_rdata,
    done      => i2c_done,
    busy      => i2c_busy,
    --APB_master interface
    master_start => master_start,
    master_write => master_write,
    master_addr  => master_addr,
    master_wdata => master_wdata,
    master_rdata => master_rdata,
    master_busy  => master_busy,
    master_done  => master_done
);

--this controls the I2C core so that transactions are correctly generated.
--its outputs are commands for the I2C_core_controller component
--To generate a transaction:
--  1. set slave_read (1=read, 0=write)
--  2. set slave_addr (7-bit i2c address)
--  3. if WRITE: fill tx_buf with address & data, and set tx_size to the number of bytes to send (incl. addr);
--     if READ : write address to tx_buf(0), set tx_size = 1 and rx_size to the number of bytes to receive
--  4. wait until m_s is IDLE
--  5. set slave_start to '1' (release when transaction_Status = TS_RUNNING)
--  6. wait until transaction_status /= TS_RUNNING
--  7. check if transaction_status = TS_SUCCESS. If READ, data in rxtx_buf is valid.
I2C_FSM_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if s_PRESETn_reg = '0' then
            i2c_execute   <= '0';
            m_s        <= INIT_0;
        else
            case m_s is
            --------------------INIT SECTION ---------------------
            when INIT_0 =>
                --reset ctrl register
                if i2c_busy = '0' then
                    i2c_cmd     <= CMD_RESET_CTRL;
                    i2c_execute <= '1';
                    m_s         <= WAIT_DONE;
                    m_ns        <= INIT_1;
                end if;
            when INIT_1 =>
                --enable i2c core
                if i2c_busy = '0' then
                    i2c_cmd      <= CMD_WRITE_ENS1;
                    i2c_wdata(0) <= '1';
                    i2c_execute  <= '1';
                    m_s          <= WAIT_DONE;
                    m_ns         <= INIT_2;
                end if;
            when INIT_2 =>
                --set clock divider ("011" = /160)
                if i2c_busy = '0' then
                    i2c_cmd               <= CMD_WRITE_CLK_DIV;
                    i2c_wdata(2 downto 0) <= "011";
                    i2c_execute           <= '1';
                    m_s                   <= WAIT_DONE;
                    m_ns                  <= IDLE; --TODO eventually add a clear interrupt step
                end if;
            --------------------MAIN SECTION ---------------------
            when IDLE =>
                tx_count <= 0;
                rx_count <= 0;
                transaction_dir <= '0'; --first transaction is always write
                --send start
                i2c_cmd      <= CMD_WRITE_STA;
                i2c_wdata(0) <= '1';

                if slave_start = '1' and i2c_busy = '0' then 
                    transaction_status <= TS_RUNNING;
                    i2c_execute  <= '1';
                    m_s          <= WAIT_DONE;
                    m_ns         <= INIT_STATUS; --then get status to init ISR state machine
                end if;
            when INIT_STATUS => --first STATUS read after slave_start
                --send READ STATUS command
                i2c_cmd      <= CMD_READ_STAT;
                if i2c_busy = '0' then
                    i2c_execute  <= '1';
                    m_s          <= SAVE_STATUS; --don't use WAIT_DONE status (save 1 transition)
                    m_ns         <= CHECK_IRQ;
                end if;
            when CHECK_IRQ =>
                if core_i2c_irq = '1' then
                    m_s  <= RESET_IRQ;
                    m_ns <= READ_STATUS;
                end if;
            when RESET_IRQ =>
                i2c_cmd      <= CMD_WRITE_SI;
                i2c_wdata(0) <= '0';
                if i2c_busy = '0' then
                    i2c_execute  <= '1';
                    m_s          <= WAIT_DONE;
                    --m_ns         <= m_ns; --set by previous state
                end if;
            when READ_STATUS =>
                --send READ STATUS command
                i2c_cmd      <= CMD_READ_STAT;
                if i2c_busy = '0' then
                    i2c_execute  <= '1';
                    m_s          <= SAVE_STATUS; --don't use WAIT_DONE status (save 1 transition)
                    m_ns         <= ISR;
                end if;
            when SAVE_STATUS =>
                --read_stat command sent: update i2c_s directly
                i2c_execute <= '0';
                if i2c_done = '1' then
                    i2c_s <= i2c_rdata; --i2c_s is used as status for ISR FSM
                    m_s   <= m_ns;
                end if;

            --------------------THE INTERRUPT SERVICE ROUTINE------------------------
            when ISR =>
                case i2c_s is
                when ST_START | ST_RESTART =>
                    transaction_buf <= slave_addr & transaction_dir;
                    i2c_wdata(0)    <= '0'; --reset start bit
                    m_s             <= WRITE_START;
                    m_ns            <= SEND_DATA; 
                    --after SLAW the i2c_s is automatically updated
                when ST_ARB_LST =>
                    i2c_wdata(0) <= '1'; --retry start
                    m_s          <= WRITE_START; 
                    m_ns         <= CHECK_IRQ; --and wait for next event
                when ST_STOP => --TODO: check how we get out of here
                    --do nothing
                    m_s <= CHECK_IRQ;
                -------------MASTER TRANSMITTER--------------------
                when ST_SLAW_NA | ST_TXD_NA => --merged: original ISR had 2 separate identical states
                    i2c_wdata(0) <= '1';
                    m_s  <= WRITE_STOP;
                    m_ns <= FAIL;
                when ST_SLAW_A | ST_TXD_A =>
                    if tx_count < tx_size then --transmit
                        i2c_wdata <= tx_buf(tx_count);
                        --tx_count  <= tx_count + 1; incremented in SEND_DATA state
                        m_s       <= SEND_DATA;
                        --following state is always CHECK_IRQ
                    elsif slave_read = '1' then --SLA+R was just sent
                        transaction_dir <= '1'; --change direction from "write" to "read"
                        i2c_wdata(0)    <= '1'; --send restart
                        m_s  <= WRITE_START;
                        m_ns <= CHECK_IRQ;
                    else --over: send stop
                        i2c_wdata(0) <= '1';
                        m_s  <= WRITE_STOP;
                        m_ns <= SUCCESS;
                    end if;
                -------------MASTER RECEIVER-----------------------
                when ST_SLAR_A =>
                    if rx_size > 1 then --READ bigger than 1 byte
                        i2c_wdata(0) <= '1'; --set to return ACK on received bytes (NACK only for last one)
                        m_s  <= WRITE_AA;
                        m_ns <= CHECK_IRQ;
                    elsif rx_size = 1 then --1 byte read
                        i2c_wdata(0) <= '0'; --set to return NACK on received byte
                        m_s  <= WRITE_AA;
                        m_ns <= CHECK_IRQ;
                    else --rx_size = 0 => write
                        i2c_wdata(0) <= '1'; --set to return ACK on received bytes (NACK only for last one)
                        m_s  <= WRITE_AA;
                        m_ns <= STOP_AND_OK;
                    end if;
                when ST_SLAR_NA =>
                    i2c_wdata(0) <= '1';
                    m_s  <= WRITE_STOP;
                    m_ns <= FAIL;
                when ST_RXD_A =>
                    m_s  <= READ_DATA;
                    m_ns <= CHECK_RXCNT;
                when ST_RXD_NA =>
                    --read last data then send stop
                    m_s  <= READ_DATA;
                    m_ns <= STOP_AND_OK;

                when others =>
                    i2c_wdata(0) <= '0'; --clear start bit to allow for further transactions
                    m_s <= WRITE_START;
                    m_ns <= FAIL;
                end case;

            when STOP_AND_OK =>
                i2c_wdata(0) <= '1';
                m_s  <= WRITE_STOP;
                m_ns <= SUCCESS;

            when WRITE_START =>
                --set/reset start bit
                i2c_cmd      <= CMD_WRITE_STA;
                if i2c_busy = '0' then
                    --i2c_wdata(0) <= preset
                    i2c_execute  <= '1';
                    m_s          <= WAIT_DONE;
                    --m_ns         <= m_ns; --set by previous state
                end if;
           
            when WRITE_STOP =>
                --set/reset stop bit
                i2c_cmd      <= CMD_WRITE_STO;
                if i2c_busy = '0' then
                    --i2c_wdata(0) <= preset
                    i2c_execute  <= '1';
                    m_s          <= WAIT_DONE;
                    --m_ns         <= m_ns; --set by previous state
                end if;
           
            when WRITE_AA =>
                --set/reset Ack control bit
                i2c_cmd      <= CMD_WRITE_AA;
                if i2c_busy = '0' then
                    --i2c_wdata(0) <= preset
                    i2c_execute  <= '1';
                    m_s          <= WAIT_DONE;
                    --m_ns         <= m_ns; --set by previous state
                end if;

            when SEND_DATA =>
                i2c_wdata <= transaction_buf;
                i2c_cmd   <= CMD_WRITE_DATA;
                --send data byte
                if i2c_busy = '0' then
                    --i2c_wdata <= already set
                    i2c_execute <= '1';
                    tx_count    <= tx_count+1;
                    m_s         <= WAIT_DONE;
                    m_ns        <= CHECK_IRQ; 
                end if;

            when READ_DATA =>
                --send READ DATA command
                i2c_cmd      <= CMD_READ_DATA;
                if i2c_busy = '0' then
                    i2c_execute  <= '1';
                    m_s          <= SAVE_DATA; --don't use WAIT_DONE status (save 1 transition)
                    --m_ns         <= ISR;
                end if;
            when SAVE_DATA =>
                --read_stat command sent: update i2c_s directly
                i2c_execute <= '0';
                if i2c_done = '1' then
                    rx_buf(rx_count) <= i2c_rdata; --store data
                    rx_count         <= rx_count+1;
                    m_s              <= m_ns;
                end if;

            when CHECK_RXCNT =>
                if rx_count = (rx_size-1) then --replaced >= with = (counter shall not jump)
                    i2c_wdata(0) <= '0'; --prepare NACK for last byte
                    m_s          <= WRITE_AA;
                    m_ns         <= CHECK_IRQ;
                else
                    m_s          <= CHECK_IRQ;
                end if;

            when WAIT_DONE =>
                i2c_execute <= '0';
                if i2c_done = '1' then
                    m_s <= m_ns;
                end if;

            when SUCCESS =>
                transaction_status <= TS_SUCCESS;
                m_s <= IDLE;

            when FAIL =>
                transaction_status <= TS_FAILURE;
                m_s <= IDLE;

            when others =>
                i2c_execute <= '0';
                m_s         <= INIT_0;
            end case;

        end if;
    end if;
end process; --I2C_FSM_P

--The following process implements the application layer:
--Wait for initialization to be over, then read the temperature periodically
APP_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if s_PRESETn_reg = '0' then
            slave_start <= '0';
            a_s <= 0;
        else 
            case a_s is
            when 0 =>
                if m_s = IDLE then
                    --read register 0 of BOARD sensor
                    tx_buf(0) <= X"00";
                    tx_size     <= 1;
                    rx_size     <= 2;
                    slave_read  <= '1';
                    slave_addr  <= T112_BOARD_ADDR;
                    slave_start <= '1';
                    a_s <= a_s+1;
                end if;
            when 1 =>
                if transaction_status = TS_RUNNING then
                    slave_start <= '0';
                    a_s <= a_s+1;
                end if;
            when 2 =>
                if transaction_status = TS_SUCCESS then
                    --[rx_buf(0) & rx_buf(1)] >> 4
                    temp_board <= rx_buf(0) & rx_buf(1);
                    a_s <= a_s+1;
                elsif transaction_status = TS_FAILURE then
                    temp_board <= X"FFFF";
                    a_s <= a_s+1;
                --else wait
                end if;
            when others =>
                --always restart
                a_s <= 0;
            end case;
        end if;
    end if;
end process;

end architecture rtl; --of FAN_controller





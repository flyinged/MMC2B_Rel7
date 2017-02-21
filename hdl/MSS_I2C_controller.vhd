--------------------------------------------------------------------------------
-- Company: PSI
--
-- File: MSS_I2C_controller.vhd
-- File history:
--      1.0 : 19/11/2015 : Creation
--      1.1 : 11/10/2016 : User-settable fan speed
--
-- Description: 
-- APB master port used to control MSS_I2C via MSS_SLAVE_APB interface
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



entity MSS_I2C_controller is
generic(
    CLK_FREQ : natural   := 50000000;
    FAN_RPM_FRONT_TOP    : natural := 5000;
    FAN_RPM_FRONT_BOTTOM : natural := 9000;
    FAN_RPM_BACK         : natural := 3000
);
port (
    --
    PCLK    : in  std_logic;
    --Fan control
    fan_reset        : in std_logic; --when 1, fan speeds are reset to default values
    fan_speedup_en   : in std_logic; --when 1, fan speed is increased by RPM_STEP every STEP_INTERVAL seconds
    fan_speedup_step : in std_logic_vector(15 downto 0);
    fan_speedup_int  : in std_logic_vector( 7 downto 0);
    --at least 1 enable must be asserted in order to allow I2C slave access
    i2c_slave_en1_n : in std_logic; 
    i2c_slave_en2_n : in std_logic; 
    mss_int : in std_logic_vector(7 downto 0);
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
end entity MSS_I2C_controller;

architecture rtl of MSS_I2C_controller is

----------------- COMPONENTS --------------------------------------------------------
component APB_master is
generic(
    WATCHDOG_EN : std_logic := '0';
    TIMEOUT : natural := 5000000
);
port(
    --control interface
    master_rst   : in  std_logic;
    master_start : in  std_logic;
    master_write : in  std_logic;
    master_addr  : in  std_logic_vector(31 downto 0);
    master_wdata : in  std_logic_vector(31 downto 0);
    master_rdata : out std_logic_vector(31 downto 0);
    master_busy  : out std_logic;
    master_done  : out std_logic;
    master_error : out std_logic;
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

component APB3_Regbank_ram is
    generic(
        DEBUG : std_logic := '0'; --address 0 direct
        ALLOW_WRITE : std_logic := '0';
        ADDR_W : natural := 4 --DWORD address
    );
    port (
        DBG_DATA : in  std_logic_vector(31 downto 0);
        --APB3
        PCLK    : in  std_logic;
        PRESETn : in  std_logic;
        PSEL    : in  std_logic;
        PENABLE : in  std_logic;
        PWRITE  : in  std_logic;
        PADDR   : in  std_logic_vector(ADDR_W+2-1 downto 0); --BYTE address
        PWDATA  : in  std_logic_vector(31 downto 0);
        PRDATA  : out std_logic_vector(31 downto 0);
        PREADY  : out std_logic;
        PSLVERR : out std_logic;
        --RAM interface
        we_n : out std_logic;
        addr : out  std_logic_vector(ADDR_W-1 downto 0); --DWORD address
        wdata : out std_logic_vector(31 downto 0);
        rdata  : in  std_logic_vector(31 downto 0)
);
end component APB3_Regbank_ram;

--component fifo8x32b is
--
--    port( DATA   : in    std_logic_vector(31 downto 0);
--          Q      : out   std_logic_vector(31 downto 0);
--          WE     : in    std_logic;
--          RE     : in    std_logic;
--          WCLOCK : in    std_logic;
--          RCLOCK : in    std_logic;
--          FULL   : out   std_logic;
--          EMPTY  : out   std_logic;
--          RESET  : in    std_logic
--        );
--
--end component fifo8x32b;


component myfifo is
generic(
    LOG_DEPTH : natural := 3 --DEPTH = 2^LOG_DEPTH 
);
port( 
    DATA   : in  std_logic_vector(31 downto 0);
    Q      : out std_logic_vector(31 downto 0);
    WE     : in  std_logic;
    RE     : in  std_logic;
    CLOCK : in  std_logic;
    FULL   : out std_logic;
    EMPTY  : out std_logic;
    RESET  : in  std_logic
);
end component myfifo;

---------- SIGNALS -----------------------------------------------------------------
attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of rtl : architecture is "tmr";

constant REGBANK_ADDR_W : natural := 7; --128 DWORDS
constant REGBANK_NREG : natural := 2**REGBANK_ADDR_W;

--signal fabric2cpu_reg : slv32_array(0 to 2**REGBANK_ADDR_W-1);
--signal cpu2fabric_reg : slv32_array(0 to 2**REGBANK_ADDR_W-1);
signal dina, douta, dinb, doutb : std_logic_vector(31 downto 0);
signal addra : std_logic_vector(REGBANK_ADDR_W-1 downto 0); --dword address
signal addrb : unsigned(REGBANK_ADDR_W-1 downto 0); --dword address
signal wea_n, web_n : std_logic;

signal m_PRESETn_reg, s_PRESETn_reg, i2c_slave_en : std_logic := '0';

signal master_start, master_error, master_done, master_write, master_busy : std_logic := '0';
signal master_wdata, master_rdata, master_addr : std_logic_vector(31 downto 0) := (others => '0');

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

--MSS addresses 
constant MSS_I2C : std_logic := '1';
constant MSS_SOFT_RST_CR  : std_logic_vector(31 downto 0) := X"E0042030"; --peripheral reset register
constant MSS_IRQ_ENABLE   : std_logic_vector(31 downto 0) := X"E000E100"; --irq enable register
constant FIIC_MR          : std_logic_vector(31 downto 0) := X"40007040"; --FIIC mode register
constant FIIC_MSSIRQEN_0  : std_logic_vector(31 downto 0) := X"40007000"; --FIIC mss interrupt enables
constant FIIC_MSSIRQSRC_0 : std_logic_vector(31 downto 0) := X"40007020"; --FIIC mss interrupt enables

--MSS I2C core parameters
constant I2C_CTRL_DIV     : std_logic_vector(7 downto 0)  := X"00";       --I2C divider settings (25M/256=97kHz, max 100kHz for fan controller)
constant I2C_CTRL_AA      : std_logic_vector(7 downto 0)  := X"04";       --I2C acknowledge enable       
constant I2C_CTRL_STO     : std_logic_vector(7 downto 0)  := X"10";       --I2C acknowledge enable       
constant I2C_CTRL_STA     : std_logic_vector(7 downto 0)  := X"20";       --I2C acknowledge enable       
constant I2C_CTRL_ENA     : std_logic_vector(7 downto 0)  := X"40";       --I2C acknowledge enable       
constant I2C_CTRL_SI_BIT  : natural := 3;
constant I2C_WRITE        : std_logic := '0';
constant I2C_READ         : std_logic := '1';

signal I2C_RST_BIT : natural range 0 to 31;
signal I2C_IRQ_BIT : natural range 0 to 31;
signal I2C_MSSIRQ_BIT : natural range 0 to 7;
signal I2C_RST_MASK : std_logic_vector(31 downto 0);
signal I2C_IRQ_MASK : std_logic_vector(31 downto 0);
signal I2C_CTRL   : std_logic_vector(31 downto 0);
signal I2C_STATUS : std_logic_vector(31 downto 0);
signal I2C_DATA   : std_logic_vector(31 downto 0);
signal I2C_GLITCH : std_logic_vector(31 downto 0);

--FAN CONTROLLER SPECIFIC (MAX31785)
type addr_array_t is array (0 to 1) of std_logic_vector(6 downto 0);
constant FAN_CTRL_ADDR  : addr_array_t := ("1010010", "1010011"); --X"52", X"53" Fan controller 1&2 I2C serial addresses
signal fc_id : natural range 0 to 1;

--PAGES 0 to 4 => FANs (other pages not used)
constant FAN_PAGE        : std_logic_vector(7 downto 0) := X"00"; --page select (1B) 
constant FAN_CLR_FAULT   : std_logic_vector(7 downto 0) := X"03"; --clear faults
constant FAN_WR_PROTECT  : std_logic_vector(7 downto 0) := X"10"; --control write protection
constant FAN_STORE_ALL   : std_logic_vector(7 downto 0) := X"11"; --store all settings in NVM
constant FAN_RESTORE_ALL : std_logic_vector(7 downto 0) := X"12"; --restore settings from NVM

--COMMON REGISTERS
--FAN_STATUS bits: 1=comm fault, 10=fan fault 
constant FAN_STATUS_G    : std_logic_vector(7 downto 0) := X"79"; --general status word (2B, common)
--FAN_STATUS_CML bits: 7=rx invalid command, 6=rx invalid data, 0=fault log full
constant FAN_STATUS_CML  : std_logic_vector(7 downto 0) := X"7E"; --communication faults (1B, common)
--FAN_MFR_MODE bits: 15=force nvm log, 14=clear nvm log, 13=enable ARA, 11=soft reset (1,0,1)
-- 7:6=fan health criteria, 5:0=voltage sense enable
constant FAN_MFR_MODE    : std_logic_vector(7 downto 0) := X"D1"; --settings (2B, common)          

--PAGE REGISTERS
--FAN_CFG12 Bits: 7=enable, 6=RPM/PWM#, 5:4=tach pulses, 
constant FAN_CFG12       : std_logic_vector(7 downto 0) := X"3A"; --fan configuration (1B, per page)
constant FAN_CMD         : std_logic_vector(7 downto 0) := X"3B"; --fan command (2B, per page)
--FAN_STATUS_FAN bits: 7=fan fault, 5=fan warn, 3=bad health, 2=medium health, 1=unstable speed, 0=good health
constant FAN_STATUS_FAN  : std_logic_vector(7 downto 0) := X"81"; --fan specific faults (1B, per page)
constant FAN_TEMP        : std_logic_vector(7 downto 0) := X"8D"; --fan temperature (internal page 12)
constant FAN_SPEED       : std_logic_vector(7 downto 0) := X"90"; --read fan speed (2B, per page) 
--FAN_FAULT_RESP bits: 7=log faults, 2=enable fault# pin, 0=force fan to 100% on fault#
constant FAN_FAULT_RESP  : std_logic_vector(7 downto 0) := X"D9"; --fault response (1 per page) 
constant FAN_FAULT_LOG   : std_logic_vector(7 downto 0) := X"DC"; --fault log
constant FAN_TIME_CNT    : std_logic_vector(7 downto 0) := X"DD"; --time counter (4B, common)
--FAN_CONFIG bits: 15:13=pwm freq, 12=enable dual tach, 11:10=hyst for auto mode, 
--                 9=0 =>100% on missing cmd update, 8=0 =>100% on fan fault
--                 7:5=ramp speed, 4=health check en, 3=tach pol for stop, 2=use tach as lock, 1:0=spinup ctrl
constant FAN_MFR_CFG     : std_logic_vector(7 downto 0) := X"F1"; --MFR fan config (2B, per page)
constant FAN_PWM         : std_logic_vector(7 downto 0) := X"F3"; --read fan pwm  (2B, per page)  
constant FAN_RUN_TIME    : std_logic_vector(7 downto 0) := X"F7"; --read fan run time  (2B, per page)
constant FAN_AVG_PWM     : std_logic_vector(7 downto 0) := X"F8"; --read fan average PWM  (2B, per page)
--constant FAN_PWM2RPM     : std_logic_vector(7 downto 0) := X"F8"; --




--registers local copy
signal mss_int_reg          : std_logic_vector( 7 downto 0) := (others => '0');
signal mss_soft_rst_cr_reg  : std_logic_vector(31 downto 0) := (others => '0');
signal mss_irq_enable_reg   : std_logic_vector(31 downto 0) := (others => '0');
signal fiic_mr_reg          : std_logic_vector(31 downto 0) := (others => '0');
signal fiic_mssirqen_0_reg  : std_logic_vector(31 downto 0) := (others => '0');
signal fiic_mssirqsrc_0_reg : std_logic_vector(31 downto 0) := (others => '0');
signal i2c_ctrl_reg         : std_logic_vector(7 downto 0) := (others => '0');
signal i2c_status_reg       : std_logic_vector(7 downto 0) := (others => '0');
--signal i2c_glitch_reg       : std_logic_vector(31 downto 0) := (others => '0');

--FSMs and control signals
constant I2C_TIMEOUT : natural := 3*CLK_FREQ;
--signal irq_timer     : natural range 0 to 255; --count IRQ polling interval (DEBUG)
signal i2c_tout      : std_logic := '0';
signal i2c_timeout_c : natural range 0 to I2C_TIMEOUT; --count i2c transaction timeout
signal s             : std_logic_vector(7 downto 0) := (others => '0');
signal as            : unsigned(7 downto 0)         := (others => '0');
signal i2c_tstatus   : std_logic_vector(1 downto 0) := (others => '0'); --b1=transaction running b0=fail/success#
signal i2c_ack_rx    : std_logic_vector(7 downto 0) := (others => '0'); --set to I2C_CTRL_AA to send ack on receive, 0 otherwise
signal i2c_send_stop : std_logic_vector(7 downto 0) := (others => '0'); --set to I2C_CTRL_STO to send STOP, 0 otherwise
signal i2c_nbytes    : unsigned(2 downto 0)         := (others => '0'); --number of bytes to read/write 
signal i2c_err_cnt   : unsigned(31 downto 0)        := (others => '0');
--data buffers
signal i2c_rx_data : std_logic_vector(31 downto 0) := (others => '0');
alias  i2c_rx_b0 : std_logic_vector(7 downto 0) is i2c_rx_data( 7 downto  0);
alias  i2c_rx_b1 : std_logic_vector(7 downto 0) is i2c_rx_data(15 downto  8);
alias  i2c_rx_b2 : std_logic_vector(7 downto 0) is i2c_rx_data(23 downto 16);
alias  i2c_rx_b3 : std_logic_vector(7 downto 0) is i2c_rx_data(31 downto 24);
signal i2c_tx_data, i2c_tx_sr  : std_logic_vector(31 downto 0) := (others => '0'); 
--alias  i2c_tx_b0 : std_logic_vector(7 downto 0) is i2c_tx_data( 7 downto  0);
--alias  i2c_tx_b1 : std_logic_vector(7 downto 0) is i2c_tx_data(15 downto  8);
--alias  i2c_tx_b2 : std_logic_vector(7 downto 0) is i2c_tx_data(23 downto 16);
--alias  i2c_tx_b3 : std_logic_vector(7 downto 0) is i2c_tx_data(31 downto 24);

--i2c control
signal i2c_opcode     : std_logic_vector(18 downto 0) := (others => '0'); --i2c_slave_addr & i2c_rnw & i2c_tot_bytes & i2c_reg
signal busy, start    : std_logic := '0';
signal i2c_slave_addr : std_logic_vector(6 downto 0) := (others => '0');
signal i2c_rnw        : std_logic := '0';
signal i2c_tot_bytes  : unsigned(2 downto 0) := (others => '0');
signal i2c_reg_addr   : std_logic_vector(7 downto 0) := (others => '0');

signal page : unsigned(3 downto 0) := (others => '0');
signal fan_speed1, fan_speed2, fan_speed3 : unsigned(15 downto 0);
signal set_rpm_front_top, set_rpm_front_bottom, set_rpm_back : unsigned(15 downto 0);
signal sec_cnt : natural range 0 to CLK_FREQ;
signal fan_step_tmr : unsigned(7 downto 0);

constant RSEQ_LAST : natural := 12;
type rd_rom is array (0 to RSEQ_LAST) of std_logic_vector(11 downto 0);
constant read_seq : rd_rom := ( --rnw, nbytes(3), reg_addr
    '1' & "010" & FAN_STATUS_G,   --0
    '1' & "010" & FAN_MFR_MODE,   --1
    '1' & "100" & FAN_TIME_CNT,   --2
    '0' & "001" & FAN_PAGE,       --3 
    '1' & "001" & FAN_CFG12,      --4
    '1' & "010" & FAN_MFR_CFG,    --5
    '1' & "001" & FAN_FAULT_RESP, --6
    '1' & "001" & FAN_STATUS_FAN, --7
    '1' & "010" & FAN_CMD,        --8
    '1' & "010" & FAN_SPEED,      --9
    '1' & "010" & FAN_RUN_TIME,   --10
    '1' & "010" & FAN_PWM,        --11
    '1' & "010" & FAN_AVG_PWM);    --12

constant WSEQ_LAST : natural := 15;
type wr_rom is array (0 to WSEQ_LAST) of std_logic_vector(34 downto 0);
--I2C_ADDR(7) & DATA(16) & ENA(1) & NBYTES(3) & REG_ADDR(8)
signal write_seq : wr_rom;

signal seq : natural range 0 to WSEQ_LAST := 0; --max between WSEQ_LAST and RSEQ_LAST

--timers
constant APP_UPDATE : natural := CLK_FREQ; --update registers every 1000 ms
signal wd_tmr : natural range 0 to I2C_TIMEOUT := 0;
signal wd_rst, wd_tout : std_logic := '0';

constant I2C_PAUSE : natural := CLK_FREQ/1000; --pause 1ms between transactions
signal p_tmr : natural := 0;
signal p_rst, p_tout : std_logic := '0';

--debug
--signal dbg_reg : std_logic_vector(31 downto 0) := (others => '0');
--signal shift : std_logic_vector(31 downto 0) := (others => '0');
--signal s_r   : std_logic_vector(7 downto 0) := (others => '0');

signal fifo_dout : std_logic_vector(31 downto 0);
signal fifo_wr, fifo_rd, fifo_full, fifo_empty, fifo_rst : std_logic;

--debug
signal fan_rst_cnt : unsigned(15 downto 0);
signal fifo_cnt : unsigned(15 downto 0);
signal fifo_dat, dbg_data : std_logic_vector(31 downto 0);

signal reset_command, reset_command_r : std_logic; --v32 added
signal tick : std_logic_vector(1 downto 0); --v1.1
-----------------------------------------------------------------------------------
begin

--set I2C parameters according to which I2C core is used
I2C0_GEN: if MSS_I2C = '0' generate
    I2C_RST_BIT  <= 11;
    I2C_IRQ_BIT  <= 14;
    I2C_MSSIRQ_BIT  <= 0;
    I2C_RST_MASK <= X"00000800";
    I2C_IRQ_MASK <= X"00004000";
    I2C_CTRL     <= X"40002000";
    I2C_STATUS   <= X"40002004";
    I2C_DATA     <= X"40002008";
    I2C_GLITCH   <= X"40002018";
end generate;

I2C1_GEN: if MSS_I2C = '1' generate
    I2C_RST_BIT  <= 12;
    I2C_IRQ_BIT  <= 17;
    I2C_MSSIRQ_BIT  <= 5;
    I2C_RST_MASK <= X"00001000";
    I2C_IRQ_MASK <= X"00020000";
    I2C_CTRL     <= X"40012000";
    I2C_STATUS   <= X"40012004";
    I2C_DATA     <= X"40012008";
    I2C_GLITCH   <= X"40012018";
end generate;

--sync for speed
SPEED_REG_P : process(PCLK)
begin 
    if rising_edge(PCLK) then
        s_PRESETn_reg <= '1'; --s_PRESETn;
        m_PRESETn_reg <= '1'; --m_PRESETn;
        mss_int_reg   <= mss_int;
        reset_command_r <= reset_command; --v32 added
        --power enabled when at least one signal is asserted
        if ((i2c_slave_en1_n = '0' or i2c_slave_en2_n = '0') and reset_command_r = '0') then --v32 added reset_command
            i2c_slave_en <= '1';
        else
            i2c_slave_en <= '0';
        end if;
    end if;
end process;


--slave register bank (for data exchange with CPU)
REGBANK : APB3_Regbank_ram
generic map(
    DEBUG => '1',
    ALLOW_WRITE  => '1',
    ADDR_W => REGBANK_ADDR_W
)
port map(
    DBG_DATA => dbg_data,
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
    --RAM interface
    we_n   => wea_n,
    addr   => addra, --: out  std_logic_vector(ADDR_W-1 downto 0); --DWORD address
    wdata  => dina, --: out std_logic_vector(31 downto 0);
    rdata  => douta --: in  std_logic_vector(31 downto 0)
);

--v32 added status
DBG_DATA_R : process(PCLK)
begin
    if rising_edge(PCLK) then
        --s(7:0) MAIN FSM status
        --i2c_status_reg(7:0) I2C_FSM
        --as(7:0) Application FSM status
        --i2c_tstatus 00=idle, 10=running, 11=running,error, 01=over,error
        dbg_data <= (X"4" & "00" & i2c_tstatus) & std_logic_vector(as) & i2c_status_reg & s; --version
    end if;
end process;

REGBANK_RAM : ram_128x32
port map( 
    --connected to APB interface
    ena   => '1',
    dina  => dina,
    douta => douta,
    addra => addra,
    wea_n => wea_n,
    --USER port
    enb   => '1',
    dinb  => dinb,
    doutb => doutb,
    addrb => std_logic_vector(addrb),
    web_n => web_n,

    clk   => PCLK
);

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
generic map( --v32 added timeout
    WATCHDOG_EN => '1',
    TIMEOUT => CLK_FREQ
)
port map(
    --control interface
    master_rst   => reset_command_r, --v32 added
    master_start => master_start,
    master_write => master_write,
    master_addr  => master_addr,
    master_wdata => master_wdata,
    master_rdata => master_rdata,
    master_done  => master_done,
    master_error => master_error,
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


MAIN : process(PCLK)
begin
    if rising_edge(PCLK) then
        if m_PRESETn_reg = '0' or s_PRESETn_reg = '0' or i2c_slave_en = '0' then --v32:added i2c_slave_en
            s <= X"00";
            master_start <= '0';
            i2c_tstatus <= "11"; --init
            i2c_err_cnt <= (others => '0');
        else
            case s is
            --SETUP 
            when X"00" => --read reset register
                if master_busy = '0' then
                    master_write <= '0';
                    master_addr  <= MSS_SOFT_RST_CR;
                    master_start <= '1';
                    s <= X"01";
                end if;
            when X"01" => --check reset register
                master_start <= '0';
                if master_done = '1' then
                    mss_soft_rst_cr_reg <= master_rdata; --store value
                    --if (master_rdata(I2C_RST_BIT) = '1') then
                        ----i2c under reset: release
                        --s <= X"02";
                    --else
                        s <= X"04"; --I2C core out of reset, go on
                    --end if;
                end if;
            when X"02" => --start RST_CR write transaction
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '1';
                    master_addr  <= MSS_SOFT_RST_CR;
                    master_wdata <= mss_soft_rst_cr_reg and (not I2C_RST_MASK);
                    s <= X"03";
                end if;
            when X"03" => --wait for end of RST_CR write transaction
                master_start <= '0';
                if master_done = '1' then
                    s <= X"00"; --read reset register again
                end if;
            ----------------------------------------------------------------------
            when X"04" => --read interrupt enable vector
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '0';
                    master_addr  <= MSS_IRQ_ENABLE;
                    s <= X"05";
                end if;
            when X"05" => --get irw enable vector
                master_start <= '0';
                if master_done = '1' then
                    mss_irq_enable_reg <= master_rdata; --store value
                    --if (master_rdata(I2C_IRQ_BIT) = '0') then
                        ----irq disabled: enable
                        --s <= X"06";
                    --else
                        s <= X"08"; --go on
                    --end if;
                end if;
            when X"06" => --start IRQ_EN write transaction
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '1';
                    master_addr  <= MSS_IRQ_ENABLE;
                    master_wdata <= mss_irq_enable_reg or I2C_IRQ_MASK;
                    s <= X"07";
                end if;
            when X"07" => --wait for end of IRQ_EN write transaction
                master_start <= '0';
                if master_done = '1' then
                    s <= X"04"; --read irq register again
                end if;
            ---------------------------------------------------------------------
            when X"08" => --read FIIC_MR register
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '0';
                    master_addr  <= FIIC_MR;
                    s <= X"09";
                end if;
            when X"09" => --check MODE bit
                master_start <= '0';
                if master_done = '1' then
                    fiic_mr_reg <= master_rdata; --store value
                    if (master_rdata(0) = '1') then
                        --MODE=ACE, change to NON-ACE
                        s <= X"0A";
                    else
                        s <= X"0C"; --go on
                    end if;
                end if;
            when X"0A" => --start FIIC_MR write transaction
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '1';
                    master_addr  <= FIIC_MR;
                    master_wdata <= X"00000000";
                    s <= X"0B";
                end if;
            when X"0B" => --wait for end of write transaction
                master_start <= '0';
                if master_done = '1' then
                    s <= X"08"; --read FIIC_MR register again
                end if;
            ---------------------------------------------------------------------------
            when X"0C" => --get FIIC IRQ mask
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '0';
                    master_addr  <= FIIC_MSSIRQEN_0;
                    s <= X"0D";
                end if;
            when X"0D" => --check IRQ enable bit
                master_start <= '0';
                if master_done = '1' then
                    fiic_mssirqen_0_reg <= master_rdata; --store value
                    if (master_rdata(I2C_IRQ_BIT) = '0') then
                        --irq masked: enable
                        s <= X"0E";
                    else
                        s <= X"10"; --go on
                    end if;
                end if;
            when X"0E" => --start FIIC IRQEN write transaction
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '1';
                    master_addr  <= FIIC_MSSIRQEN_0;
                    master_wdata <= X"00024000"; --enable both i2c
                    s <= X"0F";
                end if;
            when X"0F" => --wait for end of write transaction
                master_start <= '0';
                if master_done = '1' then
                    s <= X"0C"; --read register again
                end if;
            --------------------------------------------------------------------------
            when X"10" => --set I2C control register (clock divider)
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '1';
                    master_addr  <= I2C_CTRL;
                    master_wdata(7 downto 0) <= I2C_CTRL_AA or I2C_CTRL_ENA or I2C_CTRL_DIV;
                    s <= X"11";
                end if;
            when X"11" => --wait complete
                master_start <= '0';
                if master_done = '1' then
                    s <= X"12"; --go on
                end if;
            when X"12" => --read I2C control register
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '0';
                    master_addr  <= I2C_CTRL;
                    s <= X"13";
                end if;
            when X"13" => --store data
                master_start <= '0';
                if master_done = '1' then
                    i2c_ctrl_reg <= master_rdata(7 downto 0);
                    s <= X"14"; --go on
                end if;
            ------------------------------------------------------------------------------
             --read status and IRQ
             when X"14" => --read Fabric Interrupt Controller status
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '0';
                    master_addr  <= FIIC_MSSIRQSRC_0;
                    s <= X"15";
                end if;
            when X"15" => --store data
                master_start <= '0';
                if master_done = '1' then
                    fiic_mssirqsrc_0_reg <= master_rdata;
                    s <= X"16"; --go on
                end if;
            when X"16" => --request I2C STATUS
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '0';
                    master_addr  <= I2C_STATUS;
                    s <= X"17";
                end if;
            when X"17" => --store I2C_STATUS
                master_start <= '0';
                if master_done = '1' then
                    i2c_status_reg <= master_rdata(7 downto 0);
                    if i2c_tstatus = "11" then --fist read
                        i2c_tstatus <= "00"; --go idle
                        s <= X"18"; --go to wait-for-start state
                    else --i2c_tstatus = "10" running
                        --go to ISR state
                        s <= X"FF";
                    end if;
                end if;

            ----------------------------------------------------------------------------------
            when X"18" => --IDLE state
                if master_busy = '0' and start = '1' then
                    i2c_tstatus <= "10"; --set transaction running (irq check enabled)
                    i2c_send_stop <= X"00";
                    --enable i2c core and send start
                    master_start <= '1';
                    master_write <= '1';
                    master_addr  <= I2C_CTRL;
                    master_wdata(7 downto 0) <= I2C_CTRL_STA or I2C_CTRL_ENA or I2C_CTRL_DIV;
                    i2c_nbytes <= i2c_tot_bytes;
                    i2c_tx_sr  <= i2c_tx_data;
                    s <= X"19"; --wait write
                else
                    --stay in this state and wait for START
                    i2c_tstatus(1) <= '0';
                end if;

            ---------------------------------------------------------------------------------
            ---------------------------------------------------------------------------------
             when X"FF" => --ISR
                case i2c_status_reg is
                when X"08" => --START sent
                    --reset STA, write ADDR+W in DATA register, reset IRQ
                    if master_busy = '0' then
                        master_start <= '1';
                        master_write <= '1';
                        master_addr  <= I2C_DATA;
                        master_wdata(7 downto 0) <= i2c_slave_addr & I2C_WRITE; --SLAW
                        i2c_ack_rx <= I2C_CTRL_AA; --don't care
                        s <= X"1C"; --write CTRL reg
                    end if;
                when X"18" => --SLAW sent, ACK received
                    --write i2c command/register, then reset IRQ
                    if master_busy = '0' then
                        master_start <= '1';
                        master_write <= '1';
                        master_addr  <= I2C_DATA;
                        master_wdata(7 downto 0) <= i2c_reg_addr;
                        i2c_ack_rx <= I2C_CTRL_AA; --don't care
                        s <= X"1C"; --write CTRL reg
                    end if;
                when X"28" => --DATA sent, ACK received
                    if master_busy = '0' then
                        if i2c_rnw = '1' then 
                            master_start <= '1';
                            master_write <= '1';
                            --if i2c_reg_addr has been sent, and a READ is requested, send a REPEATED START
                            master_addr  <= I2C_CTRL;
                            master_wdata(7 downto 0) <= I2C_CTRL_STA or I2C_CTRL_ENA or I2C_CTRL_DIV;
                            s <= X"19"; --wait for APB transaction to finish
                        else --write
                            if i2c_nbytes = "000" then
                                --over: send STOP
                                i2c_tstatus(0) <= '0'; --success
                                i2c_send_stop  <= I2C_CTRL_STO;
                                s <= X"1D"; --write CTRL register (reset IRQ and send STOP)
                            else --data to send
                                master_start <= '1';
                                master_write <= '1';
                                --write, send DATA
                                i2c_nbytes   <= i2c_nbytes - 1;
                                master_addr  <= I2C_DATA;
                                master_wdata(7 downto 0) <= i2c_tx_sr(7 downto 0);
                                i2c_tx_sr(23 downto 0) <= i2c_tx_sr(31 downto 8);
                                --s <= X"19"; --wait for APB transaction to finish
                                s <= X"1C";
                            end if; --i2c_nbytes
                        end if; --rnw
                    end if; --master_busy

                when X"20" | X"30" | X"48" => --SLAW/TXD/SLAR sent, NACK received
                    --ERROR: send STOP, reset IRQ
                    i2c_send_stop  <= I2C_CTRL_STO;
                    i2c_tstatus(0) <= '1'; --error
                    i2c_err_cnt    <= i2c_err_cnt+1;
                    s <= X"1D"; --wait for transaction done, then wait for IRQ

                when X"10" => --repeated start: performing a READ transaction
                    --write ADDR+W in DATA register, reset IRQ and STA
                    if master_busy = '0' then
                        master_start <= '1';
                        master_write <= '1';
                        master_addr  <= I2C_DATA;
                        master_wdata(7 downto 0) <= i2c_slave_addr & I2C_READ; --SLAR
                        i2c_ack_rx <= I2C_CTRL_AA; --don't care 
                        s <= X"1C"; --write CTRL reg
                    end if;
                when X"40" => --SLAR sent, ack received
                    --decide whether to send rx_ack or not
                    if i2c_nbytes = "001" then --single byte, send NACK
                        i2c_ack_rx <= X"00";
                    else --more data to receive, send ACK
                        i2c_ack_rx <= I2C_CTRL_AA;
                    end if;
                    i2c_send_stop <= X"00";
                    s <= X"1D"; --reset IRQ and set AA
                when X"50" | X"58" => --DATA received, ACK/NACK sent
                    --read data, then set AA and reset IRQ
                    if master_busy = '0' then
                        master_start <= '1';
                        master_write <= '0';
                        master_addr  <= I2C_DATA;
                        i2c_nbytes <= i2c_nbytes-1;
                        s <= X"1E";
                    end if;

                when others =>
                    --SEND STOP
                    i2c_err_cnt   <= i2c_err_cnt+1;
                    i2c_send_stop <= I2C_CTRL_STO;
                    i2c_rx_data   <= X"BAADBAAD";
                    s <= X"1D";
                end case;
            ----------------------------------------------------------------------------------------
            ----------------------------------------------------------------------------------------

            when X"19" => --wait for APB write transaction to finish
                master_start <= '0';
                if master_done = '1' then
                    if i2c_send_stop = X"00" then
                        s <= X"1A"; --go to check IRQ state
                    else
                        i2c_tstatus(1) <= '0'; --end transaction
                        s <= X"18";
                    end if;
                end if;
            when X"1A" => --check IRQ lines
                if i2c_tout = '1' then
                    i2c_err_cnt   <= i2c_err_cnt+1;
                    i2c_send_stop <= I2C_CTRL_STO;
                    i2c_rx_data   <= X"FFFFFFFF";
                    i2c_tstatus(0) <= '1'; --error
                    s <= X"1D";
                elsif master_busy = '0' and (mss_int_reg(0) = '1' or mss_int_reg(5) = '1') then
                --elsif master_busy = '0' and (mss_int_reg(I2C_MSSIRQ_BIT) = '1') then --TODO: use only correct bit
                    --read ctrl register (to check for IRQ)
                    master_start <= '1';
                    master_write <= '0';
                    master_addr  <= I2C_CTRL;
                    s <= X"1B"; --go to check IRQ bit state
                end if;
            when X"1B" => --check Interrupt bit in I2C_CTRL register
                master_start <= '0';
                if master_done = '1' then
                    i2c_ctrl_reg <= master_rdata(7 downto 0); --update register
                    if master_rdata(I2C_CTRL_SI_BIT) = '1' then
                        --interrupt set, update status reg and go on 
                        s <= X"16";
                    else --continue checking
                        s <= X"1A";
                    end if;
                end if;

            -------------------------------------------------------------------------------------
            when X"1C" => --wait for DATA register write (SLAW/SLAR/DATA), then reset IRQ and set AA flag
                master_start <= '0';
                if master_done = '1' then
                    --reset IRQ and STA bits
                    i2c_send_stop <= X"00";
                    s <= X"1D"; --write ctrl register
                end if;
            when X"1D" => --reset IRQ and set CTRL REGISTER
                if master_busy = '0' then
                    master_start <= '1';
                    master_write <= '1'; 
                    master_addr  <= I2C_CTRL;
                    master_wdata(7 downto 0) <= i2c_ack_rx or i2c_send_stop or I2C_CTRL_ENA or I2C_CTRL_DIV;
                    s <= X"19"; --wait for transaction done then check IRQ
                end if;
            when X"1E" => --wait for read data
                master_start <= '0';
                if master_done = '1' then
                    --shift data in (LSB is received first, so bytes will be reversed)
                    i2c_rx_data(31 downto 24) <= i2c_rx_data(23 downto 16);
                    i2c_rx_data(23 downto 16) <= i2c_rx_data(15 downto  8);
                    i2c_rx_data(15 downto  8) <= i2c_rx_data( 7 downto  0);
                    i2c_rx_data( 7 downto  0) <= master_rdata(7 downto 0);
                    if i2c_nbytes = "000" then --send STOP & complete
                        i2c_tstatus(0) <= '0'; --success
                        i2c_send_stop  <= I2C_CTRL_STO;
                    elsif i2c_nbytes = "001" then --last data, send NACK
                        i2c_send_stop <= X"00";
                        i2c_ack_rx    <= X"00";
                    else --more data to receive, send ACK
                        i2c_send_stop <= X"00";
                        i2c_ack_rx    <= I2C_CTRL_AA;
                    end if;
                    --then reset IRQ
                    s <= X"1D";
                end if;

            when others =>
                i2c_err_cnt   <= i2c_err_cnt+1;
                i2c_send_stop <= I2C_CTRL_STO;
                i2c_rx_data   <= X"FFFFFFFF";
                i2c_tstatus(0) <= '1'; --error
                s <= X"1D";
            end case;
        end if; --reset
    end if; --clock
end process; --MAIN

busy <= i2c_tstatus(1);


I2C_TOUT_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if m_PRESETn_reg = '0' or s_PRESETn_reg = '0' or i2c_slave_en = '0' then --v32: added i2c_slave_en
            i2c_timeout_c <= I2C_TIMEOUT;
            i2c_tout <= '0';
        else
            --counter
            if busy = '0' or i2c_timeout_c = 0 then
                i2c_timeout_c <= I2C_TIMEOUT;
            else
                i2c_timeout_c <= i2c_timeout_c-1;
            end if;

            --timeout flag
            if i2c_timeout_c = 0 then
                i2c_tout <= '1';
            else 
                i2c_tout <= '0';
            end if;
        end if;
    end if;
end process;

TIMERS : process(PCLK)
begin
    if rising_edge(PCLK) then
        if m_PRESETn_reg = '0' or s_PRESETn_reg = '0' or i2c_slave_en = '0' then --v32: i2c_slave_en
            wd_tmr  <= APP_UPDATE; --i2c watchdog timer
            wd_tout <= '0'; --i2c watchdog timeout
            p_tmr   <= 30*CLK_FREQ; --pause timer (first run: 30 seconds)
            p_tout  <= '0'; --pause timeout
        else
            if wd_tout = '0' then --counting
                if wd_tmr = 0 then --timer expired: reload
                    wd_tout <= '1';
                    wd_tmr  <= APP_UPDATE;
                else --downcount
                    wd_tout <= '0';
                    wd_tmr  <= wd_tmr-1;
                end if;
            else --timed out
                if wd_rst = '1' then --reset request
                    wd_tout <= '0';
                    wd_tmr  <= APP_UPDATE;
                end if;
            end if;

            if p_tout = '0' then --counting
                if p_tmr = 0 then --expired
                    p_tout <= '1';
                    p_tmr <= I2C_PAUSE;
                else --downcount
                    p_tout <= '0';
                    p_tmr <= p_tmr-1;
                end if;
            else --timeout
                if p_rst = '1' then --reset request
                    p_tout <= '0';
                    p_tmr <= I2C_PAUSE;
                end if;
            end if;
        end if;
    end if;
end process;

i2c_reg_addr   <= i2c_opcode(7 downto 0);
i2c_tot_bytes  <= unsigned(i2c_opcode(10 downto 8));
i2c_rnw        <= i2c_opcode(11);
i2c_slave_addr <= i2c_opcode(18 downto 12);



--0x00: X"00" & i2c_status & i2c_ctrl & FSM_S
--0x04: last 4 I2C states
--0x08: i2c_error_count
--0x0C: APB busy, APB error, i2c trans.status, APB wdata, APB addr

--following addresses are calculated according to Fan controller (0,1) and FAN (0,1,2,3) indexes
--N0 = number of utility registers (starting from 0). Currently = 4
--NCOM = number of common registers per fan controller = 3
--NFAN = number of registers per fan. Currently = 10
--NFC  = number of registers per fan controller = NCOM+4*NFAN = 43

--4*[(NFC*FC)+N0+0]: status (FC)
--4*[(NFC*FC)+N0+1]: mfr mode (FC)
--4*[(NFC*FC)+N0+2]: device lifetime in seconds (FC)

--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+0)] : page (FC,FAN) not used
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+1)] : fan cfg12 (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+2)] : fan mfr cfg (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+3)] : fan fault response (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+4)] : fan status (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+5)] : fan command (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+6)] : fan speed (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+7)] : fan run time (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+8)] : fan pwm (FC,FAN)
--4*[(NFC*FC)+(NFAN*FAN))+N0+NCOM+9)] : fan average pwm (FC,FAN)
APP_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if m_PRESETn_reg = '0' or s_PRESETn_reg = '0' or i2c_slave_en = '0' then --v32: i2c_slave_en
            as       <= X"00";
            web_n    <= '1';
            tick     <= "00"; --v1.1
            wd_rst   <= '1';
            p_rst    <= '1';
            start    <= '0';
            addrb    <= (others => '0');
            seq      <= 0;
            page     <= X"0";
            fc_id    <= 0;
            fifo_rd  <= '0';
            reset_command <= '0'; --v32 added
        else
            case as is 
            --first update utility RAM locations (debug/info)
            --v1.1: don't write anymore, but read data
            when X"00" =>
                if tick = "00" then --v1.1
                    --WRITE
                    --dinb <= X"00000000"; --NOT available for debug
                    tick <= "01"; --v1.1
                elsif tick = "01" then
                    --CHANGE ADDRESS
                    addrb <= "0000001"; --addrb+1; 
                    tick <= "10"; --v1.1
                else
                    --READ PREVIOUS
                    tick <= "00";
                    as <= as+1;
                end if;
            when X"01" =>
                if tick = "00" then
                    --WRITE
                    --dinb <= fifo_full & fifo_empty & std_logic_vector(i2c_err_cnt(29 downto 0)); --available for debug
                    tick <= "01";
                elsif tick = "01" then
                    --change address
                    tick <= "10";
                    addrb <= addrb+1;
                else
                    --READ PREVIOUS
                    set_rpm_front_top <= unsigned(doutb(15 downto 0));
                    tick <= "00";
                    as <= as+1;
                end if;
            when X"02" =>
                if tick = "00" then
                    --WRITE
                    --dinb <= std_logic_vector(fan_rst_cnt) & std_logic_vector(fifo_cnt);
                    tick <= "01";
                elsif tick = "01" then
                    --change address
                    tick <= "10";
                    addrb <= addrb+1;
                else
                    --READ PREVIOUS
                    set_rpm_front_bottom <= unsigned(doutb(15 downto 0));
                    tick <= "00";
                    as <= as+1;
                end if;
            when X"03" =>
                if tick = "00" then
                    --WRITE
                    --dinb <= fifo_dat;
                    tick <= "01";
                elsif tick = "01" then
                    --read address 3
                    tick <= "10";
                    addrb <= addrb+1;
                    seq   <= 0;
                    page  <= X"0";
                else
                    --READ PREVIOUS
                    set_rpm_back <= unsigned(doutb(15 downto 0));
                    tick <= "00";
                    as <= as+1;
                end if;
            -------------------------------------------------------------------
            --Read common I2C registers (seq=0:2)
            when X"04" => --start I2C transaction
                --if busy = '0' and i2c_slave_en = '1' and p_tout = '1' then 
                if busy = '0' and p_tout = '1' then --v32 changed
                    p_rst <= '1';
                    i2c_opcode  <= FAN_CTRL_ADDR(fc_id) & read_seq(seq); --Fan controller serial address & transaction parameters from READ_ROM
                    i2c_tx_data(7 downto 0) <= std_logic_vector(X"0" & page); --write only PAGE in this FSM section
                    start       <= '1';
                    as          <= as+1;
                else
                    p_rst <= '0'; --start pause timer
                end if;
            when X"05" => --wait for transaction to start
                if busy = '1' then
                    start <= '0';
                    as    <= as+1;
                end if;
            when X"06" => --wait for READ transaction to finish, then write to RAM
                if busy = '0' then
                    if web_n = '1' and i2c_rnw = '1' then --do only for read transaction
                        web_n <= '0'; --assert RAM write enable
                        --compose data according to transaction length
                        case i2c_tot_bytes is
                        when "001" =>
                            dinb <= X"000000" & i2c_rx_b0;
                        when "010" =>
                            dinb <= X"0000" & i2c_rx_b0 & i2c_rx_b1;
                        when "100" =>
                            dinb <= i2c_rx_b0 & i2c_rx_b1 & i2c_rx_b2 & i2c_rx_b3;
                        when others =>
                            dinb <= X"DEAD0001";
                        end case;
                    else --web_n = '0' or i2c_rnw = '0'
                        web_n <= '1';
                        as <= as+1;
                    end if;
                end if;
            when X"07" => --go on
                --end of transaction
                if seq = RSEQ_LAST then --read table completed 
                    if page = X"3" then --no more pages to read
                        seq     <= 0;
                        page    <= X"0";

                        if fc_id = 1 then --both controllers done: go to write section
                            fc_id <= 0;
                            as    <= as+1;
                            addrb <= (others => '0');
                        else --read from controller 2
                            fc_id <= 1;
                            addrb <= addrb+1;
                            as    <= X"04";
                        end if;
                    else --next page
                        seq   <= 3; --PAGE WRITE
                        page  <= page+1;
                        addrb <= addrb+1;
                        as    <= X"04";
                    end if;
                else --table not completed: process next entry
                    --page does not change
                    seq   <= seq+1;
                    addrb <= addrb+1;
                    as    <= X"04";
                end if;
            -------------------------------------------------------------------
            when X"08" =>
                --if busy = '0' and i2c_slave_en = '1' and p_tout = '1' then 
                if busy = '0' and p_tout = '1' then --v32 changed
                    p_rst <= '1';
                    --                          I2C_ADDR                       RNW   BYTES&REG_ADDR
                    i2c_opcode               <= write_seq(seq)(34 downto 28) & '0' & write_seq(seq)(10 downto 0);
                    i2c_tx_data(15 downto 0) <= write_seq(seq)(27 downto 12);

                    if write_seq(seq)(11) = '1' then --entry enabled
                        start <= '1';
                        as    <= as+1;
                    else
                        as <= X"0B";
                    end if;
                else
                    p_rst <= '0'; --start pause timer
                end if;
            when X"09" => --wait for transaction to start
                if busy = '1' then
                    start <= '0';
                    as    <= as+1;
                end if;
            when X"0A" => --wait for WRITE transaction to finish
                if busy = '0' then
                    as <= as+1;
                end if;
            when X"0B" => --go on
                --end of transaction
                if seq = WSEQ_LAST then --write table completed 
                    if page = X"3" then --no more pages to write
                        seq     <= 0;
                        page    <= X"0";

                        if fc_id = 1 then --both controllers done: finish
                            fc_id <= 0;
                            as    <= as+1;
                            addrb <= "1111111";
                        else --write to controller 2
                            fc_id <= 1;
                            as    <= X"08";
                        end if;
                    else --next page
                        seq   <= 0; 
                        page  <= page+1;
                        as    <= X"08";
                    end if;
                else --table not completed: process next entry
                    --page does not change
                    seq   <= seq+1;
                    as    <= X"08";
                end if;
            -------------------------------------------------------------------
            when X"0C" => --custom commands
                if fifo_empty = '1' then
                    as <= X"17"; --last state
                else
                    fifo_rd <= '1';
                    as <= as+1;
                end if;
            when X"0D" =>
                --wait for FIFO data
                fifo_rd <= '0';
                as <= as+1;
            when X"0E" =>
                if fifo_dout(31 downto 24) = X"43" then --command
                    i2c_opcode <= fifo_dout(18 downto 0);
                    if fifo_dout(11) = '0' then --WRITE
                        as <= as+1; --read another value from FIFO 
                    else --READ
                        addrb <= "1111110"; --write always in location 0x7E (PLB 0x1F8)
                        as <= X"14";
                    end if;
                elsif fifo_dout(31 downto 24) = X"FF" then --reset
                    reset_command <= '1'; --v32 added
                    as <= X"17"; --end state
                else --bad command
                    as <= X"17"; --end state
                end if;
            --WRITE----------------------------------------------------------------------
            when X"0F" => --wait data from FIFO
                --Wait until new data or timeout
                wd_rst <= '0'; --start timer
                if fifo_empty = '0' then
                    --get next data
                    fifo_rd <= '1';
                    wd_rst  <= '1';
                    as <= as+1;
                elsif wd_tout = '1' then
                    wd_rst <= '1';
                    addrb <= (others => '0');
                    as <= X"18"; --default state
                end if;
            when X"10" => 
                --wait for FIFO data
                fifo_rd <= '0';
                as <= as+1;
            when X"11" =>
                --start transaction
                i2c_tx_data <= fifo_dout;
                --if busy = '0' and i2c_slave_en = '1' and p_tout = '1' then 
                if busy = '0' and  p_tout = '1' then --v32 changed
                    p_rst <= '1';
                    start <= '1';
                    as <= as+1;
                else
                    p_rst <= '0'; --start pause timer
                end if;
            when X"12" => --wait for transaction to start
                if busy = '1' then
                    start <= '0';
                    as    <= as+1;
                end if;
            when X"13" => --wait for WRITE transaction to finish
                if busy = '0' then
                    as <= X"0C"; --check FIFO again
                end if;
            --READ-----------------------------------------------------------------------
            when X"14" => 
                --if busy = '0' and i2c_slave_en = '1' and p_tout = '1' then 
                if busy = '0' and p_tout = '1' then --v32 changed
                    p_rst <= '1';
                    start       <= '1';
                    as          <= as+1;
                else
                    p_rst <= '0'; --start pause timer
                end if;
            when X"15" => --wait for transaction to start
                if busy = '1' then
                    start <= '0';
                    as    <= as+1;
                end if;
            when X"16" => --wait for READ transaction to finish, then write to RAM
                if busy = '0' then
                    if web_n = '1' then --do only for read transaction
                        web_n <= '0'; --assert RAM write enable
                        --compose data according to transaction length
                        case i2c_tot_bytes is
                        when "001" =>
                            dinb <= X"000000" & i2c_rx_b0;
                        when "010" =>
                            dinb <= X"0000" & i2c_rx_b0 & i2c_rx_b1;
                        when "100" =>
                            dinb <= i2c_rx_b0 & i2c_rx_b1 & i2c_rx_b2 & i2c_rx_b3;
                        when others =>
                            dinb <= X"DEAD0002";
                        end case;
                    else --web_n = '0' or i2c_rnw = '0'
                        web_n <= '1';
                        as <= X"0C"; --check FIFO again
                    end if;
                end if;

            when X"17" => --go on
                --PAUSE state
                wd_rst <= '0'; --start timer
                if wd_tout = '1' then
                    --wd_rst <= '1';
                    --addrb  <= (others => '0');
                    --as <= X"00"; 
                    as <= as+1; --pass through default state to reset cleanly
                end if;
            when others => 
                as       <= X"00";
                web_n    <= '1';
                wd_rst   <= '1';
                p_rst    <= '1';
                start    <= '0';
                addrb    <= (others => '0');
                seq      <= 0;
                page     <= X"0";
                fc_id    <= 0;
                fifo_rd  <= '0';
                reset_command <= '0'; --v32 added
            end case;
        end if; --rst
    end if; --clk
end process; --APP_P
       
--write to FIFO when PLB writes on address 0x1FC (RAM location 0x7F)
fifo_wr <= '1' when wea_n = '0' and addra = "1111111"  and fifo_full = '0' else '0';

--CMD_FIFO : fifo8x32b
CMD_FIFO: myfifo
generic map(
    LOG_DEPTH => 3
)
port map( 
    DATA   => dina, --data from regbank
    Q      => fifo_dout,
    WE     => fifo_wr,
    RE     => fifo_rd,
    --WCLOCK => PCLK,
    --RCLOCK => PCLK,
    CLOCK  => PCLK,
    FULL   => fifo_full,
    EMPTY  => fifo_empty,
    RESET  => fifo_rst
);
fifo_rst <= not s_PRESETn_reg or not i2c_slave_en;

--DEBUG_P : process(PCLK)
--begin
--    if rising_edge(PCLK) then
--        if s_PRESETn_reg = '0' then
--            fan_rst_cnt <= X"0000";
--            fifo_cnt    <= X"0000";
--            fifo_dat    <= X"DEADF00D";
--        else
--            if as = X"0E" then
--                fifo_cnt <= fifo_cnt+1;
--                fifo_dat <= fifo_dout;
--            end if;
--
--            if reset_command_r = '1' then
--                fan_rst_cnt <= fan_rst_cnt+1;
--            end if;
--        end if;
--    end if;
--end process;

---------------------------------------------------------------------------------------------------

--              DATA              ENA   NBYTES  REG_ADDR   
--write_seq(0) <= std_logic_vector(
--                 X"000" & page) & '1' & "001" & FAN_PAGE;    --select page
--write_seq(1) <= (X"0050")       & '0' & "001" & FAN_CFG12;   --disable fan (needed is configuration is changed)
--write_seq(2) <= (X"E302")       & '0' & "010" & FAN_MFR_CFG; --set configuration (if needed)
--write_seq(3) <= fan_speed_rpm   & '1' & "010" & FAN_CMD;     --set fan speed
--write_seq(4) <= (X"00D0")       & '0' & "001" & FAN_CFG12;   --re-enable fan (if previously disabled)

--     |---------------------------------------|
--     |  FAN11  |  FAN12  |  FAN13  |  FAN14  |
--     |  RPM 1  |  RPM 1  |  RPM 3  |  RPM 3  |
--FRONT|---------------------------------------|BACK
--     |  FAN21  |  FAN22  |  FAN23  |  FAN24  |
--     |  RPM 2  |  RPM 2  |  RPM 3  |  RPM 3  |
--     |---------------------------------------|
-----------------(34:28)------------(27:12)------------------------(11)--(10:8)--(7:0)---------------------------
-----------------I2C_ADDR-----------VALUE--------------------------ENA---BYTES---REG_ADDR------------------------
write_seq(0)  <= FAN_CTRL_ADDR(0) & X"0000"                      & '1' & "001" & FAN_PAGE; --select page0 (fan11)
write_seq(1)  <= FAN_CTRL_ADDR(0) & std_logic_vector(fan_speed1) & '1' & "010" & FAN_CMD;  --set fan speed
write_seq(2)  <= FAN_CTRL_ADDR(0) & X"0001"                      & '1' & "001" & FAN_PAGE; --select page1 (fan22)
write_seq(3)  <= FAN_CTRL_ADDR(0) & std_logic_vector(fan_speed2) & '1' & "010" & FAN_CMD;  --set fan speed
write_seq(4)  <= FAN_CTRL_ADDR(0) & X"0002"                      & '1' & "001" & FAN_PAGE; --select page2 (fan13)
write_seq(5)  <= FAN_CTRL_ADDR(0) & std_logic_vector(fan_speed3) & '1' & "010" & FAN_CMD;  --set fan speed
write_seq(6)  <= FAN_CTRL_ADDR(0) & X"0003"                      & '1' & "001" & FAN_PAGE; --select page3 (fan24)
write_seq(7)  <= FAN_CTRL_ADDR(0) & std_logic_vector(fan_speed3) & '1' & "010" & FAN_CMD;  --set fan speed
write_seq(8)  <= FAN_CTRL_ADDR(1) & X"0000"                      & '1' & "001" & FAN_PAGE; --select page0 (fan21)
write_seq(9)  <= FAN_CTRL_ADDR(1) & std_logic_vector(fan_speed2) & '1' & "010" & FAN_CMD;  --set fan speed
write_seq(10) <= FAN_CTRL_ADDR(1) & X"0001"                      & '1' & "001" & FAN_PAGE; --select page1 (fan12)
write_seq(11) <= FAN_CTRL_ADDR(1) & std_logic_vector(fan_speed1) & '1' & "010" & FAN_CMD;  --set fan speed
write_seq(12) <= FAN_CTRL_ADDR(1) & X"0002"                      & '1' & "001" & FAN_PAGE; --select page2 (fan23)
write_seq(13) <= FAN_CTRL_ADDR(1) & std_logic_vector(fan_speed3) & '1' & "010" & FAN_CMD;  --set fan speed
write_seq(14) <= FAN_CTRL_ADDR(1) & X"0003"                      & '1' & "001" & FAN_PAGE; --select page3 (fan14)
write_seq(15) <= FAN_CTRL_ADDR(1) & std_logic_vector(fan_speed3) & '1' & "010" & FAN_CMD;  --set fan speed


--start with predefined speeds
--if fan_speedup_en = '1', then increase speed by SPEEDUP_STEP every SPEEDUP_INT seconds
FAN_SPEED_CTRL_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if m_PRESETn_reg = '0' or s_PRESETn_reg = '0' or fan_reset = '1' or i2c_slave_en = '0' then --v32 i2c_slave_en
            fan_speed1 <= (others => '0'); --to_unsigned(FAN_RPM_FRONT_TOP,16); --X"1388"; --5000 rpm
            fan_speed2 <= (others => '0'); --to_unsigned(FAN_RPM_FRONT_BOTTOM,16); --X"2328"; --9000 rpm
            fan_speed3 <= (others => '0'); --to_unsigned(FAN_RPM_BACK,16); --X"0BB8"; --3000 rpm
            sec_cnt    <= 0;
            --fan_step_tmr <= X"00";
        else
            --seconds counter
            if sec_cnt = 0 then
                sec_cnt <= CLK_FREQ;
            else
                sec_cnt <= sec_cnt-1;
            end if;

            --v1.1
            fan_speed1 <= set_rpm_front_top; 
            fan_speed2 <= set_rpm_front_bottom;
            fan_speed3 <= set_rpm_back;

            --v1.1 disabled speedup (not used anyway)
            --fan speed update counter
            --if fan_step_tmr = X"00" then
            --    --timeout: reload timer and increment speed (if needed).
            --    if fan_speedup_en = '1' then
            --        fan_speed1 <= fan_speed1 + unsigned(fan_speedup_step);
            --        fan_speed2 <= fan_speed2 + unsigned(fan_speedup_step);
            --        fan_speed3 <= fan_speed3 + unsigned(fan_speedup_step);
            --    end if;
            --    fan_step_tmr <= unsigned(fan_speedup_int);
            --else
            --    fan_step_tmr <= fan_step_tmr-1;
            --end if;
        end if;
    end if;
end process;

end architecture rtl; --of FAN_controller



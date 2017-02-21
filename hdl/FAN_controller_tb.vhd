
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library USER;
use USER.APB3_regbank_pkg.all;

entity FAN_controller_tb is
end entity FAN_controller_tb;

architecture test of FAN_controller_tb is

component FAN_Controller is
generic(
    CLK_FREQ : natural := 50000000;
    I2C_CORE_BASE : std_logic_vector(31 downto 0) := X"40050020"; --BASE + (CHAN<<5)
    TMP112_I2C_ADDR_0 : std_logic_vector(6 downto 0) --7bit I2C address of TMP112 sensor
);
port (
    PCLK    : in  std_logic;
    core_i2c_irq : in std_logic;
    --APB3 SLAVE (register interface)
    s_PRESETn : in  std_logic;
    s_PSELx   : in  std_logic;
    s_PENABLE : in  std_logic;
    s_PWRITE  : in  std_logic;
    s_PADDR   : in  std_logic_vector(3+2-1 downto 0); --byte addressing, 32bit registers
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
end component FAN_Controller;

signal clk, rst_n, irq : std_logic;
signal o_psel, o_pena, o_wr, i_ready, i_err : std_logic;
signal o_addr, o_wdata, i_rdata : std_logic_vector(31 downto 0);

signal regs_in  : slv32_array(0 to 3);
signal regs_out : slv32_array(0 to 3);

signal ctrl_w, data_w, ctrl_r, data_r, stat_r, stat_rr : std_logic_vector(7 downto 0);
signal w, w_r, r, r_psel, re_psel, force_irq : std_logic := '0';
signal rx_timer : natural;

--I2C controller status encoding
constant ST_IDLE    : std_logic_vector(7 downto 0) := X"F8"; --start has been sent
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


begin

CLKGEN : clk  <= '1' after 10 ns when clk = '0' else 
                '0' after 10 ns;
RSTGEN : rst_n <= '0', '1' after 1234 ns;                

UUT : FAN_Controller
generic map(
    CLK_FREQ          => 50000000,
    I2C_CORE_BASE     => X"40050020",
    TMP112_I2C_ADDR_0 => "0000000" --not used (yet)
)
port map(
    PCLK         => clk,
    core_i2c_irq => irq,
    --APB3 SLAVE (register interface)
    s_PRESETn => rst_n,
    s_PSELx   => '0',
    s_PENABLE => '0',
    s_PWRITE  => '0',
    s_PADDR   => "00000",
    s_PWDATA  => X"00000000",
    s_PRDATA  => open,
    s_PREADY  => open,
    s_PSLVERR => open,
    --APB3 MASTER (I2C access)
    m_PRESETn => '1',
    m_PSELx   => o_psel,
    m_PENABLE => o_pena,
    m_PWRITE  => o_wr,
    m_PADDR   => o_addr,
    m_PWDATA  => o_wdata,
    m_PRDATA  => i_rdata,
    m_PREADY  => i_ready,
    m_PSLVERR => i_err
);

--slave register bank (mimic I2C core controller slave interface)
I2C_CC_SLAVE : APB3_Regbank
generic map(
    ADDR_W => 2 --number of 32 bit registers is 2**3=4
)
port map(
    --APB3
    PCLK    => clk,
    PRESETn => rst_n,
    PSEL    => o_psel,
    PENABLE => o_pena,
    PWRITE  => o_wr,
    PADDR   => o_addr(3 downto 0), --address bytes (ADDR_W+2)
    PWDATA  => o_wdata,
    PRDATA  => i_rdata,
    PREADY  => i_ready,
    PSLVERR => i_err,
    --Register bank
    rd_en_o     => open,
    regbank_in  => regs_in,
    regbank_out => regs_out
);
--written <= REG-REG(o_psel and o_wr)
ctrl_w <= regs_out(0)(7 downto 0);
data_w <= regs_out(2)(7 downto 0);

regs_in(0) <= X"000000" & ctrl_r;
regs_in(1) <= X"000000" & stat_r;
regs_in(2) <= X"000000" & data_r;

irq <= ctrl_r(3);

re_psel <= o_psel and not r_psel;

DUMMY : process(clk)
begin
    if rising_edge(clk) then
        if rst_n = '0' then
            stat_r <= X"F8";
            data_r <= X"01";
            w_r    <= '0';
            rx_timer <= 0;
        else
            r_psel <= o_psel;
            w      <= re_psel and o_wr;
            r      <= re_psel and not o_wr;
            w_r    <= w and not w_r;
            if w_r = '1' then --register written
                --update local register (irq bit controlled separatela)
                ctrl_r(7 downto 4) <= ctrl_w(7 downto 4); --update local register
                ctrl_r(2 downto 0) <= ctrl_w(2 downto 0); --update local register
            end if;

            stat_rr <= stat_r;
            if stat_rr /= stat_r or force_irq = '1' then
               ctrl_r(3) <= '1'; --set irq when status changes
            elsif w_r = '1' and ctrl_w(3) = '0' then --reset irq when requested
               ctrl_r(3) <= '0'; --set irq when status changes
            end if;

            case stat_r is
            when ST_IDLE =>
                if ctrl_r(5) = '1' then --start
                    stat_r <= ST_START;
                end if;
            when ST_START | ST_ARB_LST =>
                if w_r = '1' then
                    --always ack
                    if data_w(0) = '0' then --write
                        stat_r <= ST_SLAW_A;
                    end if;
                --elsif ctrl_r(5) = '0' then <<============
                --    stat_r <= ST_IDLE;
                end if;

            when ST_RESTART => --wait for data
                if w_r = '1' and o_addr(3 downto 0) = X"8" then
                    --always ack
                    if data_w(0) = '1' then --read
                        stat_r <= ST_SLAR_A;
                    else --write
                        stat_r <= ST_SLAW_A;
                    end if;
                end if;
            when ST_SLAR_A =>
                if rx_timer < 32 then --simulate pause before 1st data
                    rx_timer <= rx_timer+1;
                elsif ctrl_r(3) = '0' then --irq has been reset
                    rx_timer <= 0;
                    if ctrl_r(2) = '0' then
                        stat_r <= ST_RXD_NA;
                    else
                        stat_r <= ST_RXD_A;
                    end if;
                end if;
            when ST_SLAW_A | ST_TXD_A =>
                if w_r = '1' then
                    if ctrl_w(5) = '1' and o_addr(3 downto 0) = X"0" then --repeated start
                        stat_r <= ST_RESTART; 
                    elsif ctrl_w(4) = '1' and o_addr(3 downto 0) = X"0" then --stop
                        stat_r <= ST_STOP;
                    elsif o_addr(3 downto 0) = X"8" then --wdata
                        stat_r <= ST_TXD_A; --always ack
                    end if;
                end if;
            when ST_RXD_A =>
                --first wait until data is read, then wait some time, then take action 
                force_irq <= '0';
                if rx_timer = 0 then
                    if r = '1' and o_addr(3 downto 2) = "10" then --read data register
                        rx_timer <= 1;
                    end if;
                elsif rx_timer < 32 then
                    rx_timer <= rx_timer+1;
                else --rx_timer = 32
                    data_r <= std_logic_vector(unsigned(data_r)+1);
                    rx_timer <= 0;
                    if ctrl_r(2) = '0' then
                        stat_r <= ST_RXD_NA;
                    else
                        stat_r <= ST_RXD_A; --need to trigger IRQ
                        force_irq <= '1';
                    end if;
                end if;
            when ST_RXD_NA =>
                if w_r = '1' then
                    if ctrl_w(5) = '1' and o_addr(3 downto 0) = X"0" then --repeated start
                        stat_r <= ST_START; 
                    elsif ctrl_w(4) = '1' and o_addr(3 downto 0) = X"0" then --stop
                        stat_r <= ST_STOP;
                    end if;
                end if;
            when ST_STOP =>
                ctrl_r(4) <= '0'; --reset stop flag
                if ctrl_r(5) = '1' then --start
                    data_r <= std_logic_vector(unsigned(data_r)+1);
                    stat_r <= ST_START;
                end if;
            when others =>
            end case;
        end if;
    end if;
end process;

end architecture test;

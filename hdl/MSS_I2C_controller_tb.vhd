library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.APB3_regbank_pkg.all;

entity MSS_I2C_controller_tb is
end entity MSS_I2C_controller_tb;

architecture t of MSS_I2C_controller_tb is

component MSS_I2C_controller is
generic(
    CLK_FREQ : natural := 50000000
);
port (
    PCLK    : in  std_logic;
    --
    fan_reset        : in std_logic; --when 1, fan speeds are reset to default values
    fan_speedup_en   : in std_logic; --when 1, fan speed is increased by RPM_STEP every STEP_INTERVAL seconds
    fan_speedup_step : in std_logic_vector(15 downto 0);
    fan_speedup_int  : in std_logic_vector( 7 downto 0);

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
end component MSS_I2C_controller;


signal clk, rst_n : std_logic;
signal m_psel, m_pena, m_pwrite, m_pready, m_perr : std_logic;
signal m_paddr, m_pwdata, m_prdata : std_logic_vector(31 downto 0);
--signal s_psel, s_pena, s_pwrite, s_pready, s_perr : std_logic;
--signal s_paddr, s_pwdata, s_prdata : std_logic_vector(31 downto 0);
signal regbank_in, regbank_out : slv32_array(0 to 2**4-1);
signal mss_int : std_logic_vector(7 downto 0);

signal step_cmd, s_psel, s_pena, s_pwrite, s_pready : std_logic := '0';
signal s_pwdata : std_logic_vector(31 downto 0);
signal k : natural := 0;

begin

clk   <= '1' after 10 ns when clk = '0' else
       '0' after 10 ns;
rst_n <= '0', '1' after 456 ns;

mss_int <= X"00";

process(clk)
begin
    if rising_edge(clk) then
        case k is
        when 0 =>
            if step_cmd = '1' then
                s_psel   <= '1';
                s_pwrite <= '1';
                s_pwdata <= X"80000000";
                k <= k+1;
            end if;
        when 1 =>
            s_pena <= '1';
            k <= k+1;
        when 2 =>
            if s_pready = '1' then
                s_pena <= '0';
                s_psel <= '0';
                k <= k+1;
            end if;
        when 3 =>
            s_psel   <= '1';
            s_pwrite <= '1';
            s_pwdata <= X"80000001";
            k <= k+1;
        when 4 =>
            s_pena <= '1';
            k <= k+1;
        when 5 =>
            if s_pready = '1' then
                s_pena <= '0';
                s_psel <= '0';
                k <= k+1;
            end if;
        when 6 =>
            s_psel   <= '1';
            s_pwrite <= '1';
            s_pwdata <= X"80000000";
            k <= k+1;
        when 7 =>
            s_pena <= '1';
            k <= k+1;
        when 8 =>
            if s_pready = '1' then
                s_pena <= '0';
                s_psel <= '0';
                k <= 0;
            end if;
        when others => 
            k <= 0;
        end case;
    end if;
end process;


UUT : MSS_I2C_controller
port map(
    --
    PCLK    => clk,
    fan_reset        => '0',
    fan_speedup_en   => '0',
    fan_speedup_step => X"0000",
    fan_speedup_int  => X"00",
    i2c_slave_en1_n => '0',
    i2c_slave_en2_n => '1',
    mss_int => mss_int,
    --APB3 SLAVE (register interface)
    s_PRESETn => rst_n,
    s_PSELx   => s_psel,
    s_PENABLE => s_pena,
    s_PWRITE  => s_pwrite,
    s_PADDR   => X"00000000",
    s_PWDATA  => s_pwdata,
    s_PRDATA  => open,
    s_PREADY  => s_pready,
    s_PSLVERR => open,
    --APB3 MASTER (I2C access)
    m_PRESETn => rst_n,
    m_PSELx   => m_psel,
    m_PENABLE => m_pena,
    m_PWRITE  => m_pwrite,
    m_PADDR   => m_paddr,
    m_PWDATA  => m_pwdata,
    m_PRDATA  => m_prdata,
    m_PREADY  => m_pready,
    m_PSLVERR => m_perr
);

DUMMY : APB3_Regbank
generic map(
    ADDR_W => 4
)
port map(
    --APB3
    PCLK    => clk,
    PRESETn => rst_n,
    PSEL    => m_psel,
    PENABLE => m_pena,
    PWRITE  => m_pwrite,
    PADDR   => m_paddr(5 downto 0),
    PWDATA  => m_pwdata,
    PRDATA  => m_prdata,
    PREADY  => m_pready,
    PSLVERR => m_perr,
    --Register bank
    rd_en_o     => open,
    wr_en_o     => open,
    regbank_in  => regbank_in,
    regbank_out => regbank_out
);
 
regbank_in(0)  <= X"00024000"; --00 --IRQEN+MODE_REG+CTRL_REG
regbank_in(1)  <= X"000000F8"; --04 --STATUS
regbank_in(2)  <= X"33333333"; --08
regbank_in(3)  <= X"44444444"; --0C
regbank_in(4)  <= X"55555555"; --10
regbank_in(5)  <= X"66666666"; --14
regbank_in(6)  <= X"77777777"; --18
regbank_in(7)  <= X"88888888"; --1C
regbank_in(8)  <= X"00000000"; --20 --IRQ
regbank_in(9)  <= X"AAAAAAAA"; --24
regbank_in(10) <= X"BBBBBBBB"; --28
regbank_in(11) <= X"CCCCCCCC"; --2C
regbank_in(12) <= X"DDDDDDDD"; --30
regbank_in(13) <= X"EEEEEEEE"; --34
regbank_in(14) <= X"FFFFFFFF"; --38
regbank_in(15) <= X"12345678"; --3C

end architecture;

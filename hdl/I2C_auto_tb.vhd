library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity i2c_auto_tb is
end entity;

architecture test of i2c_auto_tb is
 
component I2C_auto is
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
    --GPIO
    gpi_data : out std_logic_vector(15 downto 0); 
    gpo_data : in  std_logic_vector(15 downto 0);
    --i2c
    o_i2c_scl_tx : out std_logic_vector(1 downto 0);
    i_i2c_scl_rx : in  std_logic_vector(1 downto 0);
    o_i2c_sda_tx : out std_logic_vector(1 downto 0);
    i_i2c_sda_rx : in  std_logic_vector(1 downto 0)
);
end component I2C_auto;

signal clk, rst_n, psel, pena, pwrite, pready, continue, flag : std_logic;
signal o_scl, i_scl, o_sda, i_sda : std_logic_vector(1 downto 0);
signal paddr, pwdata, prdata : std_logic_vector(31 downto 0);
signal tbs, timer, id : natural;
signal data_cnt : unsigned(11 downto 0);

type slv32 is array (0 to 7) of std_logic_vector(31 downto 0);
signal avec : slv32 := (
    X"00000084", --0
    X"00000080", --1
    X"00000084", --2
    X"00000080", --3
    X"00000084", --4
    X"00000080", --5
    X"00000084", --6
    X"00000080"  --7
);
signal dvec : slv32 := (
    X"00000020", --0
    X"00000010", --1
    X"AABBCCDD", --2
    X"00000009", --3
    X"99887766", --4
    X"00000008", --5
    X"00000000", --6
    X"00000010"  --7
);

begin

clk <= '0' after 10 ns when clk = '1' else
       '1' after 10 ns;

rst_n <= '0', '1' after 1 us;

i_scl <= "11";
i_sda <= "11";
o_scl <= "HH";
o_sda <= "HH";


uut : I2C_auto
port map(
    PCLK    => clk,
    --APB3 SLAVE (register interface)
    s_PRESETn => rst_n,
    s_PSELx   => psel,
    s_PENABLE => pena,
    s_PWRITE  => pwrite,
    s_PADDR   => paddr,
    s_PWDATA  => pwdata,
    s_PRDATA  => prdata,
    s_PREADY  => pready,
    s_PSLVERR => open,
    --GPIO
    gpi_data => open,
    gpo_data => X"ABCD",
    --i2c
    o_i2c_scl_tx => o_scl,
    i_i2c_scl_rx => i_scl,
    o_i2c_sda_tx => o_sda,
    i_i2c_sda_rx => i_sda
);

CTRL : process(clk)
begin
    if rst_n = '0' then
        tbs    <= 6;
        psel   <= '0';
        pena   <= '0';
        pwrite <= '0';
        continue <= '0';
        paddr  <= X"00000000";
        pwdata <= X"00000000";
        data_cnt <= X"111";
        timer <= 0;
        id <= 0;
        flag <= '0';
    elsif rising_edge(clk) then
        case tbs is
        when 0 =>
            psel   <= '1';
            pwrite <= '1';
            paddr  <= avec(id);
            pwdata <= dvec(id);
            tbs <= tbs+1;
        when 1 =>
            pena <= '1';
            tbs <= tbs+1;
        when 2 =>
            if pready = '1' then
                psel <= '0';
                pena <= '0';
                paddr <= X"FFFFFFFF";
                pwdata <= X"FFFFFFFF";
                tbs <= tbs+1;
            end if;
        when 3 =>

            if flag = '0' then
                tbs <= 0;
            elsif id < 8 then
                tbs <= tbs+1;
            else 
                tbs <= tbs+2;
            end if;

            id <= id+1;
            flag <= not flag;
        when 4 => --pause
            if timer < 5000 then
                timer <= timer+1;
            else 
                timer <= 0;
                tbs <= 0;
            end if;
        when 5 => --suspend
            id <= 0;
            if continue = '1' then
                tbs <= 0;
            end if;
        when others =>
            tbs <= 0;
        end case;
    end if;
end process;


end architecture;


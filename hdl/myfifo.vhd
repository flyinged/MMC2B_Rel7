---------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity myfifo is
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
end entity myfifo;

architecture rtl of myfifo is

signal rp, wp, dcnt : unsigned(LOG_DEPTH-1 downto 0);
signal f, e, write, read : std_logic;

constant ZERO : unsigned(LOG_DEPTH-1 downto 0) := (others => '0');
constant ONES : unsigned(LOG_DEPTH-1 downto 0) := (others => '1');

type mem_t is array (0 to (2**LOG_DEPTH)-1) of std_logic_vector(31 downto 0);
signal mem : mem_t;

begin

write <= WE and not f;
read  <= RE and not e;

e <= '1' when dcnt = ZERO else '0';
f <= '1' when dcnt = ONES else '0';

FULL <= f;
EMPTY <= e;

DCNT_P : process(CLOCK)
begin
    if rising_edge(CLOCK) then
        if (RESET = '1') then
            dcnt <= ZERO;
        else
            if write = '1' and read = '0' then
                dcnt <= dcnt+1;
            elsif write = '0' and read = '1' then
                dcnt <= dcnt-1;
            end if;
        end if;
    end if;
end process;
   

WRITE_P : process(CLOCK)
begin
    if rising_edge(CLOCK) then
        if (RESET = '1') then
            wp <= ZERO;
        else
            if write = '1' then
                mem(to_integer(wp)) <= DATA;
                wp <= wp+1;
            end if;
        end if;
    end if;
end process;


READ_P : process(CLOCK)
begin
    if rising_edge(CLOCK) then
        if (RESET = '1') then
            rp <= ZERO;
        else
            if read = '1' then
                Q <= mem(to_integer(rp));
                rp <= rp+1;
            end if;
        end if;
    end if;
end process;


end architecture rtl;


-----------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity myfifo_tb is
end entity myfifo_tb;

architecture test of myfifo_tb is

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

signal dout : std_logic_vector(31 downto 0);
signal din: unsigned(31 downto 0);
signal we,re,clk,rst,full,empty :std_logic;
signal s : natural;

begin

rst <= '1', '0' after 1111 ns;
clk <= '1' after 5 ns when clk = '0' else
       '0' after 5 ns;

STIM_P:process(clk)
begin
    if rising_edge(clk) then
        if rst = '1' then
            s <= 42;
        else
            case s is
            when 0 =>
                report "Filling FIFO until half full" severity note;
                s <= s+1;
            when 1 =>
                if din < X"00000004" then
                    if we = '0' then
                        we <= '1';
                    else
                        we <= '0';
                        din <= din+1;
                    end if;
                else
                    s <= s+1;
                end if;
            when 2 =>
                report "Write+read while half full" severity note;
                s <= s+1;
            when 3 =>
                if din < X"00000008" then
                    if we = '0' then
                        we <= '1';
                    else
                        we <= '0';
                        din <= din+1;
                    end if;

                    if re = '0' then
                        re <= '1';
                    else 
                        re <= '0';
                    end if;
                else
                    we <= '0';
                    re <= '0';
                    s <= s+1;
                end if;
            when 4 =>
                report "Read until empty" severity note;
                s <= s+1;
            when 5 =>
                if empty = '0' then
                    if re = '0' then
                        re <= '1';
                    else 
                        re <= '0';
                    end if;
                else
                    s <= s+1;
                end if;
            when 6 =>
                report "Write+read while empty" severity note;
                s <= s+1;
            when 7 =>
                if din < X"0000000C" then
                    if we = '0' then
                        we <= '1';
                    else
                        we <= '0';
                        din <= din+1;
                    end if;

                    if re = '0' then
                        re <= '1';
                    else 
                        re <= '0';
                    end if;
                else
                    we <= '0';
                    re <= '0';
                    s <= s+1;
                end if;
            when 8 =>
                report "Write until full" severity note;
                s <= s+1;
            when 9 =>
                if full = '0' then
                    if we = '0' then
                        we <= '1';
                    else
                        we <= '0';
                        din <= din+1;
                    end if;
                else
                    s <= s+1;
                end if;
            when 10 => 
                report "Write and read simultaneously when full" severity note;
                s <= s+1;
            when 11 =>
                if din < X"00000028" then
                    if we = '0' then
                        we <= '1';
                    else
                        we <= '0';
                        din <= din+1;
                    end if;

                    if re = '0' then
                        re <= '1';
                    else 
                        re <= '0';
                    end if;
                else
                    we <= '0';
                    re <= '0';
                    s <= s+1;
                end if;

            when others =>
                report "Test cycle over" severity warning;
                din <= X"00000001";
                we <= '0';
                re <= '0';
                s <= 0;
            end case;
        end if;
    end if;
end process;

FIFO: myfifo
generic map(
    LOG_DEPTH => 3
)
port map( 
    DATA   => std_logic_vector(din),
    Q      => dout,
    WE     => we,
    RE     => re,
    CLOCK  => clk,
    FULL   => full,
    EMPTY  => empty, 
    RESET  => rst
);


end architecture test;

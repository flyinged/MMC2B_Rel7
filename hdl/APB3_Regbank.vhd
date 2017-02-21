--------------------------------------------------------------------------------
-- Company: PSI
--
-- File: APB3_Regbank.vhd
-- File history:
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--
-- Description: 
--
-- Register bank interfaced with the APB3 bus. 
-- Can be used together with custom logic to allow command & control through the MSS.
--
-- Targeted device: <Family::SmartFusion> <Die::A2F500M3G> <Package::256 FBGA>
-- Author: Alessandro Malatesta
--
--------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;

package APB3_regbank_pkg is

    type slv32_array is array (natural range <>) of std_logic_vector(31 downto 0);

    component APB3_Regbank_ram is
    generic(
        DEBUG : std_logic := '0'; --address 0 direct
        ALLOW_WRITE : std_logic := '0';
        ADDR_W : natural := 4; --DWORD address
        DEBUG_ADDR : std_logic_vector(31 downto 0) := (others => '0') --BYTE address
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

    component APB3_Regbank is
    generic(
        ADDR_W : natural := 4 --DWORD ADDRESS
    );
    port (
        --APB3
        PCLK    : in  std_logic;
        PRESETn : in  std_logic;
        PSEL    : in  std_logic;
        PENABLE : in  std_logic;
        PWRITE  : in  std_logic;
        PADDR   : in  std_logic_vector(ADDR_W+2-1 downto 0); --BYTE ADDRESS
        PWDATA  : in  std_logic_vector(31 downto 0);
        PRDATA  : out std_logic_vector(31 downto 0);
        PREADY  : out std_logic;
        PSLVERR : out std_logic;
        --Register bank
        rd_en_o     : out std_logic;
        wr_en_o     : out std_logic;
        regbank_in  : in  slv32_array(0 to 2**ADDR_W-1);
        regbank_out : out slv32_array(0 to 2**ADDR_W-1)
    );
    end component APB3_Regbank;

    
end package APB3_regbank_pkg;

--------------------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--library USER;
use work.APB3_regbank_pkg.all;

library synplify;
use synplify.attributes.all;

entity APB3_Regbank is
generic(
    ADDR_W : natural := 4 --number of registers is 2^ADDR_W
);
port (
    --APB3
    PCLK    : in  std_logic;
    PRESETn : in  std_logic;
    PSEL    : in  std_logic;
    PENABLE : in  std_logic;
    PWRITE  : in  std_logic;
    PADDR   : in  std_logic_vector(ADDR_W+2-1 downto 0);-- +2 for byte addressing
    PWDATA  : in  std_logic_vector(31 downto 0);
    PRDATA  : out std_logic_vector(31 downto 0);
    PREADY  : out std_logic;
    PSLVERR : out std_logic;
    --Register bank
    rd_en_o     : out std_logic;
    wr_en_o     : out std_logic;
    regbank_in  : in  slv32_array(0 to 2**ADDR_W-1);
    regbank_out : out slv32_array(0 to 2**ADDR_W-1)
);
end entity APB3_Regbank;

architecture rtl of APB3_Regbank is

--attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of rtl : architecture is "tmr";

signal s : std_logic_vector(1 downto 0);
signal rd_en, wr_en, PRESETn_r : std_logic;
signal id : natural range 0 to (2**ADDR_W-1); --regbank index
signal regbank_out_s : slv32_array(0 to 2**ADDR_W-1) := (others => (others => '0'));

begin

PSLVERR <= '0';

FSM_P: process(PCLK)

begin

    if rising_edge(PCLK) then
        PRESETn_r <= PRESETn;
        if PRESETn_r = '0' then
            PREADY <= '1';
            rd_en <= '0';
            wr_en <= '0';
            s <= "00";
        else
            case s is
            when "00" =>
                if (PSEL = '1') then
                    s <= "01";
                    if PWRITE = '1' then
                        rd_en  <= '0';
                        wr_en  <= '1';
                        PREADY <= '1';
                    else
                        rd_en  <= '1';
                        wr_en  <= '0';
                        PREADY <= '0';
                    end if; --PWRITE
                end if; --PSEL
            when "01" =>
                if PWRITE = '1' then
                    rd_en  <= '0';
                    wr_en  <= '0';
                    PREADY <= '1';
                    s <= "00";
                else
                    rd_en  <= '0';
                    wr_en  <= '0';
                    PREADY <= '0';
                    s <= "10";                    
                end if;
            when others =>
                PREADY <= '1';
                s <= "00";
            end case;
        end if;
    end if;

end process; --FSM_P

--BYTE ADDRESSING (convert address to index. Use only ADDR_W bits)
id <= to_integer(unsigned(PADDR(ADDR_W+2-1 downto 2)));

DATA_RW : process(PCLK)
begin
    if rising_edge(PCLK) then
        if wr_en = '1' then
            regbank_out_s(id) <= PWDATA;
        end if;

        wr_en_o <= wr_en;
        PRDATA <= regbank_in(id);
    end if;
end process; --DATA_RW

rd_en_o <= rd_en;
regbank_out <= regbank_out_s;

end architecture rtl;


--------------------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--library USER;
use work.APB3_regbank_pkg.all;

library synplify;
use synplify.attributes.all;

entity APB3_Regbank_ram is
generic(
    DEBUG : std_logic := '0'; --address 0 direct
    ALLOW_WRITE : std_logic := '0';
    ADDR_W : natural := 4; --DWORD address
    DEBUG_ADDR : std_logic_vector(31 downto 0) := (others => '0') --BYTE address
);
port (
    --when DEBUG = '1', reading address 0x0 returns always DBG_DATA
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
    --APB RAM port
    we_n : out std_logic;
    addr : out  std_logic_vector(ADDR_W-1 downto 0); --DWORD address
    wdata : out std_logic_vector(31 downto 0);
    rdata  : in  std_logic_vector(31 downto 0)
);
end entity APB3_Regbank_ram;

architecture rtl of APB3_Regbank_ram is

--attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of rtl : architecture is "tmr";

signal s : std_logic_vector(1 downto 0);
signal rd_en, wr_en, PRESETn_r : std_logic;
--signal id : natural;

--constant ZERO : std_logic_vector(ADDR_W-1 downto 0) := (others => '0');

begin

PSLVERR <= '0';

FSM_P: process(PCLK)

begin

    if rising_edge(PCLK) then
        PRESETn_r <= PRESETn;
        if PRESETn_r = '0' then
            rd_en <= '0';
            wr_en <= '0';
            PREADY <= '1';
            s <= "00";
        else
            case s is
            when "00" =>
                if (PSEL = '1') then
                    s <= "01";
                    if PWRITE = '1' then
                        rd_en  <= '0';
                        wr_en  <= '1';
                        PREADY <= '1';
                    else
                        rd_en  <= '1';
                        wr_en  <= '0';
                        PREADY <= '0';
                    end if; --PWRITE
                end if; --PSEL
            when "01" =>
                if PWRITE = '1' then
                    rd_en  <= '0';
                    wr_en  <= '0';
                    PREADY <= '1';
                    s <= "00";
                else
                    rd_en  <= '0';
                    wr_en  <= '0';
                    PREADY <= '0';
                    s <= "10";                    
                end if;
            when "10" => --added wait state to let PSEL go down
                PREADY <= '1';
                s <= "11";
            when others =>
                PREADY <= '1';
                s <= "00";
            end case;
        end if;
    end if;

end process; --FSM_P

--BYTE ADDRESSING
addr <= PADDR(PADDR'left downto 2);
we_n <= not wr_en when ALLOW_WRITE = '1'
        else '1';
wdata <= PWDATA;

DATA_RW : process(PCLK)
begin
    if rising_edge(PCLK) then
        if DEBUG = '1' and  (PADDR(PADDR'left downto 2) = DEBUG_ADDR(PADDR'left downto 2)) then --ZERO then
            PRDATA <= DBG_DATA;
        else
            PRDATA <= rdata;
        end if;
    end if;
end process; --DATA_RW

end architecture rtl;


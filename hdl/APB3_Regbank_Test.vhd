--------------------------------------------------------------------------------
-- Company: <Name>
--
-- File: APB3_Regbank_Test.vhd
-- File history:
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--
-- Description: 
--
-- <Description here>
--
-- Targeted device: <Family::SmartFusion> <Die::A2F500M3G> <Package::256 FBGA>
-- Author: <Name>
--
--------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

use work.APB3_regbank_pkg.all;

entity APB3_Regbank_Test is
end APB3_Regbank_Test;

architecture struct of APB3_Regbank_Test is

component smartfusion_ram_64x32 is
port( 
    dina  : in    std_logic_vector(31 downto 0);
    douta : out   std_logic_vector(31 downto 0);
    dinb  : in    std_logic_vector(31 downto 0);
    doutb : out   std_logic_vector(31 downto 0);
    addra : in    std_logic_vector(5 downto 0);
    addrb : in    std_logic_vector(5 downto 0);
    wea_n : in    std_logic;
    web_n : in    std_logic;
    ena   : in    std_logic;
    enb   : in    std_logic;
    clka  : in    std_logic;
    clkb  : in    std_logic
);
end component smartfusion_ram_64x32;

constant ADDR_W : natural := 6; --3=64registers
constant NREG : natural := 2**ADDR_W;
signal clk, rstn, psel, pen, prdy, perr, pwr, wea_n, web_n : std_logic;
signal paddr : unsigned(ADDR_W+1 downto 0);
signal addra : std_logic_vector(ADDR_W-1 downto 0);
signal addrb : unsigned(ADDR_W-1 downto 0);
signal pwdata, prdata, dina, dinb, douta, doutb, apb_rdata, ram_rdata : std_logic_vector(31 downto 0);

signal s,k : natural;

begin

clk <= '1' after 10 ns when clk = '0' else
       '0' after 10 ns;
rstn <= '0', '1' after 1 us;

UUT : APB3_Regbank_ram
generic map(
    ADDR_W => ADDR_W 
)
port map(
    --APB3
    PCLK    => clk,
    PRESETn => rstn,
    PSEL    => psel, 
    PENABLE => pen,
    PWRITE  => pwr,
    PADDR   => std_logic_vector(paddr),
    PWDATA  => pwdata,
    PRDATA  => prdata,
    PREADY  => prdy,
    PSLVERR => perr,
    --Register bank
    we_n => wea_n,
    addr => addra,
    rdata => douta, 
    wdata => dina 
);

RAM : smartfusion_ram_64x32
port map( 
    dina  => dina,
    douta => douta,
    dinb  => dinb,
    doutb => doutb,
    addra => addra,
    addrb => std_logic_vector(addrb),
    wea_n => wea_n,
    web_n => web_n,
    ena   => '1',
    enb   => '1',
    clka  => clk,
    clkb  => clk
);

                
dinb <= std_logic_vector("00"&addrb&"00"&addrb&"00"&addrb&"00"&addrb);
                        
pwdata <= std_logic_vector(paddr&paddr&paddr&paddr);

TEST_P : process(clk, rstn)
begin
    if rstn = '0' then
        s <= 0;
        k <= 0;
        --APB bus
        psel <= '0';
        pen <= '0';
        pwr <= '0';
        paddr <= (others => '1');
        --ram port b
        addrb <= (others => '0');
        web_n <= '1';
    elsif rising_edge(clk) then
        case s is
        when 0 =>
            --first init ram content
            if addrb < "111111" then
                if web_n = '1' then
                    web_n <= '0';
                else
                    web_n <= '1';
                    addrb <= addrb+1;
                end if;
            else
                addrb <= (others => '0');
                paddr <= (others => '0');
                web_n <= '1';
                s <= 1;
            end if;
        when 1 =>
            --then read all data from APB
            if paddr < X"FC" then
                case k is
                when 0 =>
                    if prdy = '1' then
                        psel <= '1';
                        pwr <= '0';
                        k <= 1;
                    end if;
                when 1 =>
                    pen <= '1';
                    k <= 2;
                when 2 =>
                    if prdy = '1' then
                        apb_rdata <= prdata;
                        psel <= '0';
                        pen <= '0';
                        paddr <= paddr+X"04";
                        k <= 0;
                    end if;
                when others =>
                    report "Wrong state" severity failure;
                end case;
            else
                paddr <= (others => '0');
                s <= 2;
            end if;
        when 2 =>
            --then write data from APB
            if paddr < X"FC" then
                case k is
                when 0 =>
                    if prdy = '1' then
                        psel <= '1';
                        pwr <= '1';
                        k <= 1;
                    end if;
                when 1 =>
                    pen <= '1';
                    k <= 2;
                when 2 =>
                    if prdy = '1' then
                        psel <= '0';
                        pen <= '0';
                        paddr <= paddr+X"04";
                        k <= 0;
                    end if;
                when others =>
                    report "Wrong state" severity failure;
                end case;
            else
                paddr <= (others => '0');
                s <= 3;
            end if;
        when 3 =>
            --read all data from RAM port B
            if addrb < "111111" then
                if k = 0 then
                    k <= 1;
                elsif k = 1 then
                    k <= 2;
                else
                    ram_rdata <= doutb;
                    addrb <= addrb+1;
                    k <= 0;
                end if;
            else
                s <= 4;
            end if;
        when 4 =>
            report "End of simulation" severity failure;
        when others =>
            report "Wrong state" severity failure;
        end case;
    end if;
end process;

end architecture struct;

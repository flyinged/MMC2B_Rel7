library IEEE;
use IEEE.std_logic_1164.all;
use work.APB3_regbank_pkg.all;

entity vio is
    port (
        --APB3
        PCLK    : in  std_logic;
        PRESETn : in  std_logic;
        PSEL    : in  std_logic;
        PENABLE : in  std_logic;
        PWRITE  : in  std_logic;
        PADDR   : in  std_logic_vector(2+2-1 downto 0); --BYTE ADDRESS
        PWDATA  : in  std_logic_vector(31 downto 0);
        PRDATA  : out std_logic_vector(31 downto 0);
        PREADY  : out std_logic;
        PSLVERR : out std_logic;
        --Register bank
        read0 : in std_logic_vector(31 downto 0);
        read1 : in std_logic_vector(31 downto 0);
        read2 : in std_logic_vector(31 downto 0);
        read3 : in std_logic_vector(31 downto 0);

        write0 : out std_logic_vector(31 downto 0);
        write1 : out std_logic_vector(31 downto 0);
        write2 : out std_logic_vector(31 downto 0);
        write3 : out std_logic_vector(31 downto 0)
    );
end entity vio;

architecture rtl of vio is

constant ADDR_W : natural := 2;
signal regbank_in  : slv32_array(0 to 2**ADDR_W-1);
signal regbank_out : slv32_array(0 to 2**ADDR_W-1);

begin

    TOP : APB3_Regbank
    generic map(
        ADDR_W => ADDR_W
    )
    port map(
        --APB3
        PCLK    => PCLK,
        PRESETn => PRESETn,
        PSEL    => PSEL,
        PENABLE => PENABLE,
        PWRITE  => PWRITE,
        PADDR   => PADDR,
        PWDATA  => PWDATA,
        PRDATA  => PRDATA,
        PREADY  => PREADY,
        PSLVERR => PSLVERR,
        --Register bank
        rd_en_o => open,
        wr_en_o => open,
        regbank_in  => regbank_in,
        regbank_out => regbank_out
    );

regbank_in(0) <= read0;
regbank_in(1) <= read1;
regbank_in(2) <= read2;
regbank_in(3) <= read3;

write0 <= regbank_out(0);
write1 <= regbank_out(1);
write2 <= regbank_out(2);
write3 <= regbank_out(3);

end architecture rtl; --of vio

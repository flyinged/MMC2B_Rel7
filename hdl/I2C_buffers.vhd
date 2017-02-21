--------------------------------------------------------------------------------
-- Company: PSI
--
-- File: I2C_buffers.vhd
-- File history:
--      1.0: 10 Jun 2015: First version
--
-- Description: 
--
-- Implements buffers for I2C core
--
-- Targeted device: <Family::SmartFusion> <Die::A2F500M3G> <Package::256 FBGA>
-- Author: Alessandro Malatesta
--
--------------------------------------------------------------------------------

library IEEE;
library SMARTFUSION;

use IEEE.std_logic_1164.all;
use SMARTFUSION.COMPONENTS.all;

entity I2C_buffers is
generic(
    N_CHANNELS : natural := 1
);
port (
    --internal signals (from/to I2C core)
    SCLI : out   std_logic_vector(N_CHANNELS-1 downto 0);
    SDAI : out   std_logic_vector(N_CHANNELS-1 downto 0);
    SCLO : in    std_logic_vector(N_CHANNELS-1 downto 0);
    SDAO : in    std_logic_vector(N_CHANNELS-1 downto 0);
    --fins
    SDA  : inout std_logic_vector(N_CHANNELS-1 downto 0);
    SCL  : inout std_logic_vector(N_CHANNELS-1 downto 0)
);
end I2C_buffers;

architecture structural of I2C_buffers is

signal scl_tx_n : std_logic_vector(N_CHANNELS-1 downto 0);
signal sda_tx_n : std_logic_vector(N_CHANNELS-1 downto 0);

begin

BUFGEN : for c in 0 to (N_CHANNELS-1) generate

    --invert driver because driving '1' outputs '0', and driving '0' outputs 'Z' (pulled up to '1')
    scl_tx_n(c) <= not SCLO(c);
    sda_tx_n(c) <= not SDAO(c);

    SCL_BIBUF_i : BIBUF_LVCMOS33
    port map (
        D             => '0',
        E             => scl_tx_n(c),
        Y             => SCLI(c),
        PAD           => SCL(c)
    );

    SDA_BIBUF_i : BIBUF_LVCMOS33
    port map (
        D             => '0',
        E             => sda_tx_n(c),
        Y             => SDAI(c),
        PAD           => SDA(c)
    );

    ----drivers (only pull down)
    --scl(c) <= '0' when SCLO(c) = '0' else 'Z';
    --sda(c) <= '0' when SDAO(c) = '0' else 'Z';
    ----receivers (direct connection)
    --SCLI(c) <= scl(c);
    --SDAI(c) <= sda(c);
end generate;

end architecture structural;

--------------------------------------------------------------------------------
-- Company: PSI
--
-- File: mbu_mmc_v1.vhd
--
-- Description: 
--
-- Targeted device: <Family::SmartFusion> <Die::A2F500M3F> <Package::FFG256>
-- Author: Waldemar Koprek, Alessandro Malatesta
--
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- mbu_mmc_v1
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
library SMARTFUSION;
use SMARTFUSION.COMPONENTS.all;

library IEEE;
use IEEE.std_logic_1164.all;

library synplify;
use synplify.attributes.all;

entity mbu_mmc_v1 is
port(
  --debug controls
  i_v1_power_mask : in std_logic; --mask monitoring of v1_power_good. when 1, power good is forced to 1
  i_v2_power_mask : in std_logic; --mask monitoring of v2_power_good. when 1, power good is forced to 1
  i_fan_present_mask : in std_logic; --mask monitoring of fan_present_b. when 1, fan_present_b is forced to 0
  --testpoints
  o_v1_power_good_tp : out std_logic;  
  o_v2_power_good_tp : out std_logic;
  o_fan_present_b_tp : out std_logic;
  o_fp_on_tp         : out std_logic;

  ------------------------------------------------------------
  -- LOCAL INTERFACES
  ------------------------------------------------------------
  -- clocks
  i_clk                     : in    std_logic;
  --
  i_watchdog                : in    std_logic;
  o_hsc5_alert                : out   std_logic;

  -- AC/DC control
  i_use_supply2               : in    std_logic; 
  i_v1_power_good             : in    std_logic; 
  i_v2_power_good             : in    std_logic; 
  o_v1_remote_enable          : out   std_logic;
  o_v2_remote_enable          : out   std_logic;
  o_v1_sw_off                 : out   std_logic; 
  o_v2_sw_off                 : out   std_logic; 

  -- POWER ALARMS
  i_pwr_p5v0_alert_b          : in    std_logic;
  i_pwr_p3v3_alert_b          : in    std_logic;
  i_pwr_p12v_alert_b          : in    std_logic;
  i_pwr_p12v2_alert_b         : in    std_logic;
  i_pwr_p12v1_alert_b         : in    std_logic;
  i_pwr_n12v_alert_b          : in    std_logic;
  i_pwr_p12v2_ft_b            : in    std_logic;
  i_pwr_p12v1_ft_b            : in    std_logic;

  -- POWER METER
  i_pm_alert_b                : in    std_logic;

  -- FAN
  i_fan_present_b             : in    std_logic;
  i_fan_alert_b               : in    std_logic;
  o_fan_hw_reset_u1           : out   std_logic;
  o_fan_hw_reset_u2           : out   std_logic;
  o_fan_fault1                : out   std_logic;
  o_fan_fault2                : out   std_logic;

  -- TEMPERATURE
  i_tmp_alert_b               : in    std_logic;
  i_tmp_ht1_alert_b           : in    std_logic;
  i_tmp_ht2_alert_b           : in    std_logic;
  o_tmp_ht_pd_b               : out   std_logic;

  -- FRONT PANEL
  i_fp_on                     : in    std_logic;
  o_fp_fw_red                 : out   std_logic;
  o_fp_fw_green               : out   std_logic;
  o_fp_hw_red                 : out   std_logic;
  o_fp_hw_green               : out   std_logic;
  o_fp_remote_on_led          : out   std_logic; 

  -- GPAC IF
  o_master_reset              : out   std_logic; 
  i_spe                       : in    std_logic;
  o_spe_b                     : out   std_logic 
);
end entity mbu_mmc_v1;

-------------------------------------------------------
architecture arch_mbu_mmc_v1 of mbu_mmc_v1 is

attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of arch_mbu_mmc_v1 : architecture is "tmr";

component TPS3106K33 is  
generic (
    CNT_WIDTH : natural;
    C_DELAY : natural --clock sycles
);
port (
    i_clk        : in  std_logic;
    i_mr_b       : in  std_logic;
    o_rstsense_b : out std_logic := '0'
);
end component TPS3106K33; 


signal s_hsc5_alert              : std_logic := '0';
signal s_v1_power_good           : std_logic := '0';
signal s_v2_power_good           : std_logic := '0';
signal s_o_rstsense1_b           : std_logic := '0';
signal s_o_rstsense2_b           : std_logic := '0';
signal s_mr1_b                   : std_logic := '0';
signal s_master_reset            : std_logic := '0';
signal s_fan_present_b           : std_logic := '0';
signal s_v1_sw_off_b             : std_logic := '0';
signal s_v2_sw_off_b             : std_logic := '0';
signal s_v1_remote_enable        : std_logic := '0';
signal s_v2_remote_enable        : std_logic := '0';

begin

--testpoints
o_fp_on_tp         <= i_fp_on;  
o_fan_present_b_tp <= s_fan_present_b;
o_v1_power_good_tp <= i_v1_power_good;
o_v2_power_good_tp <= i_v2_power_good;

--allow masking of some inputs for debug purposes
s_fan_present_b <= i_fan_present_b and (not i_fan_present_mask); --when mask = 1, force to 0, otherwise actual value
s_v1_power_good <= i_v1_power_good or i_v1_power_mask; --when mask = 1, then power good = 1, otherwise actual value
s_v2_power_good <= i_v2_power_good or i_v2_power_mask; --when mask = 1, then power good = 1, otherwise actual value

-- added "NOT" because the hsc5_alert buffer is inverting
prc_alert_and : process ( i_clk )
begin
    if rising_edge( i_clk ) then
      --ALERT whenever one of the signals/expressions is low
      s_hsc5_alert <= not (      i_pm_alert_b 
                             and i_fan_alert_b
                             and i_tmp_alert_b
                             and i_pwr_p5v0_alert_b
                             and i_pwr_p3v3_alert_b
                             and i_pwr_p12v_alert_b
                             and i_pwr_p12v2_alert_b
                             and i_pwr_p12v1_alert_b
                             and i_pwr_n12v_alert_b
                             and (i_pwr_p12v2_ft_b or not s_v1_sw_off_b) --change: mask alarm when oring is off
                             and (i_pwr_p12v1_ft_b or not s_v2_sw_off_b) --change: mask alarm when oring is off
                             and i_tmp_ht1_alert_b --inverted on board
                             and i_tmp_ht2_alert_b --inverted on board
                             and (s_v1_power_good or not s_v1_remote_enable) --powergood = not alert (or supply disabled)
                             and (s_v2_power_good or not s_v2_remote_enable)
                         );
    end if;
end process ;
  
------------------------------------------------------------------------------
-- Front Panel LEDs
------------------------------------------------------------------------------

--HW led RED when any alarm is active, green otherwise
o_fp_hw_red   <= s_hsc5_alert;
o_fp_hw_green <= not s_hsc5_alert;

--FW led GREEN when SPE is asserted, RED otherwise
o_fp_fw_red    <= not i_spe;
o_fp_fw_green  <= i_spe;

--ON switch led color (red when remote mode, green otherwise). 
o_fp_remote_on_led <= '0';

------------------------------------------------------------------------------
-- Master reset release when at least one power good is valid
s_master_reset <= s_v1_power_good nor s_v2_power_good;
o_master_reset  <= s_master_reset;

--------------------------------------------------------------------------------
-- Reset FAN when master reset is asserted
o_fan_hw_reset_u1    <= s_master_reset;
o_fan_hw_reset_u2    <= s_master_reset;
-- Fan fault (TBD)  
o_fan_fault1 <= '0';
o_fan_fault2 <= '0';

--------------------------------------------------------------------  
-- Power enable (when switch is on and fan is present)
s_mr1_b  <= i_fp_on and not(s_fan_present_b);

--delay signal rising edge
RE_delay_pwr_ena1: TPS3106K33 
generic map(
    CNT_WIDTH => 24,
    C_DELAY  =>  2600000 --FREQ*130/1000
)
port map (
    i_clk         => i_clk,
    i_mr_b        => s_mr1_b,
    o_rstsense_b  => s_o_rstsense1_b
); 

----delay signal rising edge  --changed 15/10/15
--RE_delay_pwr_ena2: TPS3106K33
    --generic map(
    --CNT_WIDTH => 24,
    --C_DELAY  =>  2600000 --FREQ*130/1000
--) 
--port map (
    --i_clk         => i_clk,
    --i_mr_b        => s_o_rstsense1_b,
    --o_rstsense_b  => s_o_rstsense2_b
--); 

--REALTIME OUTPUTS --------------------------

-- Enable/disable external SPE signal (when '1', SPE_i is always deasserted)
o_spe_b <= '0';

--HSC5 alarm
o_hsc5_alert <= s_hsc5_alert;

--Power enable signals
--change 15/10/15: enable only power supply 2 when i_use_supply2=1, enable only power supply 1 otherwise
s_v1_remote_enable <= s_o_rstsense1_b and not i_use_supply2;
s_v2_remote_enable <= s_o_rstsense1_b and i_use_supply2;

o_v1_remote_enable <= s_v1_remote_enable;
o_v2_remote_enable <= s_v2_remote_enable;

---------------Power switch-off signals (power enable delayed, then inverted)
--delay signal rising edge
RE_delay_sw_off1: TPS3106K33 
generic map(
    CNT_WIDTH => 24,
    C_DELAY  =>  2600000 --FREQ*130/1000
)
port map (
    i_clk         => i_clk,
    i_mr_b        => s_v1_remote_enable,
    o_rstsense_b  => s_v1_sw_off_b
); 

--delay signal rising edge
RE_delay_sw_off2: TPS3106K33
    generic map(
    CNT_WIDTH => 24,
    C_DELAY  =>  2600000 --FREQ*130/1000
) 
port map (
    i_clk         => i_clk,
    i_mr_b        => s_v2_remote_enable, 
    o_rstsense_b  => s_v2_sw_off_b
); 


o_v1_sw_off <= not s_v1_sw_off_b;
o_v2_sw_off <= not s_v2_sw_off_b;

--heater power down (not used)
o_tmp_ht_pd_b <= '1';

end arch_mbu_mmc_v1;


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- TPS3106K33
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
--use IEEE.std_logic_unsigned.all;
--use IEEE.std_logic_arith.all;

library synplify;
use synplify.attributes.all;

--The original component has an active low open-drain RESETn output
--Reset would be triggered automatically when Vcc (3.3V) goes under 2.941V.
--When the manual reset input (MRn) goes low, output reset goes low immediately.
--When the MRn input goes high, the output stays low for (min,typ,max)=(65,139,195 ms), then it goes high.
entity TPS3106K33 is
generic(
    CNT_WIDTH : natural;
    C_DELAY   : natural --clock cycles
);
port (
    i_clk                       : in  std_logic;
    i_mr_b                      : in  std_logic;
    o_rstsense_b                : out std_logic := '0'
);
end entity TPS3106K33;

architecture arch_TPS3106K33 of TPS3106K33 is

attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of arch_TPS3106K33 : architecture is "tmr";

  signal s_rstsense_b        : std_logic := '0';
  signal slv_cnt              : unsigned(CNT_WIDTH-1 downto 0) := (others => '0');
begin

-- counter counts clock cycles
prc_delay : process ( i_clk )
begin
    if rising_edge( i_clk ) then
        if i_mr_b = '0' then
            s_rstsense_b <= '0';
            slv_cnt       <= (others => '0');
        else
            if s_rstsense_b = '0' then
                slv_cnt <= slv_cnt + 1;
                if slv_cnt = TO_UNSIGNED(C_DELAY, CNT_WIDTH) then
                    s_rstsense_b <= '1';
                end if;
            end if;
        end if;
    end if;
end process ;
  
o_rstsense_b  <= s_rstsense_b;

end architecture arch_TPS3106K33;
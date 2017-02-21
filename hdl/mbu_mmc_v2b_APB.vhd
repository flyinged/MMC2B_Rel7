--------------------------------------------------------------------------------
-- Company: PSI
--
-- File: APB3_Regbank_Test.vhd
-- File history:
--      1.0 : 23/06/2015 : First release
--      1.1 : 12/08/2015 : Changes for MMC_V2 REV.B
--      1.2 : 02/12/2015 : Changes for new front panel (V2 rev.b)
--      1.3 : 15/03/2016 : Added force_shutdown control
--
-- Description: 
--
-- Original V1.0 logic interfaced to the MSS via APB bus
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

--REGISTER MAP
--0x0:  READ  : (15:0) Version
--      RD/WR : bits 31:16 (used by SW to store data between reboots)
--0x4:  READ  : same as write
--      WRITE : Inputs to logic (coming from i2c GPIO)
--                  (16) enable hsc5 normal operation (toggle when '0')
--                  (15) force_shutdown (added 15/10/15, read from EEPROM, defines which power supply to use)
--                  (14) use_supply2    (added 15/03/16, when 1, power supplies are never enabled)
--                  (13) etc_alert_b  
--                  (12) pm_alert_b  
--                  (11) fan_alert_b  
--                  (10) tmp_alert_b  
--                  (9)  tmp_ht2_alert_b  
--                  (8)  tmp_ht1_alert_b  
--                  (7)  pwr_p12v2_ft_b    
--                  (6)  pwr_p12v1_ft_b    
--                  (5)  pwr_n12v_alert_b  
--                  (4)  pwr_p12v_alert_b  
--                  (3)  pwr_p12v2_alert_b  
--                  (2)  pwr_p12v1_alert_b  
--                  (1)  pwr_p3v3_alert_b  
--                  (0)  pwr_p5v0_alert_b  
--0x8:  READ  : Outputs from logic (written to i2c GPIO)
--                  (30) tmp_ht_pd_b
--                  (29) v2_sw_off
--                  (28) v1_sw_off
--                  (27) v2_remote_enable
--                  (26) v1_remote_enable
--                  (25) hsc5_alert
--
--                  (21) v2_power_good
--                  (20) v1_power_good
--                  (19) watchdog
--                  (18) spe (in)
--                  (17) fan_present_b
--                  (16) fp_on
--
--                  (11) o_tmp_ht_pd_b
--                  (10) o_spe_b
--                  (9)  master_reset
--                  (8)  fan_fault2
--                  (7)  fan_fault1
--                  (6)  fan_hw_reset_u2
--                  (5)  fan_hw_reset_u1
--                  (4)  fp_remote_on_led
--                  (3)  fp_hw_led (green/red_n)
--                  (2)  oled_rst_n (hardwired to '1')
--                  (1)  fp_fw_led (green/red_n)
--                  (0)  not used
entity MBU_MMC_V2B_APB is
generic(
    CLK_FREQ : natural := 50000000
);
port (
    VERSION : in std_logic_vector(15 downto 0);
    --Real-time inputs
    i_clk           : in std_logic;
    i_watchdog      : in std_logic;
    i_v1_power_good : in std_logic; 
    i_v2_power_good : in std_logic; 
    i_fan_present_b : in std_logic;
    i_fp_on         : in std_logic;
    i_spe           : in std_logic;
    --Real-time outputs
    --o_spe_b            : out std_logic; 
    o_hsc5_alert       : out std_logic; 
    --o_tmp_ht_pd_b      : out std_logic;
    o_v1_remote_enable : out std_logic;
    o_v2_remote_enable : out std_logic;
    o_v1_sw_off        : out std_logic; 
    o_v2_sw_off        : out std_logic;
    --GPIO
    gpo_data : out std_logic_vector(15 downto 0);
    gpi_data : in std_logic_vector(15 downto 0);
    --APB3
    PCLK    : in  std_logic;
    PRESETn : in  std_logic;
    PSELx   : in  std_logic;
    PENABLE : in  std_logic;
    PWRITE  : in  std_logic;
    PADDR   : in  std_logic_vector(3+2-1 downto 0);
    PWDATA  : in  std_logic_vector(31 downto 0);
    PRDATA  : out std_logic_vector(31 downto 0);
    PREADY  : out std_logic;
    PSLVERR : out std_logic
);
end MBU_MMC_V2B_APB;

architecture struct of MBU_MMC_V2B_APB is

attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of struct: architecture is "tmr";

component mbu_mmc_v2b is
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
  i_clk                       : in    std_logic;
  --
  i_force_shutdown            : in    std_logic;
  i_watchdog                  : in    std_logic;
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
  o_fp_fw_led               : out   std_logic;
  o_fp_hw_led               : out   std_logic;
  o_fp_remote_on_led          : out   std_logic; 

  -- GPAC IF
  o_master_reset              : out   std_logic; 
  i_spe                       : in    std_logic;
  o_spe_b                     : out   std_logic 
);
end component mbu_mmc_v2b;

constant ADDR_W : natural := 3; --4 registers
constant NREG : natural := 2**ADDR_W;

signal fabric2cpu_reg : slv32_array(0 to NREG-1);
signal cpu2fabric_reg : slv32_array(0 to NREG-1);
signal logic_inputs, logic_outputs, control_reg : std_logic_vector(31 downto 0);

signal s_hsc5_alert        : std_logic;
signal s_v1_remote_enable  : std_logic;
signal s_v2_remote_enable  : std_logic;
signal s_v1_sw_off         : std_logic; 
signal s_v2_sw_off         : std_logic; 
signal s_tmp_ht_pd_b       : std_logic; 
signal s_spe_b             : std_logic; 
signal s_fp_fw_led       : std_logic;
signal s_fp_hw_led       : std_logic;
signal s_fp_remote_on_led  : std_logic;
signal s_master_reset      : std_logic;
signal s_fan_hw_reset_u1   : std_logic;
signal s_fan_hw_reset_u2   : std_logic;
signal s_fan_fault1        : std_logic;
signal s_fan_fault2        : std_logic;

signal s_pwr_p5v0_alert_b  : std_logic;
signal s_pwr_p3v3_alert_b  : std_logic;
signal s_pwr_p12v1_alert_b : std_logic;
signal s_pwr_p12v2_alert_b : std_logic;
signal s_pwr_p12v_alert_b  : std_logic;
signal s_pwr_n12v_alert_b  : std_logic;
signal s_pwr_p12v1_ft_b    : std_logic;
signal s_pwr_p12v2_ft_b    : std_logic;
signal s_tmp_ht1_alert_b   : std_logic;
signal s_tmp_ht2_alert_b   : std_logic;
signal s_tmp_alert_b       : std_logic;
signal s_fan_alert_b       : std_logic;
signal s_pm_alert_b        : std_logic;
signal s_etc_alert_b       : std_logic;

signal s_fan_hw_reset_u1_p : std_logic := '0';
signal s_fan_hw_reset_u2_p : std_logic := '0';
signal s_fan_hw_reset_u1_r : std_logic := '0';
signal s_fan_hw_reset_u2_r : std_logic := '0';
signal fan_rst_timeout1  : std_logic := '0';
signal fan_rst_timeout2  : std_logic := '0';
signal fan_rst_cnt1 : unsigned(27 downto 0) := (others => '0');
signal fan_rst_cnt2 : unsigned(27 downto 0) := (others => '0');

signal s_v1_power_fail  : std_logic := '0';
signal s_v2_power_fail  : std_logic := '0';
signal pwr_fail_cnt1 : unsigned(7 downto 0) := (others => '0');
signal pwr_fail_cnt2 : unsigned(7 downto 0) := (others => '0');
signal acdc_start_cnt1, acdc_start_cnt2 : natural := 0;
signal pwrgood_valid1, pwrgood_valid2 : std_logic := '0';

signal PRESETn_reg, apb_read, s_use_supply2 : std_logic;
signal s_force_shutdown : std_logic := '0';
signal s_hsc5_alert_en  : std_logic := '0';
signal s_hsc5_alert_out    : std_logic;
signal s_hsc5_toggle    : std_logic := '0';
signal s_hsc5_cnt : unsigned(7 downto 0) := X"00";

signal timer : unsigned(31 downto 0);
signal timeout : std_logic := '0';

begin

RST_REG_P : process(PCLK)
begin 
    if rising_edge(PCLK) then
        PRESETn_reg <= PRESETn;
    end if;
end process;

REGBANK : APB3_Regbank
generic map(
    ADDR_W => ADDR_W
)
port map(
    --APB3
    PCLK    => PCLK,
    PRESETn => PRESETn_reg,
    PSEL    => PSELx, 
    PENABLE => PENABLE,
    PWRITE  => PWRITE,
    PADDR   => PADDR,
    PWDATA  => PWDATA,
    PRDATA  => PRDATA,
    PREADY  => PREADY,
    PSLVERR => PSLVERR,
    --Register bank
    wr_en_o     => open,    
    rd_en_o     => apb_read,
    regbank_in  => fabric2cpu_reg,
    regbank_out => cpu2fabric_reg
);

--ASSIGN REGISTERS FROM/TO APB REGISTER BANK ****************
--Fabric 2 MSS (write data to APB)
fabric2cpu_reg(0) <= cpu2fabric_reg(0)(31 downto 16) & VERSION(15 downto 0);
fabric2cpu_reg(1) <= logic_inputs; --readback of signals from CPU to FPGA fabric
fabric2cpu_reg(2) <= logic_outputs; --actual signals from FPGA fabric to CPU
fabric2cpu_reg(3) <= cpu2fabric_reg(3);

--CPU to Fabric (get data from APB)
control_reg               <= (others => '0'); --cpu2fabric_reg(0); --debug only
logic_inputs(15)          <= '0'; --cpu2fabric_reg(1)(15); --force_shutdown (added 15.3.2016)
logic_inputs(14)          <= '0'; --cpu2fabric_reg(1)(14); --use_supply2
logic_inputs(13 downto 0) <= cpu2fabric_reg(1)(13 downto 0);

--DATA TO CPU *********************************************
--Application signals
logic_outputs(0)  <= '0'; --unconnected;
logic_outputs(1)  <= s_fp_fw_led;
logic_outputs(2)  <= '1'; --oled rst_n;
logic_outputs(3)  <= s_fp_hw_led;
logic_outputs(4)  <= '1';
logic_outputs(5)  <= s_fan_hw_reset_u1_p;
logic_outputs(6)  <= s_fan_hw_reset_u2_p;
logic_outputs(7)  <= '0'; --s_fan_fault1; 
logic_outputs(8)  <= '0'; --s_fan_fault2; 
logic_outputs(9)  <= s_master_reset;
logic_outputs(10) <= s_spe_b;
logic_outputs(11) <= s_tmp_ht_pd_b;
logic_outputs(12) <= '1'; --VPP ON

gpo_data <= "000" & logic_outputs(12 downto 0);

--SW readable test signals
--PAD inputs
logic_outputs(16) <= i_fp_on; --TP
logic_outputs(17) <= i_fan_present_b; --TP
logic_outputs(18) <= i_spe; --TP
logic_outputs(19) <= i_watchdog; --TP
logic_outputs(20) <= i_v1_power_good; --TP
logic_outputs(21) <= i_v2_power_good; --TP
logic_outputs(22) <= s_v1_power_fail; --TP
logic_outputs(23) <= s_v2_power_fail; --TP
--PAD outputs
--logic_outputs(24) <= s_spe_b; --TP
logic_outputs(25) <= s_hsc5_alert_out; --TP
logic_outputs(26) <= s_v1_remote_enable; --TP
logic_outputs(27) <= s_v2_remote_enable; --TP
logic_outputs(28) <= s_v1_sw_off; --TP
logic_outputs(29) <= s_v2_sw_off; --TP

--DATA FROM I2C GPI ********************************************
--s_pwr_p5v0_alert_b  <= gpi_data(0);
--s_pwr_p3v3_alert_b  <= gpi_data(1);
--s_pwr_p12v1_alert_b <= gpi_data(2);
--s_pwr_p12v2_alert_b <= gpi_data(3);
--s_pwr_p12v_alert_b  <= gpi_data(4);
--s_pwr_n12v_alert_b  <= gpi_data(5);
--s_pwr_p12v1_ft_b    <= gpi_data(6);
--s_pwr_p12v2_ft_b    <= gpi_data(7);
--s_tmp_ht1_alert_b   <= gpi_data(8);
--s_tmp_ht2_alert_b   <= gpi_data(9);
--s_tmp_alert_b       <= gpi_data(10);
--s_fan_alert_b       <= gpi_data(11);
--s_pm_alert_b        <= gpi_data(12);
s_pwr_p5v0_alert_b  <= logic_inputs(0);
s_pwr_p3v3_alert_b  <= logic_inputs(1);
s_pwr_p12v1_alert_b <= logic_inputs(2);
s_pwr_p12v2_alert_b <= logic_inputs(3);
s_pwr_p12v_alert_b  <= logic_inputs(4);
s_pwr_n12v_alert_b  <= logic_inputs(5);
s_pwr_p12v1_ft_b    <= logic_inputs(6);
s_pwr_p12v2_ft_b    <= logic_inputs(7);
s_tmp_ht1_alert_b   <= logic_inputs(8);
s_tmp_ht2_alert_b   <= logic_inputs(9);
s_tmp_alert_b       <= logic_inputs(10);
s_fan_alert_b       <= logic_inputs(11);
s_pm_alert_b        <= logic_inputs(12);
s_etc_alert_b       <= logic_inputs(13); --NOT USED

--DATA FROM CPU ********************************************
--s_use_supply2       <= cpu2fabric_reg(1)(14); --added 15/10/15
--s_force_shutdown    <= cpu2fabric_reg(1)(15); --added 15/03/16
--s_hsc5_alert_en     <= cpu2fabric_reg(1)(16); --enable hsc5 alert signal
s_use_supply2       <= cpu2fabric_reg(3)(0); --added 15/10/15
s_force_shutdown    <= cpu2fabric_reg(3)(1) when (timeout = '1') else '1'; --changed 17/01/2017 (force shutdown for 1 second after power up)
s_hsc5_alert_en     <= cpu2fabric_reg(3)(2); --enable hsc5 alert signal

--Assign real-time outputs ******************************
o_hsc5_alert       <= s_hsc5_alert_out;
o_v1_remote_enable <= s_v1_remote_enable;
o_v2_remote_enable <= s_v2_remote_enable;
o_v1_sw_off        <= s_v1_sw_off;
o_v2_sw_off        <= s_v2_sw_off;

s_hsc5_alert_out <= s_hsc5_alert; -- when s_hsc5_alert_en = '1' else
                    --s_hsc5_toggle;

--HSC5_TOGGLE : process(i_clk)
--begin
    --if rising_edge(i_clk) then
        --s_hsc5_cnt <= s_hsc5_cnt+1;
    --end if;
--end process;
--s_hsc5_toggle <= s_hsc5_cnt(7);

TIMER_P : process(i_clk)
begin
    if rising_edge(i_clk) then
        if timeout = '0' then
            timer <= timer + 1;
        end if;

        if timer > X"05F5E100" then --100e6
            timeout <= '1';
        end if;
    end if;
end process;


LOGIC_I : mbu_mmc_v2b
port map(
  --debug controls
  i_v1_power_mask    => control_reg(0),
  i_v2_power_mask    => control_reg(1),
  i_fan_present_mask => control_reg(2),
  --testpoints
  o_v1_power_good_tp => open,
  o_v2_power_good_tp => open,
  o_fan_present_b_tp => open,
  o_fp_on_tp         => open,
  -- clocks
  i_clk => i_clk,
  --
  i_force_shutdown => s_force_shutdown, --added 15.3.2016
  i_watchdog   => i_watchdog,
  o_hsc5_alert => s_hsc5_alert,
  -- AC/DC control
  i_use_supply2       => s_use_supply2, --added 15/10/15
  i_v1_power_good     => i_v1_power_good,
  i_v2_power_good     => i_v2_power_good,
  o_v1_remote_enable  => s_v1_remote_enable,
  o_v2_remote_enable  => s_v2_remote_enable,
  o_v1_sw_off         => s_v1_sw_off,
  o_v2_sw_off         => s_v2_sw_off,
  -- POWER ALARMS
  i_pwr_p5v0_alert_b  => s_pwr_p5v0_alert_b, 
  i_pwr_p3v3_alert_b  => s_pwr_p3v3_alert_b, 
  i_pwr_p12v1_alert_b => s_pwr_p12v1_alert_b,
  i_pwr_p12v2_alert_b => s_pwr_p12v2_alert_b,
  i_pwr_p12v_alert_b  => s_pwr_p12v_alert_b, 
  i_pwr_n12v_alert_b  => s_pwr_n12v_alert_b,
  i_pwr_p12v1_ft_b    => s_pwr_p12v1_ft_b, 
  i_pwr_p12v2_ft_b    => s_pwr_p12v2_ft_b,
  -- POWER METER
  i_pm_alert_b        => s_pm_alert_b, 
  -- FAN
  i_fan_present_b     => i_fan_present_b,
  i_fan_alert_b       => s_fan_alert_b,
  o_fan_hw_reset_u1   => s_fan_hw_reset_u1,
  o_fan_hw_reset_u2   => s_fan_hw_reset_u2,
  o_fan_fault1        => s_fan_fault1,
  o_fan_fault2        => s_fan_fault2,
  -- TEMPERATURE
  i_tmp_alert_b       => s_tmp_alert_b, 
  i_tmp_ht1_alert_b   => s_tmp_ht1_alert_b, 
  i_tmp_ht2_alert_b   => s_tmp_ht2_alert_b, 
  o_tmp_ht_pd_b       => s_tmp_ht_pd_b,
  -- FRONT PANEL
  i_fp_on             => i_fp_on,
  o_fp_fw_led         => s_fp_fw_led,
  o_fp_hw_led       => s_fp_hw_led, 
  o_fp_remote_on_led  => s_fp_remote_on_led,
  -- GPAC IF
  o_master_reset      => s_master_reset, 
  i_spe               => i_spe,
  o_spe_b             => s_spe_b 
);

--make sure FAN controller reset never stays asserted for more than 1 second
FAN_RST_FLT_P : process(i_clk)
begin
    if rising_edge(i_clk) then
        s_fan_hw_reset_u1_r <= s_fan_hw_reset_u1;
        --assert reset on rising edge of internal signal, deassert it on timer timeout
        if s_fan_hw_reset_u1_r = '0' and s_fan_hw_reset_u1 = '1' then
            s_fan_hw_reset_u1_p <= '1';
        elsif fan_rst_timeout1 = '1' then
            s_fan_hw_reset_u1_p <= '0';
        end if;
        --count reset assertion time
        if s_fan_hw_reset_u1_p = '1' then
            fan_rst_cnt1 <= fan_rst_cnt1+1;
        else
            fan_rst_cnt1 <= (others => '0');
        end if;
        --timeout after 1 second
        if fan_rst_cnt1 = 100000000 then
            fan_rst_timeout1 <= '1';
        else
            fan_rst_timeout1 <= '0'; --corrected 2/11/15
        end if;
        

        s_fan_hw_reset_u2_r <= s_fan_hw_reset_u2;
        --assert reset on rising edge of internal signal, deassert it on timer timeout
        if s_fan_hw_reset_u2_r = '0' and s_fan_hw_reset_u2 = '1' then
            s_fan_hw_reset_u2_p <= '1';
        elsif fan_rst_timeout2 = '1' then
            s_fan_hw_reset_u2_p <= '0';
        end if;
        --count reset assertion time
        if s_fan_hw_reset_u2_p = '1' then
            fan_rst_cnt2 <= fan_rst_cnt2+1;
        else
            fan_rst_cnt2 <= (others => '0');
        end if;
        --timeout after 1 second
        if fan_rst_cnt2 = 100000000 then
            fan_rst_timeout2 <= '1';
        else
            fan_rst_timeout2 <= '0';  --corrected 2/11/15
        end if;
    end if;
end process;

--ACDC supply enable => power_good takes about 2 seconds
--  don't monitor power_good until that time is elapsed
PWR_FAIL_CONTROL: process(i_clk)
begin
    if rising_edge(i_clk) then
        --POWER SUPPLY1
        --wait 2s after ACDC enable before checking power_good
        if s_v1_remote_enable = '1' and PRESETn_reg = '1' then
            if (acdc_start_cnt1 = (5*CLK_FREQ)) then
                pwrgood_valid1 <= '1';
            else
                pwrgood_valid1 <= '0';
                acdc_start_cnt1 <= acdc_start_cnt1+1;
            end if;
        else
            acdc_start_cnt1 <= 0;
            pwrgood_valid1 <= '0';
        end if;
        
        --if power_good is valid, count the LOW time. Send alarm when reaching 255 occurrences
        if (PRESETn_reg = '0') then
            pwr_fail_cnt1   <= (others => '0');
            s_v1_power_fail <= '0';
        elsif (pwrgood_valid1 = '1' and i_v1_power_good = '0') then
            if (pwr_fail_cnt1 = X"FF") then
                s_v1_power_fail <= '1';
            else
                pwr_fail_cnt1 <= pwr_fail_cnt1 + 1;
                s_v1_power_fail <= '0';
            end if;
        end if;

        --POWER SUPPLY2
        --wait 2s after ACDC enable before checking power_good
        if s_v2_remote_enable = '1' and PRESETn_reg = '1' then
            if (acdc_start_cnt2 = (5*CLK_FREQ)) then
                pwrgood_valid2 <= '1';
            else
                pwrgood_valid2 <= '0';
                acdc_start_cnt2 <= acdc_start_cnt1+1;
            end if;
        else
            acdc_start_cnt2 <= 0;
            pwrgood_valid2 <= '0';
        end if;
        
        --if power_good is valid, count the LOW time. Send alarm when reaching 255 occurrences
        if (PRESETn_reg = '0') then
            pwr_fail_cnt2   <= (others => '0');
            s_v2_power_fail <= '0';
        elsif (pwrgood_valid2 = '1' and i_v2_power_good = '0') then
            if (pwr_fail_cnt2 = X"FF") then
                s_v2_power_fail <= '1';
            else
                pwr_fail_cnt2 <= pwr_fail_cnt2 + 1;
                s_v2_power_fail <= '0';
            end if;
        end if;
    end if;
end process;

end architecture struct;

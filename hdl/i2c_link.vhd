------------------------------------------------------------------------------
--                       Paul Scherrer Institute (PSI)
------------------------------------------------------------------------------
-- Unit    : i2c_link.vhd
-- Author  : Goran Marinkovic, Section Diagnostic
-- Version : $Revision: 1.2 $
------------------------------------------------------------------------------
-- Copyright© PSI, Section Diagnostic
------------------------------------------------------------------------------
-- Comment : This is the package for the I2C link.
------------------------------------------------------------------------------

------------------------------------------------------------------------------
-- Module I2C Package
------------------------------------------------------------------------------
-- Std. library (platform) ---------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;

-- Work library (platform) ---------------------------------------------------

-- Work library (application) ------------------------------------------------

package i2c_link_package is

   ---------------------------------------------------------------------------
   -- Module I2C link - sequential byte read/write
   ---------------------------------------------------------------------------
   component i2c_link_v4
   generic
   (
      c_flnk_bit_clk_div          : integer range 8 to 1023 := 625
   );
   port
   (
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_rst                       : in    std_logic; --ML84 added
      i_clk                       : in    std_logic;
      ------------------------------------------------------------------------
      -- Parallel link interface
      ------------------------------------------------------------------------
      i_i2c_clk_divider           : in    std_logic_vector(9 downto 0);
      o_i2c_idle                  : out   std_logic;
      o_i2c_fail                  : out   std_logic;
      o_i2c_busbusy               : out   std_logic;
      i_i2c_tx_rdy                : in    std_logic;
      o_i2c_tx_ack                : out   std_logic;
      i_i2c_tx_data               : in    std_logic_vector( 7 downto  0);
      i_i2c_tx_start              : in    std_logic;
      i_i2c_tx_stop               : in    std_logic;
      o_i2c_rx_data               : out   std_logic_vector( 7 downto  0);
      o_i2c_rx_valid              : out   std_logic;
      ------------------------------------------------------------------------
      -- Serial link interface
      ------------------------------------------------------------------------
      o_i2c_scl_tx                : out   std_logic;
      i_i2c_scl_rx                : in    std_logic;
      o_i2c_sda_tx                : out   std_logic;
      i_i2c_sda_rx                : in    std_logic
   );
   end component i2c_link_v4;

end package i2c_link_package;

------------------------------------------------------------------------------
-- End of package
------------------------------------------------------------------------------

------------------------------------------------------------------------------
------------------------------------------------------------------------------
------------------------------------------------------------------------------
-- Module I2C link version 4
------------------------------------------------------------------------------
------------------------------------------------------------------------------
------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.ALL;
use ieee.std_logic_unsigned.ALL;
use ieee.std_logic_misc.all;
use ieee.std_logic_arith.all;

entity i2c_link_v4 is
   generic
   (
      c_flnk_bit_clk_div          : integer range 8 to 1023 := 625
   );
   port
    (
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_rst                       : in    std_logic; --ml84 added
      i_clk                       : in    std_logic;
      ------------------------------------------------------------------------
      -- Parallel link interface
      ------------------------------------------------------------------------
      i_i2c_clk_divider           : in    std_logic_vector(9 downto 0);
      o_i2c_idle                  : out   std_logic;
      o_i2c_fail                  : out   std_logic;
      o_i2c_busbusy               : out   std_logic;
      i_i2c_tx_rdy                : in    std_logic;
      o_i2c_tx_ack                : out   std_logic;
      i_i2c_tx_data               : in    std_logic_vector( 7 downto  0);
      i_i2c_tx_start              : in    std_logic;
      i_i2c_tx_stop               : in    std_logic;
      o_i2c_rx_data               : out   std_logic_vector( 7 downto  0);
      o_i2c_rx_valid              : out   std_logic;
      ------------------------------------------------------------------------
      -- Serial link interface
      ------------------------------------------------------------------------
      o_i2c_scl_tx                : out   std_logic;
      i_i2c_scl_rx                : in    std_logic;
      o_i2c_sda_tx                : out   std_logic;
      i_i2c_sda_rx                : in    std_logic
   );
end i2c_link_v4;

architecture behavioral of i2c_link_v4 is

   -- Types ------------------------------------------------------------------
   -- Types used only in this interface
   type clk_state_type is 
      (
         clk_l2,
         clk_r,
         clk_h1,
         clk_hm,
         clk_h2,
         clk_f,
         clk_l1,
         clk_lm
      );

   type frame_state_type is 
      (
         reset,
         bus_busy,
         idle,
         fetch_addr,
         --addr
         cd_sr,
         cd_s1,
         cd_s2,
         cd_addr,
         cd_addr_ack,
         --write
         wr_byte_fetch,
         wr_byte_send,
         wr_byte_ack,
         --read
         rd_byte_count_fetch,
         rd_byte_receive,
         rd_byte_store,
         rd_byte_ack,
         rd_byte_nack,
         
         --terminate
         cd_pe,
         cd_p1,
         cd_p2,
         cd_p3
      );
   -- Signals ----------------------------------------------------------------
   signal frame_clk_f_trig        : std_logic := '0';
   signal frame_clk_lm_trig       : std_logic := '0';
   signal frame_clk_r_trig        : std_logic := '0';
   signal frame_clk_hm_trig       : std_logic := '0';

   signal frame_8bit_cnt          : std_logic_vector( 3 downto  0) := (others => '0');
   signal frame_8bit_trig         : std_logic := '0';
   --signal frame_reset_cnt         : std_logic_vector(13 downto  0) := "11000110010100";
   signal frame_reset_cnt         : std_logic_vector(13 downto  0) := "10000000000100";
   signal frame_reset_trig        : std_logic := '0';
   signal frame_state             : frame_state_type := reset;

   signal clk_cnt                 : std_logic_vector( 10 downto  0) := (others => '0');
   signal clk_trig                : std_logic := '0';
   signal clk_state               : clk_state_type;

   signal sl_i2c_rd               : std_logic := '0';
   signal slv_rx_count            : std_logic_vector( 7 downto  0) := (others => '0');
   signal slv_tx_byte             : std_logic_vector( 7 downto  0) := (others => '0');
   signal slv_rx_byte             : std_logic_vector( 7 downto  0) := (others => '0');
   signal i2c_sda_sel             : std_logic_vector( 2 downto  0) := "001";
   signal i2c_sda_addr            : std_logic := '0';
   signal i2c_sda_cmd             : std_logic := '0';
   signal i2c_sda_data_tx         : std_logic := '0';
   signal i2c_sda_tx              : std_logic := '1';
   signal i2c_sda_rx              : std_logic := '1';
   signal i2c_scl_rx              : std_logic := '1';
   signal i2c_scl_sel             : std_logic_vector( 1 downto  0) := (others => '0');
   signal i2c_scl_clk             : std_logic := '0';
   signal i2c_scl                 : std_logic := '0';
   signal sl_i2c_stop             : std_logic := '0';
   signal i2c_fail                : std_logic := '0';

   signal slv_i2c_clk_divider     : std_logic_vector(9 downto 0) := CONV_STD_LOGIC_VECTOR(c_flnk_bit_clk_div - 2, 10);
   signal slv_divider_change      : std_logic;

begin
   ---------------------------------------------------------------------------
   -- Send frame frame_state machine
   ---------------------------------------------------------------------------
   frame_state_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
          if (i_rst = '1') then --ML84 added
              frame_state <= reset;
          else
         case frame_state is
            --idle state ------------------------
            when reset =>
               i2c_scl_sel        <= "01";
               i2c_sda_sel        <= "001";
               if (frame_reset_trig = '1') then
                  frame_state     <= bus_busy;
               end if;
            when bus_busy => 
               i2c_scl_sel        <= "01";
               i2c_sda_sel        <= "001";
               if (i2c_sda_rx = '1' and i2c_scl_rx = '1') then
                  frame_state     <= idle;
               end if;               
            when idle =>
               i2c_scl_sel        <= "01";
               i2c_sda_sel        <= "001";
               if (i2c_sda_rx = '0' and i2c_scl_rx = '0') then
                  frame_state     <= bus_busy;
               end if;               
               if i_i2c_tx_rdy = '1' then
                  frame_state     <= fetch_addr;
               end if;
            when fetch_addr =>
              if (i_i2c_tx_start = '1') and (i_i2c_tx_stop = '0') then
                frame_state     <= cd_s1;
              else
                frame_state     <= idle;
              end if;
            --transmit address------------------------
            when cd_sr =>
              if (frame_clk_r_trig = '1') then
                frame_state     <= cd_s1;
              end if;
            when cd_s1 =>
               i2c_scl_sel        <= "01";
               i2c_sda_sel        <= "001";
               if (frame_clk_hm_trig = '1') then
                  frame_state     <= cd_s2;
               end if;
            when cd_s2 =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "000";
               if (frame_clk_lm_trig = '1') then
                  frame_state     <= cd_addr;
               end if;
            when cd_addr =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "010";
               if (frame_8bit_trig = '1') then
                  frame_state     <= cd_addr_ack;
               end if;
            when cd_addr_ack =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "001";
               if (frame_clk_hm_trig = '1') and (i2c_sda_rx = '1') then --no acknowledge
                  frame_state     <= cd_pe;
               elsif (frame_clk_lm_trig = '1') then
                 if i_i2c_tx_rdy = '0' then --no data available in tx_fifo
                   frame_state     <= cd_pe;
                 else --data available
                   if sl_i2c_rd = '1' then --read
                    frame_state     <= rd_byte_count_fetch;
                   else --write
                    frame_state     <= wr_byte_fetch;
                   end if;
                 end if;
               end if;
            --transmit data------------------------
            when wr_byte_fetch =>
              if (i_i2c_tx_start = '1') then --repeated start
                frame_state     <= cd_sr;
              else --transmit data byte
                frame_state     <= wr_byte_send;
              end if;
            when wr_byte_send =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "010";
               if (frame_8bit_trig = '1') then
                  frame_state     <= wr_byte_ack;
               end if;
            when wr_byte_ack =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "001";
               if (frame_clk_hm_trig = '1') and (i2c_sda_rx = '1') then --no slave acknowledge
                  frame_state     <= cd_pe;
               elsif (frame_clk_lm_trig = '1') then
                 if sl_i2c_stop = '1' then --end of the transmission
                  frame_state     <= cd_p1;
                 else
                   if i_i2c_tx_rdy = '0' then --no data available in tx_fifo
                     frame_state     <= cd_pe;
                   else --data available
                     frame_state     <= wr_byte_fetch;
                   end if;
                 end if;
               end if;
            --receive data------------------------
            when rd_byte_count_fetch =>
              i2c_scl_sel        <= "10";
              i2c_sda_sel        <= "001";
              frame_state     <= rd_byte_receive;            
            when rd_byte_receive =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "001";
               if (frame_8bit_trig = '1') then
                  frame_state     <= rd_byte_store;
               end if;
            when rd_byte_store =>
              i2c_scl_sel        <= "10";
              i2c_sda_sel        <= "001";
              if or_reduce(slv_rx_count) = '1' then
                frame_state        <= rd_byte_ack;
              else
                frame_state        <= rd_byte_nack;
              end if;
            when rd_byte_ack =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "000";
               if (frame_clk_lm_trig = '1') then
                frame_state        <= rd_byte_receive;
               end if;
            when rd_byte_nack =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "001";
               if (frame_clk_lm_trig = '1') then
                  frame_state     <= cd_p1;
               end if;

            --transmission termination------------------------
            when cd_pe =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "001";
               if (frame_clk_lm_trig = '1') then
                  frame_state     <= cd_p1;
               end if;
            when cd_p1 =>
               i2c_scl_sel        <= "10";
               i2c_sda_sel        <= "000";
               if (frame_clk_hm_trig = '1') then
                  frame_state     <= cd_p2;
               end if;
            when cd_p2 =>
               i2c_scl_sel        <= "01";
               i2c_sda_sel        <= "000";
               if (frame_clk_f_trig = '1') then
                  frame_state     <= cd_p3;
               end if;
            when cd_p3 =>
               i2c_scl_sel        <= "01";
               i2c_sda_sel        <= "001";
               if (frame_clk_r_trig = '1') then
                  frame_state     <= idle;
               end if;

            when others =>
               frame_state        <= reset;
         end case;
     end if; --reset
      end if; --clock
   end process frame_state_proc;

  ---------------------------------------------------------------------------
  -- I2C link status
  ---------------------------------------------------------------------------
  o_i2c_idle    <= '1' when (frame_state = idle)      else '0';
  o_i2c_busbusy <= '1' when (frame_state = bus_busy)  else '0';

   i2c_fail_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         case frame_state is
         when idle =>
            if (i_i2c_tx_rdy = '1') then
               i2c_fail           <= '0';
            end if;
         when cd_pe =>
            if (frame_clk_lm_trig = '1') then
               i2c_fail           <= '1';
            end if;
         when others =>
            null;
         end case;
      end if;
   end process i2c_fail_proc;

   o_i2c_fail            <= i2c_fail;

   ---------------------------------------------------------------------------
   -- clock count
   ---------------------------------------------------------------------------
   clk_cnt_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
        slv_divider_change <= or_reduce(slv_i2c_clk_divider xor i_i2c_clk_divider);
        if slv_divider_change = '1' then
          slv_i2c_clk_divider <= i_i2c_clk_divider;
        end if;
        if ((frame_state = idle) or (clk_cnt(clk_cnt'high) = '0')) then
          clk_cnt               <= '1' & slv_i2c_clk_divider; --CONV_STD_LOGIC_VECTOR(c_flnk_bit_clk_div - 2, 10);
        else
          clk_cnt               <= clk_cnt - X"1";
        end if;
      end if;
   end process clk_cnt_proc;

   clk_trig                       <= not clk_cnt(clk_cnt'high);

   ---------------------------------------------------------------------------
   -- clk_state machine
   ---------------------------------------------------------------------------
   clk_state_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (frame_state = idle) then
            clk_state             <= clk_f;
         else
            case clk_state is
               when clk_f =>
                  clk_state       <= clk_l1;
               when clk_l1 =>
                  if (clk_trig = '1') then
                     clk_state    <= clk_lm;
                  end if;
               when clk_lm =>
                  clk_state       <= clk_l2;
               when clk_l2 =>
                  if (clk_trig = '1') then
                     clk_state    <= clk_r;
                  end if;
               when clk_r =>
                  clk_state       <= clk_h1;
               when clk_h1 =>
                  if (clk_trig = '1') and (i2c_scl_rx = '1') then
                     clk_state    <= clk_hm;
                  end if;
               when clk_hm =>
                  clk_state       <= clk_h2;
               when clk_h2 =>
                  if (clk_trig = '1') then
                     clk_state    <= clk_f;
                  end if;
               when others =>
                  clk_state       <= clk_l2;
            end case;
         end if;
      end if;
   end process clk_state_proc;

   ---------------------------------------------------------------------------
   -- clock edge trigger
   ---------------------------------------------------------------------------
   frame_clk_f_trig_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (clk_state = clk_f) then
            frame_clk_f_trig      <= '1';
         else
            frame_clk_f_trig      <= '0';
         end if;
      end if;
   end process frame_clk_f_trig_proc;

   frame_clk_lm_trig_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (clk_state = clk_lm) then
            frame_clk_lm_trig     <= '1';
         else
            frame_clk_lm_trig     <= '0';
         end if;
      end if;
   end process frame_clk_lm_trig_proc;

   frame_clk_r_trig_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (clk_state = clk_r) then
            frame_clk_r_trig      <= '1';
         else
            frame_clk_r_trig      <= '0';
         end if;
      end if;
   end process frame_clk_r_trig_proc;

   frame_clk_hm_trig_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (clk_state = clk_hm) then
            frame_clk_hm_trig     <= '1';
         else
            frame_clk_hm_trig     <= '0';
         end if;
      end if;
   end process frame_clk_hm_trig_proc;

   ---------------------------------------------------------------------------
   -- 8 bit counter trigger
   ---------------------------------------------------------------------------
   frame_8bit_cnt_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (frame_state = wr_byte_send) or (frame_state = rd_byte_receive) or (frame_state = cd_addr) then
            if (clk_state = clk_lm) then
               frame_8bit_cnt     <= frame_8bit_cnt - "0001";
            end if;
         else
            frame_8bit_cnt        <= "1111";
         end if;
      end if;
   end process frame_8bit_cnt_proc;

   frame_8bit_trig                <= not frame_8bit_cnt(frame_8bit_cnt'high);

   ---------------------------------------------------------------------------
   -- reset counter trigger
   ---------------------------------------------------------------------------
   frame_reset_cnt_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (frame_state = reset) then
            if (clk_state = clk_lm) then
               frame_reset_cnt    <= frame_reset_cnt - "00000000000001";
            end if;
         else
            frame_reset_cnt       <= "11000110010100";
            --frame_reset_cnt       <= "10000000000100";
         end if;
      end if;
   end process frame_reset_cnt_proc;

   frame_reset_trig               <= not frame_reset_cnt(frame_reset_cnt'high);

   ---------------------------------------------------------------------------
   -- tx fetch data, tx shift register
   ---------------------------------------------------------------------------
  o_i2c_tx_ack <= '1' when (frame_state = fetch_addr) or (frame_state = wr_byte_fetch) or (frame_state = rd_byte_count_fetch) else '0';
   
  prc_tx_fetch : process ( i_clk )
    variable frame_state_prev : frame_state_type;
  begin
    if rising_edge( i_clk ) then
      --fetch RD/NWR, address and stop
      if frame_state = fetch_addr then      
        sl_i2c_rd     <= i_i2c_tx_data(0);
        sl_i2c_stop   <= i_i2c_tx_stop;
        slv_tx_byte   <= i_i2c_tx_data(7 downto 0);
      end if;
      --fetch data byte
      if frame_state = wr_byte_fetch then            
        sl_i2c_rd     <= i_i2c_tx_data(0);
        sl_i2c_stop   <= i_i2c_tx_stop;
        slv_tx_byte   <= i_i2c_tx_data(7 downto 0);
      end if;
      --shift register
      if (i2c_sda_sel = "010") and (frame_clk_lm_trig = '1'  ) then
        slv_tx_byte           <= slv_tx_byte( 6 downto  0) & slv_tx_byte( 7);
      end if;
      i2c_sda_addr             <= slv_tx_byte(slv_tx_byte'high);
    end if;
  end process ;
      
   ---------------------------------------------------------------------------
   -- rx shift register, byte counter
   ---------------------------------------------------------------------------
  i2c_data_rx_proc: process(i_clk)
  begin
    if rising_edge(i_clk) then
      --fetch byte count
      if frame_state = rd_byte_count_fetch then            
        slv_rx_count  <= i_i2c_tx_data(7 downto 0) - X"01";
      end if;      
      --shift register
      if frame_state = rd_byte_receive then
        if frame_clk_hm_trig = '1' then
          slv_rx_byte     <= slv_rx_byte( 6 downto  0) & i2c_sda_rx;
        end if;
      end if;
      --valid data and count bytes
      if frame_state = rd_byte_store then
        o_i2c_rx_valid  <= '1';
        slv_rx_count    <= slv_rx_count - X"1";
      else
        o_i2c_rx_valid  <= '0';
      end if;      
    end if;
  end process i2c_data_rx_proc;

  o_i2c_rx_data                  <= slv_rx_byte;

   ---------------------------------------------------------------------------
   -- I2C sda tx
   ---------------------------------------------------------------------------
   i2c_sda_tx_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
          if i_rst = '1' then
              i2c_sda_tx <= '1';
          else
            case i2c_sda_sel is
              when "000" =>
                i2c_sda_tx         <= '0';
              when "001" =>
                i2c_sda_tx         <= '1';
              when "010" =>
                i2c_sda_tx         <= i2c_sda_addr;
              when others =>
                i2c_sda_tx         <= '1';
            end case;
          end if;
      end if;
   end process i2c_sda_tx_proc;

   o_i2c_sda_tx                   <= i2c_sda_tx;

   ---------------------------------------------------------------------------
   -- I2C sda rx
   ---------------------------------------------------------------------------
   i2c_rx_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         i2c_sda_rx               <= i_i2c_sda_rx;
         i2c_scl_rx               <= i_i2c_scl_rx;
      end if;
   end process i2c_rx_proc;

   ---------------------------------------------------------------------------
   -- I2C clock
   ---------------------------------------------------------------------------
   i2c_scl_clk_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         if    (frame_state = idle) then
            i2c_scl_clk           <= '1';
         else
            if    (frame_clk_f_trig = '1') then
               i2c_scl_clk        <= '0';
            elsif (frame_clk_r_trig = '1') then
               i2c_scl_clk        <= '1';
            end if;
         end if;
      end if;
   end process i2c_scl_clk_proc;

   ---------------------------------------------------------------------------
   -- I2C scl
   ---------------------------------------------------------------------------
   i2c_scl_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
          if i_rst = '1' then
            case i2c_scl_sel is
              when "00" =>
                i2c_scl            <= '0';
              when "01" =>
                i2c_scl            <= '1';
              when "10" =>
                i2c_scl            <= i2c_scl_clk;
              when others =>
                i2c_scl            <= '1';
            end case;
          end if;
      end if;
   end process i2c_scl_proc;

   o_i2c_scl_tx                   <= i2c_scl;

end behavioral;

------------------------------------------------------------------------------
-- End of file
------------------------------------------------------------------------------

------------------------------------------------------------------------------
--                       Paul Scherrer Institute (PSI)
------------------------------------------------------------------------------
-- Unit    : mmap_package_multi.vhd
-- Author  : Goran Marinkovic, Alessandro Malatesta, Section Diagnostic
-- Version : $Revision: 1.4 $
------------------------------------------------------------------------------
-- Copyright© PSI, Section Diagnostic
------------------------------------------------------------------------------
-- Comment : This is the package for the I2C memory map.
------------------------------------------------------------------------------
------------------------------------------------------------------------------
-- Module MMAP Package
------------------------------------------------------------------------------
-- Std. library (platform) ---------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;

-- Work library (platform) ---------------------------------------------------

-- Work library (application) ------------------------------------------------

package mmap_package_multi is

   ---------------------------------------------------------------------------
   -- Types
   ---------------------------------------------------------------------------
   type rom_entry_type is record
      dev                         : std_logic_vector( 7 downto  0);
      reg_len                     : std_logic_vector( 1 downto  0);
      reg                         : std_logic_vector(15 downto  0);
      data_len                    : std_logic_vector( 2 downto  0);
      rep                         : std_logic;
   end record rom_entry_type;

   type rom_type is array (natural range <>) of rom_entry_type;

   ---------------------------------------------------------------------------
   -- Module ROM
   ---------------------------------------------------------------------------
   component rom
   generic
   (
      c_storage                   : rom_type
   );
   port
    (
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_clk                       : in    std_logic;
      ------------------------------------------------------------------------
      -- Data interface
      ------------------------------------------------------------------------
      i_addr                      : in    std_logic_vector( 9 downto  0);
      o_data                      : out   rom_entry_type
   );
   end component rom;

   ---------------------------------------------------------------------------
   -- Module MMAP
   ---------------------------------------------------------------------------
   component mmap
   generic
   (
      c_fpga                      : string := "virtex5";
      c_num_to_read               : std_logic_vector(11 downto 0); --ML84: number of fields to read
      c_i2c_clk_div               : integer range 8 to 1023 := 200; --ML84
      c_storage                   : rom_type
   );
   port
   (
      i2c_start_id1               : in std_logic_vector( 9 downto  0); --ML84
      ------------------------------------------------------------------------
      -- Debug interface
      ------------------------------------------------------------------------
      debug                       : out   std_logic_vector(127 downto  0);
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_rst                       : in    std_logic;
      i_clk                       : in    std_logic;
      ------------------------------------------------------------------------
      -- Data interface
      ------------------------------------------------------------------------
      i_trig                      : in    std_logic;
      i_irq                       : in    std_logic_vector( 3 downto  0);
      i_rw                        : in    std_logic;
      i_wr_index                  : in    std_logic_vector( 9 downto  0);
      i_wr_data                   : in    std_logic_vector(31 downto  0);
      o_wr_empty                  : out   std_logic;
      o_wr_full                   : out   std_logic;
      i_rd_index                  : in    std_logic_vector( 9 downto  0);
      o_rd_data                   : out   std_logic_vector(31 downto  0);
      ------------------------------------------------------------------------
      -- Interrupt interface
      ------------------------------------------------------------------------
      o_int                       : out   std_logic_vector( 3 downto  0);
      ------------------------------------------------------------------------
      -- Periodic read interface
      ------------------------------------------------------------------------
      i_update                    : in    std_logic;
      o_busy                      : out   std_logic;
      o_periodic                  : out   std_logic;
      ------------------------------------------------------------------------
      -- Serial link interface
      ------------------------------------------------------------------------
      o_i2c_scl_tx                : out   std_logic_vector(1 downto 0); --ML84
      i_i2c_scl_rx                : in    std_logic_vector(1 downto 0);
      o_i2c_sda_tx                : out   std_logic_vector(1 downto 0);
      i_i2c_sda_rx                : in    std_logic_vector(1 downto 0)
   );
   end component mmap;

end package mmap_package_multi;

------------------------------------------------------------------------------
-- End of package
------------------------------------------------------------------------------

------------------------------------------------------------------------------
-- Module ROM
------------------------------------------------------------------------------
-- Std. library (platform) ---------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- Work library (platform) ---------------------------------------------------

-- Work library (application) ------------------------------------------------
--library plb46_to_iic_mmap_v1_00_a;
use work.mmap_package_multi.all;

entity rom is
   generic
   (
      c_storage                   : rom_type
   );
   port
    (
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_clk                       : in    std_logic;
      ------------------------------------------------------------------------
      -- Data interface
      ------------------------------------------------------------------------
      i_addr                      : in    std_logic_vector( 9 downto  0);
      o_data                      : out   rom_entry_type
   );
end rom;

architecture behavioral of rom is

   -- Signals ----------------------------------------------------------------
   signal   data                  : rom_entry_type := (dev => X"00", reg_len => "00", reg => X"0000", data_len => "000", rep => '0');

begin

   data_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         data                     <= c_storage(to_integer(unsigned(i_addr)));
      end if;
   end process data_proc;

   o_data                         <= data;

end behavioral;

------------------------------------------------------------------------------
-- Module MMAP
------------------------------------------------------------------------------
-- Std. library (platform) ---------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- Work library (platform) ---------------------------------------------------

-- Work library (application) ------------------------------------------------
--library plb46_to_iic_mmap_v1_00_a;
use work.mmap_package_multi.all;
use work.i2c_link_package.all;

entity mmap is
   generic
   (
      c_fpga                      : string := "virtex5";
      c_num_to_read               : std_logic_vector(11 downto 0); --ML84: number of fields to read
      c_i2c_clk_div               : integer range 8 to 1023 := 200; --ML84
      c_storage                   : rom_type
   );
   port
   (
      i2c_start_id1               : in std_logic_vector( 9 downto  0); --ML84
      ------------------------------------------------------------------------
      -- Debug interface
      ------------------------------------------------------------------------
      debug                       : out   std_logic_vector(127 downto  0);
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_rst                       : in    std_logic;
      i_clk                       : in    std_logic;
      ------------------------------------------------------------------------
      -- Data interface
      ------------------------------------------------------------------------
      i_trig                      : in    std_logic;
      i_irq                       : in    std_logic_vector( 3 downto  0);
      i_rw                        : in    std_logic;
      i_wr_index                  : in    std_logic_vector( 9 downto  0);
      i_wr_data                   : in    std_logic_vector(31 downto  0);
      o_wr_empty                  : out   std_logic;
      o_wr_full                   : out   std_logic;
      i_rd_index                  : in    std_logic_vector( 9 downto  0);
      o_rd_data                   : out   std_logic_vector(31 downto  0);
      ------------------------------------------------------------------------
      -- Interrupt interface
      ------------------------------------------------------------------------
      o_int                       : out   std_logic_vector( 3 downto  0);
      ------------------------------------------------------------------------
      -- Periodic read interface
      ------------------------------------------------------------------------
      i_update                    : in    std_logic;
      o_busy                      : out   std_logic;
      o_periodic                  : out   std_logic;
      ------------------------------------------------------------------------
      -- Serial link interface
      ------------------------------------------------------------------------
      o_i2c_scl_tx                : out   std_logic_vector(1 downto 0); --ML84
      i_i2c_scl_rx                : in    std_logic_vector(1 downto 0);
      o_i2c_sda_tx                : out   std_logic_vector(1 downto 0);
      i_i2c_sda_rx                : in    std_logic_vector(1 downto 0)
   );
end mmap;

architecture behavioral of mmap is

   ---------------------------------------------------------------------------
   -- Types
   ---------------------------------------------------------------------------
   type state_type is 
   (
      idle,
      single,
      single_read,
      periodic,
      periodic_read,
      periodic_check,
      header_check,
      read_req_0,
      read_req_1,
      read_req_2,
      read_req_3,
      read_req_4,
      read_req_5,
      read_req_6,
      read_req_7,
      read_req_8,
      read_req_9,
      read_req_10,
      read_req_11,
      write_req_0,
      write_req_1,
      write_req_2,
      write_req_3,
      write_req_4,
      write_req_5,
      write_req_6,
      write_req_7,
      done
   );
   ---------------------------------------------------------------------------
   -- Globals
   ---------------------------------------------------------------------------
   constant HIGH                  : std_logic := '1';
   constant LOW                   : std_logic := '0';
   constant HIGH4                 : std_logic_vector( 3 downto  0) := X"F";
   constant LOW4                  : std_logic_vector( 3 downto  0) := X"0";
   constant HIGH8                 : std_logic_vector( 7 downto  0) := X"FF";
   constant LOW8                  : std_logic_vector( 7 downto  0) := X"00";
   constant HIGH16                : std_logic_vector(15 downto  0) := X"FFFF";
   constant LOW16                 : std_logic_vector(15 downto  0) := X"0000";
   constant HIGH32                : std_logic_vector(31 downto  0) := X"FFFFFFFF";
   constant LOW32                 : std_logic_vector(31 downto  0) := X"00000000";
   ---------------------------------------------------------------------------
   -- Update interface
   ---------------------------------------------------------------------------
   signal   update                : std_logic_vector( 1 downto  0) := (others => '0');
   ---------------------------------------------------------------------------
   -- Memory interface
   ---------------------------------------------------------------------------
   signal   fifo_wr_en            : std_logic := '0';
   signal   fifo_wr_data          : std_logic_vector(47 downto  0) := (others => '0');
   signal   fifo_wr_full          : std_logic := '0';
   signal   fifo_rd_en            : std_logic := '0';
   signal   fifo_rd_data          : std_logic_vector(47 downto  0) := (others => '0');
   signal   fifo_rd_empty         : std_logic := '1';

   signal   bram_wr_en            : std_logic := '0';
   signal   bram_wr_addr          : std_logic_vector( 9 downto  0) := (others => '0');
   signal   bram_wr_data          : std_logic_vector(31 downto  0) := (others => '0');
   signal   bram_rd_addr          : std_logic_vector( 9 downto  0) := (others => '0');
   signal   bram_rd_data          : std_logic_vector(31 downto  0) := (others => '0');

   signal   header                : rom_entry_type;
   signal   rx_data               : std_logic_vector(31 downto  0) := (others => '0');
   ---------------------------------------------------------------------------
   -- FSM interface
   ---------------------------------------------------------------------------
   signal   req_pending           : std_logic := '0';
   signal   req_periodic          : std_logic := '0';
   signal   req_irq               : std_logic_vector( 3 downto  0) := (others => '0');
   signal   req_rw                : std_logic := '0';
   signal   req_index             : std_logic_vector( 9 downto  0) := (others => '0');
   signal   req_wr_data           : std_logic_vector(31 downto  0) := (others => '0');

   signal   state                 : state_type;
   ---------------------------------------------------------------------------
   -- I2C link interface
   ---------------------------------------------------------------------------
   constant i2c_clk_divider       : std_logic_vector(9 downto 0) := "0001001110"; -- 400 kHz (50 kHz := "1001110001")
   signal   i2c_idle              : std_logic := '0';
   signal   i2c_fail              : std_logic := '0';
   signal   i2c_fail_cnt          : unsigned( 3 downto  0) := (others => '0');
   signal   i2c_busbusy           : std_logic := '0';
   signal   i2c_tx_rdy            : std_logic := '0';
   signal   i2c_tx_ack            : std_logic := '0';
   signal   i2c_tx_data           : std_logic_vector( 7 downto  0) := (others => '0');
   signal   i2c_tx_start          : std_logic := '0';
   signal   i2c_tx_stop           : std_logic := '0';
   signal   i2c_rx_data           : std_logic_vector( 7 downto  0) := (others => '0');
   signal   i2c_rx_valid          : std_logic := '0';
   ---------------------------------------------------------------------------
   -- Automatic update interface
   ---------------------------------------------------------------------------
   signal   update_cnt            : unsigned(11 downto  0) := (others => '0');
   ---------------------------------------------------------------------------
   -- Component declaration
   ---------------------------------------------------------------------------

   --ML84 added declarations for microsemi support
   signal fifo_rd_empty_x, fifo_rd_en_x : std_logic;
   --ML84 added signals to support I2C slave switching
   signal i2c_sel : natural range 0 to 1;
   signal i2c_scl_tx, i2c_scl_rx, i2c_sda_tx, i2c_sda_rx : std_logic;


   component smartfusion_fifo_32x48 is
   port( 
      din   : in    std_logic_vector(47 downto 0);
      dout  : out   std_logic_vector(47 downto 0);
      wr_en : in    std_logic;
      rd_en : in    std_logic;
      wclk  : in    std_logic;
      rclk  : in    std_logic;
      full  : out   std_logic;
      empty : out   std_logic;
      srst  : in    std_logic
   );
   end component smartfusion_fifo_32x48;

   signal bram_wr_en_n : std_logic;

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
   -------------------------------------------------

   component spartan3a_fifo_128x48
   port
   (
      clk                         : in    std_logic;
      srst                        : in    std_logic;
      wr_en                       : in    std_logic;
      din                         : in    std_logic_vector(47 downto  0);
      full                        : out   std_logic;
      rd_en                       : in    std_logic;
      dout                        : out   std_logic_vector(47 downto  0);
      empty                       : out   std_logic
   );
   end component;

   component spartan3a_bram_1024x32
   port
   (
      clka                        : in    std_logic;
      wea                         : in    std_logic_vector( 0 downto  0);
      addra                       : in    std_logic_vector( 9 downto  0);
      dina                        : in    std_logic_vector(31 downto  0);
      douta                       : out   std_logic_vector(31 downto  0);
      clkb                        : in    std_logic;                  
      web                         : in    std_logic_vector( 0 downto  0);
      addrb                       : in    std_logic_vector( 9 downto  0);
      dinb                        : in    std_logic_vector(31 downto  0);
      doutb                       : out   std_logic_vector(31 downto  0)
   );
   end component;

   component virtex5_fifo_128x48
   port
   (
      clk                         : in    std_logic;
      srst                        : in    std_logic;
      wr_en                       : in    std_logic;
      din                         : in    std_logic_vector(47 downto  0);
      full                        : out   std_logic;
      rd_en                       : in    std_logic;
      dout                        : out   std_logic_vector(47 downto  0);
      empty                       : out   std_logic
   );
   end component;

   component virtex5_bram_1024x32
   port
   (
      clka                        : in    std_logic;
      wea                         : in    std_logic_vector( 0 downto  0);
      addra                       : in    std_logic_vector( 9 downto  0);
      dina                        : in    std_logic_vector(31 downto  0);
      douta                       : out   std_logic_vector(31 downto  0);
      clkb                        : in    std_logic;                  
      web                         : in    std_logic_vector( 0 downto  0);
      addrb                       : in    std_logic_vector( 9 downto  0);
      dinb                        : in    std_logic_vector(31 downto  0);
      doutb                       : out   std_logic_vector(31 downto  0)
   );
   end component;

begin
   ---------------------------------------------------------------------------
   -- Debug
   ---------------------------------------------------------------------------
   debug( 63 downto   0)          <= (others => '0');
   debug( 64)                     <= req_pending;
   debug( 65)                     <= req_periodic;
   debug( 66)                     <= req_rw;
   debug( 73 downto  67)          <= req_index( 6 downto  0);
   debug( 78 downto  74)          <= "00000" when (state = idle) else
                                     "00001" when (state = single) else
                                     "00010" when (state = single_read) else
                                     "00011" when (state = periodic) else
                                     "00100" when (state = periodic_read) else
                                     "00101" when (state = periodic_check) else
                                     "00110" when (state = header_check) else
                                     "00111" when (state = read_req_0) else
                                     "01000" when (state = read_req_1) else
                                     "01001" when (state = read_req_2) else
                                     "01010" when (state = read_req_3) else
                                     "01011" when (state = read_req_4) else
                                     "01100" when (state = read_req_5) else
                                     "01101" when (state = read_req_6) else
                                     "01110" when (state = read_req_7) else
                                     "01111" when (state = read_req_8) else
                                     "10000" when (state = read_req_9) else
                                     "10001" when (state = read_req_10) else
                                     "10010" when (state = read_req_11) else
                                     "10011" when (state = write_req_0) else
                                     "10100" when (state = write_req_1) else
                                     "10101" when (state = write_req_2) else
                                     "10110" when (state = write_req_3) else
                                     "10111" when (state = write_req_4) else
                                     "11000" when (state = write_req_5) else
                                     "11001" when (state = write_req_6) else
                                     "11010" when (state = write_req_7) else
                                     "11011" when (state = done) else
                                     "11111";
   debug( 86 downto  79)         <= header.dev;
   debug( 88 downto  87)         <= header.reg_len;
   debug(104 downto  89)         <= header.reg;
   debug(107 downto 105)         <= header.data_len;
   debug(108)                    <= i2c_fail;
   debug(109)                    <= i2c_fail_cnt( 0);
   debug(110)                    <= i2c_fail_cnt( 1);
   debug(111)                    <= i2c_fail_cnt( 2);
   debug(112)                    <= i2c_tx_rdy;
   debug(113)                    <= i2c_tx_ack;
   debug(121 downto 114)         <= i2c_tx_data;
   debug(122)                    <= i2c_tx_start;
   debug(123)                    <= i2c_tx_stop;
   debug(127 downto 124)         <= (others => '0');

   ---------------------------------------------------------------------------
   -- ROM 
   ---------------------------------------------------------------------------
   rom_inst: rom
   generic map
   (
      c_storage                   => c_storage
   )
   port map
    (
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_clk                       => i_clk,
      ------------------------------------------------------------------------
      -- Data interface
      ------------------------------------------------------------------------
      i_addr                      => req_index,
      o_data                      => header
   );

   ---------------------------------------------------------------------------
   -- single FIFO
   ---------------------------------------------------------------------------
   fifo_wr_en                     <= i_trig;
   fifo_wr_data                   <= i_irq & i_rw & "0" & i_wr_index & i_wr_data;

   --ML84 added for microsemi support
   smartfusion_fifo_gen : if c_fpga = "smartfusion" generate

       smartfusion_fifo_32x48_inst : smartfusion_fifo_32x48
       port map( 
           din   => fifo_wr_data,
           dout  => fifo_rd_data,
           wr_en => fifo_wr_en,
           rd_en => fifo_rd_en_x,
           wclk  => i_clk,
           rclk  => i_clk,
           full  => fifo_wr_full,
           empty => fifo_rd_empty_x,
           srst  => i_rst
       );
   --fifo is not FWFT. additional logic needed:
   FWFT : process(i_clk)
   begin
       if rising_edge(i_clk) then
            if i_rst = '1' then
                fifo_rd_empty <= '1';
            else
                if fifo_rd_en_x = '1' then
                    --whenever data is read from FIFO
                    fifo_rd_empty <= '0';
                elsif fifo_rd_en = '1' then
                    --only when data is read from output register and not from fifo
                    fifo_rd_empty <= '1';
                end if;
            end if;
       end if;
   end process;
   --read fifo when data available and:
   --     out register empty
   --     or out register full and new data is being requested
   fifo_rd_en_x <= (not fifo_rd_empty_x) and (fifo_rd_empty or fifo_rd_en);

   end generate;

   virtex5_fifo_128x48_gen: if c_fpga = "virtex5" generate

      virtex5_fifo_128x48_inst: virtex5_fifo_128x48
      port map
      (
         clk                      => i_clk,
         srst                     => i_rst,
         wr_en                    => fifo_wr_en,
         din                      => fifo_wr_data,
         full                     => fifo_wr_full,
         rd_en                    => fifo_rd_en,
         dout                     => fifo_rd_data,
         empty                    => fifo_rd_empty
      );

   end generate;

   spartan3a_fifo_128x48_gen: if c_fpga = "spartan3a" generate
  
      spartan3a_fifo_128x48_inst: spartan3a_fifo_128x48
      port map
      (
         clk                      => i_clk,
         srst                     => i_rst,
         wr_en                    => fifo_wr_en,
         din                      => fifo_wr_data,
         full                     => fifo_wr_full,
         rd_en                    => fifo_rd_en,
         dout                     => fifo_rd_data,
         empty                    => fifo_rd_empty
      );

  end generate;

   --advance FIFO after serving single requests
   fifo_rd_en                     <= '1' when ((state = done) and (req_periodic = '0')) else '0';
   req_wr_data                    <= fifo_rd_data(31 downto  0); --write only through FIFO interface
   req_pending                    <= not fifo_rd_empty; --data available in FIFO

   --Single mode: get parameters from FIFO
   --Periodic mode: read-only, ROM index from counter
   req_index_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         case (state) is
         when single =>
            req_periodic          <= '0';
            req_irq               <= fifo_rd_data(47 downto 44);
            req_rw                <= fifo_rd_data(43);
            req_index             <= fifo_rd_data(41 downto 32);
         when periodic =>
            req_periodic          <= '1';
            req_irq               <= X"0";
            req_rw                <= '1';
            req_index             <= std_logic_vector(update_cnt( 9 downto  0));
         when others =>
            null;
         end case;
      end if;
   end process req_index_proc;

   o_wr_empty                     <= fifo_rd_empty;
   o_wr_full                      <= fifo_wr_full;

   --ML84 added status output
   o_busy     <= '0' when state = idle else '1';
   o_periodic <= req_periodic;
   ---------------------------------------------------------------------------
   -- Response BRAM (store data read from I2C)
   ---------------------------------------------------------------------------
   bram_wr_en                     <= '1' when (state = read_req_11) else '0';
   bram_wr_addr                   <= req_index; --address is same as ROM entry ID
   bram_wr_data                   <= rx_data;

   --ML84 added microsemi support
   smartfusion_ram_gen: if c_fpga = "smartfusion" generate

       smartfusion_ram_64x32_inst : smartfusion_ram_64x32
       port map( 
           dina  => bram_wr_data,
           douta => open,
           dinb  => LOW32,
           doutb => bram_rd_data,
           addra => bram_wr_addr(5 downto 0),
           addrb => bram_rd_addr(5 downto 0),
           wea_n => bram_wr_en_n, --warning 0=write, 1=read
           web_n => HIGH, --warning 0=write, 1=read
           ena   => HIGH,
           enb   => HIGH,
           clka  => i_clk,
           clkb  => i_clk
       );

       bram_wr_en_n <= not bram_wr_en;

   end generate;

   virtex5_bram_1024x32_gen: if c_fpga = "virtex5" generate
  
      virtex5_bram_1024x32_inst: virtex5_bram_1024x32
      port map
      (
         clka                     => i_clk,
         wea( 0)                  => bram_wr_en,
         addra                    => bram_wr_addr,
         dina                     => bram_wr_data,
         douta                    => open,
         clkb                     => i_clk,
         web( 0)                  => LOW,
         addrb                    => bram_rd_addr,
         dinb                     => LOW32,
         doutb                    => bram_rd_data
      );

   end generate;

   spartan3a_bram_1024x32_gen: if c_fpga = "spartan3a" generate
  
      spartan3a_bram_1024x32_inst: spartan3a_bram_1024x32
      port map
      (
         clka                     => i_clk,
         wea( 0)                  => bram_wr_en,
         addra                    => bram_wr_addr,
         dina                     => bram_wr_data,
         douta                    => open,
         clkb                     => i_clk,
         web( 0)                  => LOW,
         addrb                    => bram_rd_addr,
         dinb                     => LOW32,
         doutb                    => bram_rd_data
      );

  end generate;

   bram_rd_addr                   <= i_rd_index;
   o_rd_data                      <= bram_rd_data;

   ---------------------------------------------------------------------------
   -- Automatic update interface
   ---------------------------------------------------------------------------
   update_cnt_proc: process(i_clk)
   begin
       if rising_edge(i_clk) then
           if i_rst = '1' then
               update <= "00";
               update_cnt <= unsigned(c_num_to_read);
           else
               update                   <= update( 0) & i_update;
               if (update = "01") then
                   update_cnt            <= X"000";
               else
                 --if ((state = periodic) and (update_cnt /= X"200")) then
                   if ((state = periodic) and (update_cnt /= unsigned(c_num_to_read))) then --ML84
                       update_cnt         <= update_cnt + X"001";
                   end if;
               end if;
           end if;
       end if;
   end process update_cnt_proc;

   ---------------------------------------------------------------------------
   -- Transmission fault counter
   ---------------------------------------------------------------------------
   i2c_fail_cnt_proc: process(i_clk)
   begin
      if rising_edge(i_clk) then
         case state is
         when idle =>
            i2c_fail_cnt          <= X"0";
         when write_req_1 =>
            if ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
               i2c_fail_cnt       <= i2c_fail_cnt + X"1";
            end if;
         when others =>
            null;
         end case;
      end if;
   end process i2c_fail_cnt_proc;

   ---------------------------------------------------------------------------
   -- FSM
   ---------------------------------------------------------------------------
   state_proc: process(i_clk)
   begin
       if rising_edge(i_clk) then
           if i_rst = '1' then
               state <= idle;
           else
           case state is
               when idle =>
                   if ((req_pending = '1') and (i2c_idle = '1') and (i2c_busbusy = '0')) then
                       state              <= single;
                   --elsif (update_cnt /= X"200") then
                   elsif (update_cnt /= unsigned(c_num_to_read)) then --ML84
                       state              <= periodic;
                   end if;
               when single =>
                   state                 <= single_read;
               when single_read =>
                   state                 <= header_check;
               when periodic =>
                   state                 <= periodic_read;
               when periodic_read =>
                   state                 <= periodic_check;
               when periodic_check =>
                   if (header.rep = '1') then
                       state              <= header_check;
                   else
                       state              <= idle;
                   end if;
               when header_check =>
                   if (header.dev( 6 downto  0)  = "0000000") then
                       state              <= done;
                   else
                       if (req_rw = '1') then -- read/write
                           if ((header.reg_len = "01") or (header.reg_len = "10")) then
                               state        <= read_req_0;
                           else
                               state        <= read_req_3;
                           end if;
                       else
                           state           <= write_req_0;
                       end if;
                   end if;
               when read_req_0 => -- Send I2C dev address with bit0 set to write ('0')
                   i2c_tx_rdy            <= '1';
                   i2c_tx_data           <= header.dev( 6 downto  0) & '0';
                   i2c_tx_start          <= '1';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       case (header.reg_len) is
                           when "10" =>
                               state           <= read_req_1;
                           when "01" =>
                               state           <= read_req_2;
                           when others =>
                               state           <= read_req_3;
                       end case;
                   end if;
               when read_req_1 => -- Send I2C register address bit 15 downto  8
                   i2c_tx_data           <= header.reg(15 downto  8);
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       state              <= read_req_2;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= done;
                   end if;
               when read_req_2 => -- Send I2C register address bit  7 downto  0
                   i2c_tx_data           <= header.reg( 7 downto  0);
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '1';
                   if (i2c_tx_ack = '1') then
                       i2c_tx_rdy         <= '0';
                       state              <= read_req_3;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= done;
                   end if;
               when read_req_3 => -- Wait for ready bus
                   if ((i2c_idle = '1') and (i2c_busbusy = '0')) then
                       if (i2c_fail = '1') or (header.data_len = "000") then
                           state           <= done;
                       else
                           state           <= read_req_4;
                       end if;
                   end if;
               when read_req_4 => -- Send I2C dev address with bit0 set to read ('1')
                   i2c_tx_rdy            <= '1';
                   i2c_tx_data           <= header.dev( 6 downto  0) & '1';
                   i2c_tx_start          <= '1';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       state              <= read_req_5;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= done;
                   end if;
               when read_req_5 => -- Set read length in bytes 3 ... 1
                   i2c_tx_data           <= "00000" & header.data_len;
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '0';
                   rx_data               <= X"00000000";
                   if (i2c_tx_ack = '1') then
                       i2c_tx_rdy         <= '0';
                       case (header.data_len) is
                           when "100" =>
                               state           <= read_req_6;
                           when "011" =>
                               state           <= read_req_7;
                           when "010" =>
                               state           <= read_req_8;
                           when "001" =>
                               state           <= read_req_9;
                           when others =>
                               state           <= read_req_10;
                       end case;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= done;
                   end if;
               when read_req_6 => -- Receive I2C register address bit 31 downto 24
                   if (i2c_rx_valid = '1') then
                       rx_data(31 downto 24) <= i2c_rx_data;
                       state                 <= read_req_7;
                   end if;
               when read_req_7 => -- Receive I2C register address bit 23 downto 16
                   if (i2c_rx_valid = '1') then
                       rx_data(23 downto 16) <= i2c_rx_data;
                       state                 <= read_req_8;
                   end if;
               when read_req_8 => -- Receive I2C register address bit 15 downto  8
                   if (i2c_rx_valid = '1') then
                       rx_data(15 downto  8) <= i2c_rx_data;
                       state                 <= read_req_9;
                   end if;
               when read_req_9 => -- Receive I2C register address bit  7 downto  0
                   if (i2c_rx_valid = '1') then
                       rx_data( 7 downto  0) <= i2c_rx_data;
                       state                 <= read_req_10;
                   end if;
               when read_req_10 =>
                   if ((i2c_idle = '1') and (i2c_busbusy = '0')) then
                       state              <= read_req_11;
                   end if;
               when read_req_11 =>
                   state                 <= done;

               when write_req_0 => -- Send I2C dev address with bit0 set to write ('0')
                   i2c_tx_rdy            <= '1';
                   i2c_tx_data           <= header.dev( 6 downto  0) & '0';
                   i2c_tx_start          <= '1';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       case (header.reg_len) is
                           when "10" =>
                               state           <= write_req_1;
                           when "01" =>
                               state           <= write_req_2;
                           when others =>
                               case (header.data_len) is
                                   when "100" =>
                                       state        <= write_req_3;
                                   when "011" =>
                                       state        <= write_req_4;
                                   when "010" =>
                                       state        <= write_req_5;
                                   when "001" =>
                                       state        <= write_req_6;
                                   when others =>
                                       state        <= write_req_7;
                               end case;
                       end case;
                   end if;
               when write_req_1 => -- Send I2C register address bit 15 downto  8
                   i2c_tx_data           <= header.reg(15 downto  8);
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       state              <= write_req_2;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= write_req_0;
                   end if;
               when write_req_2 => -- Send I2C register address bit  7 downto  0
                   i2c_tx_data           <= header.reg( 7 downto  0);
                   i2c_tx_start          <= '0';
                   if (header.data_len = "000") then
                       i2c_tx_stop        <= '1';
                   else
                       i2c_tx_stop        <= '0';
                   end if;
                   if (i2c_tx_ack = '1') then
                       case (header.data_len) is
                           when "100" =>
                               state           <= write_req_3;
                           when "011" =>     
                               state           <= write_req_4;
                           when "010" =>     
                               state           <= write_req_5;
                           when "001" =>     
                               state           <= write_req_6;
                           when others =>    
                               state           <= write_req_7;
                       end case;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= write_req_0;
                   end if;
               when write_req_3 => -- Send I2C data bit 31 downto 24
                   i2c_tx_data           <= req_wr_data(31 downto 24);
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       state              <= write_req_4;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= write_req_0;
                   end if;
               when write_req_4 => -- Send I2C data bit 23 downto 16
                   i2c_tx_data           <= req_wr_data(23 downto 16);
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       state              <= write_req_5;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= write_req_0;
                   end if;
               when write_req_5 => -- Send I2C data bit 15 downto  8
                   i2c_tx_data           <= req_wr_data(15 downto  8);
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '0';
                   if (i2c_tx_ack = '1') then
                       state              <= write_req_6;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= write_req_0;
                   end if;
               when write_req_6 => -- Send I2C data bit  7 downto  0
                   i2c_tx_data           <= req_wr_data( 7 downto  0);
                   i2c_tx_start          <= '0';
                   i2c_tx_stop           <= '1';
                   if (i2c_tx_ack = '1') then
                       state              <= write_req_7;
                   elsif ((i2c_fail = '1') and (i2c_fail_cnt /= X"4")) then
                       state              <= write_req_0;
                   end if;
               when write_req_7 =>
                   i2c_tx_rdy            <= '0';
                   if ((i2c_idle = '1') and (i2c_busbusy = '0')) then
                       state              <= done;
                   end if;

               when done =>
                   state                 <= idle;

               when others =>
                   state                 <= idle;
           end case;
           end if;
       end if;
   end process state_proc;

   ---------------------------------------------------------------------------
   -- Issue interrupt on finishing this command (whether ok of faulty)
   ---------------------------------------------------------------------------
   o_int                          <= req_irq when (state = done) else X"0";

   ---------------------------------------------------------------------------
   -- I2C
   ---------------------------------------------------------------------------
   i2c_link_v2_inst: i2c_link_v4
   generic map
   (
      c_flnk_bit_clk_div          => c_i2c_clk_div
   )
   port map
   (
      ------------------------------------------------------------------------
      -- System
      ------------------------------------------------------------------------
      i_rst                       => i_rst, --ml84 added
      i_clk                       => i_clk,
      ------------------------------------------------------------------------
      -- Parallel link interface
      ------------------------------------------------------------------------
      i_i2c_clk_divider           => i2c_clk_divider,
      o_i2c_idle                  => i2c_idle,
      o_i2c_fail                  => i2c_fail,
      o_i2c_busbusy               => i2c_busbusy,
      i_i2c_tx_rdy                => i2c_tx_rdy,
      o_i2c_tx_ack                => i2c_tx_ack,
      i_i2c_tx_data               => i2c_tx_data,
      i_i2c_tx_start              => i2c_tx_start,
      i_i2c_tx_stop               => i2c_tx_stop,
      o_i2c_rx_data               => i2c_rx_data,
      o_i2c_rx_valid              => i2c_rx_valid,
      ------------------------------------------------------------------------
      -- Serial link interface
      ------------------------------------------------------------------------
      o_i2c_scl_tx                => i2c_scl_tx,
      i_i2c_scl_rx                => i2c_scl_rx,
      o_i2c_sda_tx                => i2c_sda_tx,
      i_i2c_sda_rx                => i2c_sda_rx
   );

   --ML84
   I2C_SWITCH_P : process(i_clk)
   begin
       if rising_edge(i_clk) then
            if unsigned(req_index) < unsigned(i2c_start_id1) then
                i2c_sel <= 0;
            else
                i2c_sel <= 1;
            end if;
        end if;
    end process;

   i2c_scl_rx <= i_i2c_scl_rx(i2c_sel);
   i2c_sda_rx <= i_i2c_sda_rx(i2c_sel);

   o_i2c_sda_tx(0) <= i2c_sda_tx when (i2c_sel = 0) else '1';
   o_i2c_scl_tx(0) <= i2c_scl_tx when (i2c_sel = 0) else '1';

   o_i2c_sda_tx(1) <= i2c_sda_tx when (i2c_sel = 1) else '1';
   o_i2c_scl_tx(1) <= i2c_scl_tx when (i2c_sel = 1) else '1';

end behavioral;

------------------------------------------------------------------------------
-- End of file
------------------------------------------------------------------------------

-- APB_master.vhd.vhd

---------------------------------------------------------------------------------------------
library IEEE; 
use IEEE.std_logic_1164.all;

library synplify;
use synplify.attributes.all;

entity APB_master is
generic(
    WATCHDOG_EN : std_logic := '0';
    TIMEOUT : natural := 5000000
);
port(
    --control interface
    master_rst   : in  std_logic;
    master_start : in  std_logic;
    master_write : in  std_logic;
    master_addr  : in  std_logic_vector(31 downto 0);
    master_wdata : in  std_logic_vector(31 downto 0);
    master_rdata : out std_logic_vector(31 downto 0);
    master_busy  : out std_logic;
    master_done  : out std_logic;
    master_error : out std_logic;
    --APB3 MASTER (I2C access)
    PCLK      : in  std_logic;
    m_PRESETn : in  std_logic;
    m_PSELx   : out std_logic := '0';
    m_PENABLE : out std_logic := '0';
    m_PWRITE  : out std_logic := '0';
    m_PADDR   : out std_logic_vector(31 downto 0) := X"00000000";
    m_PWDATA  : out std_logic_vector(31 downto 0) := X"00000000";
    m_PRDATA  : in  std_logic_vector(31 downto 0);
    m_PREADY  : in  std_logic;
    m_PSLVERR : in  std_logic
);
end entity APB_master;

architecture rtl of APB_master is

attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of rtl : architecture is "tmr";

--APB master process
type m_s_t is (M_IDLE, M_SETUP, M_ACCESS);
signal m_s : m_s_t := M_IDLE;

signal wd_count : natural;
signal tout, wd_rst : std_logic;

begin

WD_GEN: if WATCHDOG_EN = '1' generate
    WD_CNT_P : process(PCLK) 
    begin
        if rising_edge(PCLK) then
            if wd_rst = '1' or m_PRESETn = '0' or master_rst = '1' then
                wd_count <= TIMEOUT;
                tout <= '0';
            else
                if wd_count = 0 then
                    wd_count <= TIMEOUT;
                    tout <= '1';
                else
                    wd_count <= wd_count-1;
                    tout <= '0';
                end if;
            end if;
        end if;
    end process;
end generate;

WD_GEN_N: if WATCHDOG_EN = '0' generate
    tout <= '0';
end generate;

--This process performs a single APB transaction.
--To do a WRITE transaction:
--  write address to master_addr
--  write data to master_wdata
--  write 1 to master_write
--  write 1 to master_start
--  wait until master_done = 1 (lasts only 1 clock cycle)
--To do a READ transaction:
--  write address to master_addr
--  write 0 to master_write
--  write 1 to master_start
--  read data from master_rdata when master_done = 1 (lasts only 1 clock cycle)
APB_MASTER_P : process(PCLK)
begin
    if rising_edge(PCLK) then
        if m_PRESETn = '0' or tout = '1' or master_rst = '1' then
            m_PSELx     <= '0';
            m_PENABLE   <= '0';
            master_done <= '0';
            --m_PWRITE  <= '0';
            --m_PADDR   <= (others => '0');
            --m_PWDATA  <= (others => '0');
            m_s <= M_IDLE;
        else
            case m_s is
            when M_IDLE =>
                master_done <= '0';
                if master_start = '1' then
                    m_PSELx  <= '1';
                    m_PWRITE <= master_write;
                    m_PADDR  <= master_addr;
                    m_PWDATA <= master_wdata; --do always, does not interfere with read
                    m_s <= M_SETUP;
                end if;
            when M_SETUP =>
                master_done <= '0';
                m_PENABLE <= '1';
                m_s <= M_ACCESS;
            when M_ACCESS =>
                if m_PREADY = '1' then
                    master_done <= '1';
                    master_error <= m_PSLVERR;
                    if master_write = '0' then
                        master_rdata <= m_PRDATA;
                    end if;
                    m_PSELx     <= '0';
                    m_PENABLE   <= '0';
                    m_s         <= M_IDLE;
                end if;
            when others => 
                master_done <= '0';
                m_PSELx   <= '0';
                m_PENABLE <= '0';
                m_s <= M_IDLE;
            end case;
        end if; --reset
    end if; --clock
end process;

wd_rst      <= '1' when m_s = M_IDLE else '0'; --reset watchdog when FSM goes back to idle state
master_busy <= not wd_rst; --busy when FSM not IDLE 

end architecture rtl; --of APB_master

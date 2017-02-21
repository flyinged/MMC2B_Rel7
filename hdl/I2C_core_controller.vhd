-- I2C_core_controller.vhd

-------------------------------------------------------------------------------------------------------
library IEEE; 
use IEEE.std_logic_1164.all;

library synplify;
use synplify.attributes.all;

--Performs APB bus accesses on I2C core according to required action
--Actions are specified by a code on COMMAND and optional data on WDATA. 
--Actions are started with EXECUTE, only when BUSY=0.
--Command codes:
--   CMD_RESET_CTRL     : X"0", write 0x00 to control register
--   CMD_WRITE_ENS1     : X"1", write wdata(0) to the enable bit
--   CMD_WRITE_STA      : X"2", write wdata(0) to the start bit
--   CMD_WRITE_STO      : X"3", write wdata(0) to the stop bit
--   CMD_WRITE_SI       : X"4", write wdata(0) to the interrupt bit (should be 0)
--   CMD_WRITE_AA       : X"5", write wdata(0) to the acknowledge control bit
--   CMD_WRITE_CLK_DIV  : X"6", write wdata(2:0) to the CR2, CR1 and CR0 bits (clock divider)
--   CMD_WRITE_DATA     : X"7", write wdata to the data register
--   CMD_READ_DATA      : X"8", read data register and show the result on rdata
--   CMD_READ_STAT      : X"9", read stat register and show the result on rdata
entity I2C_core_controller is
generic(
    I2C_CORE_BASE : std_logic_vector(31 downto 0) --:= X"40050020" --BASE + (CHAN<<5)
);
port(
    PCLK      : in  std_logic;
    s_PRESETn : in  std_logic;
    --control interface
    execute   : in  std_logic;
    command   : in  std_logic_vector(3 downto 0);
    wdata     : in  std_logic_vector(7 downto 0);
    rdata     : out std_logic_vector(7 downto 0);
    done      : out std_logic;
    busy      : out std_logic;
    --APB_master interface
    master_start : out std_logic;
    master_write : out std_logic;
    master_addr  : out std_logic_vector(31 downto 0);
    master_wdata : out std_logic_vector(31 downto 0);
    master_rdata : in  std_logic_vector(31 downto 0);
    master_busy  : in  std_logic;
    master_done  : in  std_logic
);
end entity I2C_core_controller;


architecture rtl of I2C_core_controller is

--constant I2C_CORE_BASE : std_logic_vector(31 downto 0) := X"40050020"; --BASE + (CHAN<<5)

attribute syn_radhardlevel : string;
--attribute syn_radhardlevel of rtl : architecture is "tmr";

--I2C control process
type i_s_t is (IDLE, WAIT_DONE);
signal i_s, i_ns : i_s_t;

--i2c core addresses
constant I2C_CTRL_ADDR  : std_logic_vector(31 downto 0) := I2C_CORE_BASE(31 downto 5) & "00000"; --control register
constant I2C_ST_ADDR  : std_logic_vector(31 downto 0) := I2C_CORE_BASE(31 downto 5) & "00100"; --status register
constant I2C_DATA_ADDR  : std_logic_vector(31 downto 0) := I2C_CORE_BASE(31 downto 5) & "01000"; --data register
constant I2C_ADDR0_ADDR : std_logic_vector(31 downto 0) := I2C_CORE_BASE(31 downto 5) & "01100"; --primary slave address register
constant I2C_SMB_ADDR   : std_logic_vector(31 downto 0) := I2C_CORE_BASE(31 downto 5) & "10000"; --smbus control register
constant I2C_ADDR1_ADDR : std_logic_vector(31 downto 0) := I2C_CORE_BASE(31 downto 5) & "11100"; --secondary slave address register

--control register bit positions
constant CR_DIV2 : natural := 7;
constant CR_ENS1 : natural := 6;
constant CR_STA  : natural := 5;
constant CR_STO  : natural := 4;
constant CR_SI   : natural := 3;
constant CR_AA   : natural := 2;
constant CR_DIV1 : natural := 1;
constant CR_DIV0 : natural := 0;

--commands
constant CMD_RESET_CTRL     : std_logic_vector(3 downto 0) := X"0";
constant CMD_WRITE_ENS1     : std_logic_vector(3 downto 0) := X"1";
constant CMD_WRITE_STA      : std_logic_vector(3 downto 0) := X"2";
constant CMD_WRITE_STO      : std_logic_vector(3 downto 0) := X"3";
constant CMD_WRITE_SI       : std_logic_vector(3 downto 0) := X"4";
constant CMD_WRITE_AA       : std_logic_vector(3 downto 0) := X"5";
constant CMD_WRITE_CLK_DIV  : std_logic_vector(3 downto 0) := X"6";
constant CMD_WRITE_DATA     : std_logic_vector(3 downto 0) := X"7";
constant CMD_READ_DATA      : std_logic_vector(3 downto 0) := X"8";
constant CMD_READ_STAT      : std_logic_vector(3 downto 0) := X"9";

begin

master_wdata(31 downto 8) <= (others => '0');

busy <= '0' when (master_busy = '0') and (i_s = IDLE) else '1';

--the following process initializes the I2C core channel 2 and reads the temperature from a TMP112 sensor
I2C_CTRL_P : process(PCLK)
    variable i2c_ctrl : std_logic_vector(7 downto 0);
    variable cmd_good : std_logic;
begin
    if rising_edge(PCLK) then
        if s_PRESETn = '0' then --slave reset: same used for I2C core
            i2c_ctrl     := X"00";
            done         <= '0';
            master_start <= '0';
            master_wdata(7 downto 0) <= (others => '0');
            i_s          <= IDLE;
        else
            case i_s is
            when IDLE =>
                done <= '0';
                --accept commands only when master_busy = '0';
                if execute = '1' and master_busy = '0' then
                    case command is
                    when CMD_RESET_CTRL => --reset ctrl register
                        i2c_ctrl                 := X"00";
                        master_wdata(7 downto 0) <= i2c_ctrl;
                        master_write <= '1';
                        master_addr  <= I2C_CTRL_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_WRITE_ENS1 => --channel enable
                        i2c_ctrl(CR_ENS1) := wdata(0); 
                        i2c_ctrl(CR_SI)   := '1'; --avoid unintentionally resetting IRQ
                        master_wdata(7 downto 0) <= i2c_ctrl;
                        master_write <= '1';
                        master_addr  <= I2C_CTRL_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_WRITE_STA => --start
                        i2c_ctrl(CR_STA) := wdata(0);
                        i2c_ctrl(CR_SI)  := '1'; --avoid unintentionally resetting IRQ
                        master_wdata(7 downto 0) <= i2c_ctrl;
                        master_write <= '1';
                        master_addr  <= I2C_CTRL_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_WRITE_STO => --stop
                        i2c_ctrl(CR_STO) := wdata(0); --will be always reset in WAIT_DONE state
                        i2c_ctrl(CR_SI)  := '1'; --avoid unintentionally resetting IRQ
                        master_wdata(7 downto 0) <= i2c_ctrl;
                        master_write <= '1';
                        master_addr  <= I2C_CTRL_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_WRITE_SI => --IRQ bit
                        i2c_ctrl(CR_SI) := wdata(0);                
                        master_wdata(7 downto 0) <= i2c_ctrl;
                        master_write <= '1';
                        master_addr  <= I2C_CTRL_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_WRITE_AA => --ack control bit
                        i2c_ctrl(CR_AA) := wdata(0);                
                        i2c_ctrl(CR_SI) := '1'; --avoid unintentionally resetting IRQ
                        master_wdata(7 downto 0) <= i2c_ctrl;                
                        master_write <= '1';
                        master_addr  <= I2C_CTRL_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_WRITE_CLK_DIV => --clock divider
                        i2c_ctrl(CR_DIV2) := wdata(2);
                        i2c_ctrl(CR_DIV1) := wdata(1);
                        i2c_ctrl(CR_DIV0) := wdata(0);
                        i2c_ctrl(CR_SI)   := '1'; --avoid unintentionally resetting IRQ
                        master_wdata(7 downto 0) <= i2c_ctrl;         
                        master_write <= '1';
                        master_addr  <= I2C_CTRL_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_WRITE_DATA => --write data register
                        master_wdata(7 downto 0) <= wdata;                
                        master_write <= '1';
                        master_addr  <= I2C_DATA_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_READ_DATA => --read data register
                        master_write <= '0';
                        master_addr  <= I2C_DATA_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when CMD_READ_STAT => --read data register
                        master_write <= '0';
                        master_addr  <= I2C_ST_ADDR;
                        master_start <= '1';
                        i_s          <= WAIT_DONE;
                    when others =>
                        --do nothing on bad command
                    end case;
                end if;

            when WAIT_DONE =>
                master_start <= '0';
                i2c_ctrl(CR_STO) := '0'; --CR_STO is reset automatically
                if master_done = '1' then
                    i_s   <= IDLE; 
                    rdata <= master_rdata(7 downto 0);
                    done  <= '1';
                end if;

            when others =>
                master_start <= '0';
                i_s          <= IDLE;
            end case;
        end if; --reset
    end if; --clock
end process;

end architecture rtl; --of I2C_core_controller
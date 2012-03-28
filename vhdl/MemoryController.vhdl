--
-- Copyright (C) 2010 Chris McClelland
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--  
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
--
library ieee;

use ieee.numeric_std.all;
use ieee.std_logic_1164.all;

entity MemoryController is
	port(
		-- Memory controller interface signals
		mcClk_in      : in    std_logic;
		mcReset_in    : in    std_logic;
		mcAddr_in     : in    std_logic_vector(22 downto 0);
		mcData_in     : in    std_logic_vector(15 downto 0);
		mcData_out    : out   std_logic_vector(15 downto 0);
		mcRequest_in  : in    std_logic;
		mcRW_in       : in    std_logic;
		mcBusy_out    : out   std_logic;

		-- RAM signals routed outside the FPGA
		ramClk_out    : out   std_logic;
		ramAddr_out   : out   std_logic_vector(22 downto 0);
		ramData_io    : inout std_logic_vector(15 downto 0);
		ramADV_out    : out   std_logic;
		ramCE_out     : out   std_logic;
		ramCRE_out    : out   std_logic;
		ramWE_out     : out   std_logic;
		ramOE_out     : out   std_logic;
		ramWait_in    : in    std_logic
	);
end MemoryController;

architecture Behavioural of MemoryController is

	type MCState is (
		STATE_RESET,
		
		STATE_REQUEST_BCR_WRITE0,  -- Assert ramADV_out, ramCE_out, ramWE & ramCRE_out, drive BCR_INIT on ramAddr_out
		STATE_REQUEST_BCR_WRITE1,  -- Just in case RAM is already in sync mode, wait for ramClk_out rising
		STATE_REQUEST_BCR_WRITE2,  -- Deassert ramADV_out to clock in BCR_INIT, keep asserting ramCE_out, ramWE & ramCRE_out
		STATE_WAIT_BCR_WRITE,      -- 2x INIT states plus 7x this state gives 90ns, which meets tCW>85ns
		STATE_FINISH_BCR_WRITE,    -- READ operation must start on a rising ramClk_out edge, so insert wait
		                           -- state if necessary
		STATE_PREPARE_READ,
		STATE_REQUEST_READ,
		STATE_WAIT_READ,
		STATE_FINISH_READ,
		
		STATE_PREPARE_WRITE,
		STATE_REQUEST_WRITE,
		STATE_WAIT_WRITE,
		STATE_FINISH_WRITE,
		
		STATE_IDLE
	);
	signal state        : MCState;
	signal state_next   : MCState;
	signal count        : unsigned(3 downto 0);
	signal count_next   : unsigned(3 downto 0);
	signal ramData_out  : std_logic_vector(15 downto 0);
	signal ramData_in   : std_logic_vector(15 downto 0);
	signal dataReg      : std_logic_vector(15 downto 0);
	signal dataReg_next : std_logic_vector(15 downto 0);
	signal ramWE        : std_logic;
	signal ramClk       : std_logic;
	signal ramClk_next  : std_logic;
	constant ADDR_HIZ   : std_logic_vector(22 downto 0) := (others=>'1');
	constant DATA_HIZ   : std_logic_vector(15 downto 0) := (others=>'1');
	constant INIT_BCR   : std_logic_vector(22 downto 0) :=
		  "000"  -- Reserved
		& "10"   -- Select BCR
		& "00"   -- Reserved
		& "0"    -- Synchronous mode
		& "1"    -- Fixed initial latency
		& "011"  -- With four cycles latency, max clock for -70x RAM is 52MHz
		& "0"    -- ramWait_in is active low
		& "0"    -- Reserved
		& "0"    -- Assert ramWait_in during delay (data ready first clock after deasserts)
		& "0"    -- Reserved
		& "0"    -- Reserved
		& "01"   -- 1/2-power drive
		& "1"    -- Burst no wrap
		& "001"; -- Four-word burst length

begin

	-- Map ports
	ramWE_out <= ramWE;  
	ramData_in <= ramData_io;
	ramData_io <=
		ramData_out when ( ramWE = '0' ) else
		(others=>'Z');
	ramClk_out <= ramClk;
	mcData_out <= dataReg;
	ramClk_next <= not ramClk;

	process(mcClk_in, mcReset_in) begin
		if ( mcReset_in = '1' ) then
			state <= STATE_RESET;
			count <= (others=>'0');
			ramClk <= '0';
			dataReg <= x"DEAD";
		elsif ( mcClk_in'event and mcClk_in = '1' ) then
			state <= state_next;
			count <= count_next;
			ramClk <= ramClk_next;
			dataReg <= dataReg_next;
		end if;
	end process;

	-- Each state lasts 10ns @100MHz
	--
	process(
		state, count, ramData_in, ramWait_in, mcRequest_in, mcRW_in,
		ramClk, mcData_in, mcAddr_in, dataReg)
	begin
		ramAddr_out <= ADDR_HIZ;
		ramData_out <= DATA_HIZ;
		ramADV_out <= '1';
		ramCE_out <= '1';
		ramCRE_out <= '0';
		ramWE <= '1';
		ramOE_out <= '1';
		count_next <= count;        -- by default count keeps its value
		dataReg_next <= dataReg;
		mcBusy_out <= '1';

		case state is

			-- 0ns: In RESET: don't come out of reset until the falling edge of ramClk_out, because
			-- we want there to be a rising edge halfway through the BCR write states. This
			-- ensures the BCR write works even if the RAM happens to already be in sync mode.
			when STATE_RESET =>
				count_next <= "0000";
				if ( ramClk = '1' ) then
					-- Move on at the falling edge of ramClk
					state_next <= STATE_REQUEST_BCR_WRITE0;
				else
					-- Stay in this state whilst ramClk_out is high
					state_next <= STATE_RESET;
				end if;

			-- 10ns: Assert ramADV_out, ramCE_out, ramWE & ramCRE_out, drive BCR_INIT on ramAddr_out
			-- RamClk is low for this state
			when STATE_REQUEST_BCR_WRITE0 =>
				ramADV_out <= '0';                 -- tVP: ramADV_out low for >7ns
				ramAddr_out <= INIT_BCR;      -- tAVS: addr valid & ramCRE_out high >5ns before ramADV_out rising
				ramCRE_out <= '1';
				ramCE_out <= '0';                  -- tCW: low min 85ns (9 clks @100MHz)
				ramWE <= '0';                 -- tWP: low min 55ns (6 clks @100MHz)
				count_next <= x"0";
				state_next <= STATE_REQUEST_BCR_WRITE1;

			-- 20ns: Just in case RAM is already in sync mode, hold things stable for a ramClk_out rising
			-- RamClk is high for this state
			when STATE_REQUEST_BCR_WRITE1 =>
				ramADV_out <= '0';                 -- tVP: ramADV_out low for >7ns
				ramAddr_out <= INIT_BCR;      -- tAVS: addr valid & ramCRE_out high >5ns before ramADV_out rising
				ramCRE_out <= '1';
				ramCE_out <= '0';                  -- tCW: low min 85ns (9 clks @100MHz)
				ramWE <= '0';                 -- tWP: low min 55ns (6 clks @100MHz)
				count_next <= x"0";
				state_next <= STATE_REQUEST_BCR_WRITE2;

			-- 30ns: Deassert ramADV_out to clock in BCR_INIT, keep asserting ramCE_out, ramWE & ramCRE_out
			-- RamClk is low for this state
			when STATE_REQUEST_BCR_WRITE2 =>
				ramAddr_out <= INIT_BCR;      -- tAVH: addr valid & ramCRE_out high >2ns after ramADV_out rising
				ramCRE_out <= '1';
				ramCE_out <= '0';                  -- tCW: low min 85ns (9 clks @100MHz)
				ramWE <= '0';                 -- tWP: low min 55ns (6 clks @100MHz)
				count_next <= x"6";          -- Nine states = 2 + (6,5,4,3,2,1,0) = 90ns
				state_next <= STATE_WAIT_BCR_WRITE;

			-- 30ns: 2x REQUEST states plus 7x this state gives 90ns, which meets tCW>85ns
			-- The state entered on ramClk_out falling edge and
			--   left on a rising edge if init count_next even
			--   left on a falling edge if init count_next odd
			when STATE_WAIT_BCR_WRITE =>
				ramCE_out <= '0';   -- tCW: low min 85ns (9 clks @100MHz)
				ramWE <= '0';  -- tWP: low min 55ns (6 clks @100MHz)
				count_next <= count - 1;
				if ( count = "000" ) then
					-- Last wait state before we move on
					if ( ramClk = '0' ) then
						state_next <= STATE_PREPARE_READ;      -- If the count_next init was odd
					else
						state_next <= STATE_FINISH_BCR_WRITE;  -- If the count_next init was even
					end if;
				else
					-- Count not zero yet, so loopback...
					state_next <= STATE_WAIT_BCR_WRITE;
				end if;

			-- OPTIONAL: ramClk_out was high at the end of STATE_WAIT_BCR_WRITE, so
			-- wait for ramClk_out rising edge before starting initial read. This state is only
			-- needed if count_next is initialised to an even number in STATE_REQUEST_BCR_WRITE1.
			-- Bus definitely now in synchronous mode.
			-- RamClk low during this state.
			when STATE_FINISH_BCR_WRITE =>
				state_next <= STATE_PREPARE_READ;
			



			-- Keep ramCE_out disabled, meeting tCBPH>8ns
			-- Bus definitely now in synchronous mode.
			-- RamClk is high during this state
			when STATE_PREPARE_READ =>
				state_next <= STATE_REQUEST_READ;

			-- Initiate sync read of address zero
			-- RamClk has a rising edge half-way through this state
			when STATE_REQUEST_READ =>
				ramAddr_out <= mcAddr_in;      -- tSP: addr valid >3ns before ramClk_out rising
				ramADV_out <= '0';                  -- tHD: addr valid >2ns after ramClk_out rising
				ramCE_out <= '0';
				if ( ramClk = '1' ) then
					state_next <= STATE_WAIT_READ;   -- Move on
				else
					state_next <= STATE_REQUEST_READ;  -- Hold steady after rising edge
				end if;

			-- Keep asserting ramCE_out, assert ramOE_out and spin whilst the RAM asserts ramWait_in
			-- This state entered on ramClk_out falling edge and left on ramClk_out rising
			when STATE_WAIT_READ =>
				ramCE_out <= '0';
				ramOE_out <= '0';
				if ( ramWait_in = '1' and ramClk = '0' ) then
					-- Only proceed if ramWait_in not asserted and ramClk_out is low
					-- ramData_in will be available at the end of this cycle
					dataReg_next <= ramData_in;
					state_next <= STATE_FINISH_READ;
				else
					-- RAM still asserts ramWait_in, so stay in this state
					state_next <= STATE_WAIT_READ;
				end if;

			-- RamClk high during this state
			when STATE_FINISH_READ =>
				--mcBusy_out <= '0';
				ramCE_out <= '0';
				ramOE_out <= '0';
				state_next <= STATE_IDLE;




			-- Deassert ramCE_out & ramOE_out
			when STATE_IDLE =>
				mcBusy_out <= '0';
				if ( ramClk = '0' and mcRequest_in = '1' ) then
					if ( mcRW_in = '1' ) then
						state_next <= STATE_PREPARE_READ;
					else
						state_next <= STATE_PREPARE_WRITE;
					end if;
				else
					state_next <= STATE_IDLE;
				end if;




			-- Keep ramCE_out disabled, meeting tCBPH>8ns
			-- RamClk is high during this state
			when STATE_PREPARE_WRITE =>
				state_next <= STATE_REQUEST_WRITE;

			-- Initiate sync write of address zero
			-- RamClk has a rising edge half-way through this state
			when STATE_REQUEST_WRITE =>
				ramAddr_out <= mcAddr_in;      -- tSP: addr valid >3ns before ramClk_out rising
				ramADV_out <= '0';                  -- tHD: addr valid >2ns after ramClk_out rising
				ramCE_out <= '0';
				ramWE <= '0';
				if ( ramClk = '1' ) then
					state_next <= STATE_WAIT_WRITE;   -- Move on
				else
					state_next <= STATE_REQUEST_WRITE;  -- Hold steady after rising edge
				end if;

			-- Keep asserting ramWE & ramCE_out; drive ramData_io & spin whilst the RAM asserts ramWait_in
			-- This state entered on ramClk_out falling edge and left on ramClk_out rising
			when STATE_WAIT_WRITE =>
				ramCE_out <= '0';
				ramWE <= '0';
				ramData_out <= mcData_in;
				if ( ramWait_in = '1' and ramClk = '0' ) then
					-- Only proceed if ramWait_in not asserted and ramClk_out is low
					-- ramData_out will be written to RAM on the rising edge
					state_next <= STATE_FINISH_WRITE;
				else
					-- RAM still asserts ramWait_in, so stay in this state
					state_next <= STATE_WAIT_WRITE;
				end if;

			-- Hold data there to meet tHD
			-- RamClk will be high for this state
			when STATE_FINISH_WRITE =>
				--mcBusy_out <= '0';
				ramCE_out <= '0';
				ramWE <= '0';
				ramData_out <= mcData_in;
				state_next <= STATE_IDLE;


		end case;
	end process;
end Behavioural;

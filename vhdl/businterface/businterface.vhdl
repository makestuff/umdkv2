--
-- Copyright (C) 2011 Chris McClelland
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

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity businterface is
	port(
		reset_in         : in    std_logic;
		clk_in           : in    std_logic;

		-- External connections
		mdBufOE_out      : out   std_logic;  -- while 'Z', MD is isolated; drive low to enable all three buffers.
		mdBufDir_out     : out   std_logic;  -- drive high when MegaDrive reading, low when MegaDrive writing.
		mdOE_in          : in    std_logic;  -- active low output enable: asserted when MD reads.
		mdLDSW_in        : in    std_logic;  -- active low write enable: asserted when MD writes.
		mdAddr_in        : in    std_logic_vector(20 downto 0);
		mdData_io        : inout std_logic_vector(15 downto 0);

		-- Internal connections
		mdBeginRead_out  : out   std_logic;  -- asserted at the beginning of each relevant read
		mdBeginWrite_out : out   std_logic;  -- asserted at the beginning of each relevant write
		mdAccessMem_out  : out   std_logic;  -- asserted when this cycle is in our mem region
		mdAccessIO_out   : out   std_logic;  -- asserted when this cycle is in our I/O region
		mdSyncOE_out     : out   std_logic;  -- synchronised active-high version of mdOE_in
		mdSyncWE_out     : out   std_logic;  -- synchronised active-high version of mdLDSW_in
		mdData_in        : in    std_logic_vector(15 downto 0)
	);
end entity;

architecture behavioural of businterface is

	type BStateType is (
		BSTATE_IDLE,
		BSTATE_WAIT_READ,
		BSTATE_WAIT_WRITE
	);
	signal bstate       : BStateType;
	signal bstate_next  : BStateType;
	signal mdAccessMem  : std_logic;
	signal mdAccessIO   : std_logic;
	signal mdBeginRead  : std_logic;
	signal mdBeginWrite : std_logic;
	signal mdSyncOE     : std_logic;
	signal mdSyncWE     : std_logic;
	signal mdDriveBus   : std_logic;
	constant IO_BASE : std_logic_vector(20 downto 0) := "1" & x"42600";

begin

	------------------------------------------------------------------------------------------------
	-- All change!
	process(clk_in, reset_in)
	begin
		if ( reset_in = '1' ) then
			bstate <= BSTATE_IDLE;
			mdSyncOE <= '0';
			mdSyncWE <= '0';
		elsif ( clk_in'event and clk_in = '1' ) then
			bstate <= bstate_next;
			mdSyncOE <= not(mdOE_in);
			mdSyncWE <= not(mdLDSW_in);
		end if;
	end process;

	------------------------------------------------------------------------------------------------
	-- Bus interface state machine
	process(
		bstate,
		mdAccessMem,
		mdAccessIO,
		mdSyncOE,
		mdSyncWE
	)
	begin
		bstate_next <= bstate;
		mdBeginRead <= '0';
		mdBeginWrite <= '0';
		case bstate is
			when BSTATE_IDLE =>
				mdDriveBus <= '0';
				if ( mdAccessMem = '1' or mdAccessIO = '1' ) then
					if ( mdSyncOE = '1' ) then
						bstate_next <= BSTATE_WAIT_READ;
						mdBeginRead <= '1';
						mdDriveBus <= '1';
					elsif ( mdSyncWE = '1' ) then
						bstate_next <= BSTATE_WAIT_WRITE;
						mdBeginWrite <= '1';
						mdDriveBus <= '0';
					end if;
				end if;

			when BSTATE_WAIT_READ =>
				mdDriveBus <= '1';
				if ( mdSyncOE = '0' ) then
					bstate_next <= BSTATE_IDLE;
					mdDriveBus <= '0';
				end if;

			when BSTATE_WAIT_WRITE =>
				mdDriveBus <= '0';
				if ( mdSyncWE = '0' ) then
					bstate_next <= BSTATE_IDLE;
				end if;
		end case;
	end process;

	-- Unsynchronised strobes for accesses to memory and I/O
	mdAccessMem <=
		'1' when mdAddr_in(20) = '0'
		else '0';
	mdAccessIO <=
		'1' when mdAddr_in = IO_BASE
		else '0';
	
	-- Drive bus & buffers. Must use same signal for muxing mdData_io as for mdBufDir_out
	mdBufOE_out <= '0';  -- Level converters always on (pulled up during FPGA programming)
	mdBufDir_out <= mdDriveBus;  -- Drive data bus only when MD reading from us.
	mdData_io <=
		mdData_in when mdDriveBus = '1' else
		(others=>'Z');  -- Do not drive data bus
	
	mdAccessMem_out  <= mdAccessMem;
	mdAccessIO_out   <= mdAccessIO;
	mdBeginRead_out  <= mdBeginRead;
	mdBeginWrite_out <= mdBeginWrite;
	mdSyncOE_out     <= mdSyncOE;
	mdSyncWE_out     <= mdSyncWE;

end architecture;

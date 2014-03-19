--
-- Copyright (C) 2012 Chris McClelland
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
--
library ieee;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.mem_ctrl_pkg.all;

entity mem_arbiter is
	port(
		clk_in         : in  std_logic;
		reset_in       : in  std_logic;

		-- Connetion to mem_pipe
		ppReady_out    : out std_logic;
		ppCmd_in       : in  MCCmdType;
		ppAddr_in      : in  std_logic_vector(22 downto 0);
		ppData_in      : in  std_logic_vector(15 downto 0);
		ppData_out     : out std_logic_vector(15 downto 0);
		ppRDV_out      : out std_logic;

		-- Connection to MegaDrive
		mdDriveBus_out : out   std_logic;
		mdReset_in     : in    std_logic;
		mdDTACK_out    : out   std_logic;
		mdAddr_in      : in    std_logic_vector(22 downto 0);
		mdData_io      : inout std_logic_vector(15 downto 0);
		mdOE_in        : in    std_logic;
		mdAS_in        : in    std_logic;
		mdLDSW_in      : in    std_logic;
		mdUDSW_in      : in    std_logic;

		-- Connection to mem_ctrl
		mcAutoMode_out : out std_logic;
		mcReady_in     : in  std_logic;
		mcCmd_out      : out MCCmdType;
		mcAddr_out     : out std_logic_vector(22 downto 0);
		mcData_out     : out std_logic_vector(15 downto 0);
		mcData_in      : in  std_logic_vector(15 downto 0);
		mcRDV_in       : in  std_logic
	);
end entity;

architecture rtl of mem_arbiter is
	type StateType is (
		S_RESET,  -- MD in reset, host has access to SDRAM
		S_IDLE,   -- wait for mdOE_sync to go low when A22='0', indicating a MD cart read
		S_WAIT_READ,
		S_NOP1,
		S_NOP2,
		S_NOP3,
		S_NOP4,
		S_EXEC_REFRESH,
		S_WAIT_REFRESH,
		S_WAIT_MD
	);
	
	-- Registers
	signal state        : StateType := S_RESET;
	signal state_next   : StateType;
	signal dataReg      : std_logic_vector(15 downto 0) := (others => '0');
	signal dataReg_next : std_logic_vector(15 downto 0);

	-- Synchronise mdOE_in to sysClk
	signal mdOE_sync    : std_logic := '1';
	signal mdAddr_sync  : std_logic_vector(22 downto 0) := (others => '0');
	signal mdDriveBus   : std_logic;
	signal mdDriveBus_sync : std_logic;
begin
	-- Infer registers
	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			if ( reset_in = '1' ) then
				state <= S_RESET;
				dataReg <= (others => '0');
				mdAddr_sync <= (others => '0');
				mdOE_sync <= '1';
				mdDriveBus_sync <= '0';
			else
				state <= state_next;
				dataReg <= dataReg_next;
				mdAddr_sync <= mdAddr_in;
				mdOE_sync <= mdOE_in;
				mdDriveBus_sync <= mdDriveBus;
			end if;
		end if;
	end process;

	process(
		state, dataReg, mdDriveBus_sync, mdOE_sync, mdAddr_sync, mcReady_in, ppCmd_in, ppAddr_in, ppData_in, mcData_in, mcRDV_in,
		mdReset_in)
	begin
		-- Local register defaults
		state_next <= state;
		dataReg_next <= dataReg;

		-- Memory controller defaults
		mcAutoMode_out <= '0';  -- don't auto-refresh by default.
		mcCmd_out <= MC_NOP;
		mcAddr_out <= (others => 'X');
		mcData_out <= (others => 'X');

		-- Pipe defaults
		ppData_out <= (others => 'X');
		ppReady_out <= '0';
		ppRDV_out <= '0';

		-- State machine
		case state is
			when S_RESET =>
				-- The mem_ctrl inputs are driven by the mem_pipe outputs.
				mcAutoMode_out <= '1';  -- enable auto-refresh
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;

				-- The mem_pipe inputs are driven by the mem_ctrl outputs.
				ppData_out <= mcData_in;
				ppReady_out <= mcReady_in;
				ppRDV_out <= mcRDV_in;
				
				-- Proceed when the host releases MD from reset.
				if ( mdReset_in = '0' ) then
					state_next <= S_IDLE;
				end if;

			-- Wait until the in-progress read completes, then register the result and proceed.
			when S_WAIT_READ =>
				if ( mcRDV_in = '1' ) then
					state_next <= S_NOP1;
					dataReg_next <= mcData_in;
				end if;

			when S_NOP1 =>
				ppReady_out <= mcReady_in;  -- Do a host I/O cycle if one is pending
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;
				state_next <= S_NOP2;
				
			when S_NOP2 =>
				state_next <= S_NOP3;
				
			when S_NOP3 =>
				state_next <= S_NOP4;
				
			when S_NOP4 =>
				ppData_out <= mcData_in;
				ppRDV_out <= mcRDV_in;
				state_next <= S_EXEC_REFRESH;
				
			-- Start refresh cycle
			when S_EXEC_REFRESH =>
				state_next <= S_WAIT_REFRESH;
				mcCmd_out <= MC_REF;
				
			-- Wait for refresh cycle to complete
			when S_WAIT_REFRESH =>
				if ( mcReady_in = '1' ) then
					if ( mdOE_sync = '1' ) then
						state_next <= S_IDLE;
					else
						state_next <= S_WAIT_MD;
					end if;
				end if;

			when S_WAIT_MD =>
				if ( mdOE_sync = '1' ) then
					state_next <= S_IDLE;
				end if;

			-- S_IDLE & others
			when others =>
				if ( mdDriveBus_sync = '1' ) then
					-- MD is reading from cartridge space
					state_next <= S_WAIT_READ;
					mcCmd_out <= MC_RD;
					mcAddr_out <= mdAddr_sync;
				end if;
				if ( mdReset_in = '1' ) then
					-- MD back in reset, so give host full control again
					state_next <= S_RESET;
				end if;
		end case;
	end process;
	mdDriveBus <=
		'1' when mdOE_in = '0' and mdAddr_in(22) = '0'
		else '0';
	mdData_io <=
		mcData_in when state = S_WAIT_READ and mcRDV_in = '1'
		else dataReg when mdDriveBus_sync = '1'
		else (others => 'Z');
	mdDriveBus_out <= mdDriveBus_sync;
	mdDTACK_out <= '0';  -- for now, just rely on MD's auto-DTACK
end architecture;

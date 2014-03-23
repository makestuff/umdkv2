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
		mcRDV_in       : in  std_logic;

		-- Trace pipe
		traceData_out  : out std_logic_vector(39 downto 0);
		traceValid_out : out std_logic
	);
end entity;

architecture rtl of mem_arbiter is
	type RStateType is (
		R_RESET,  -- MD in reset, host has access to SDRAM
		R_IDLE,   -- wait for mdOE_sync to go low when A22='0', indicating a MD cart read
		R_WAIT_READ,
		R_NOP1,
		R_NOP2,
		R_NOP3,
		R_NOP4,
		R_EXEC_REFRESH,
		R_WAIT_REFRESH,
		R_WAIT_MD
	);
	type MStateType is (
		M_IDLE,
		M_WAIT_READ,
		M_WAIT_END
	);
	
	-- Registers
	signal rstate       : RStateType := R_RESET;
	signal rstate_next  : RStateType;
	signal mstate       : MStateType := M_IDLE;
	signal mstate_next  : MStateType;
	signal dataReg      : std_logic_vector(15 downto 0) := (others => '0');
	signal dataReg_next : std_logic_vector(15 downto 0);
	signal addrReg      : std_logic_vector(21 downto 0) := (others => '0');
	signal addrReg_next : std_logic_vector(21 downto 0);

	-- Synchronise mdOE_in to sysClk
	signal mdOE_sync    : std_logic := '1';
	signal mdAddr_sync  : std_logic_vector(22 downto 0) := (others => '0');
begin
	-- Infer registers
	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			if ( reset_in = '1' ) then
				rstate <= R_RESET;
				mstate <= M_IDLE;
				dataReg <= (others => '0');
				addrReg <= (others => '0');
				mdAddr_sync <= (others => '0');
				mdOE_sync <= '1';
			else
				rstate <= rstate_next;
				mstate <= mstate_next;
				dataReg <= dataReg_next;
				addrReg <= addrReg_next;
				mdAddr_sync <= mdAddr_in;
				mdOE_sync <= mdOE_in;
			end if;
		end if;
	end process;

	-- RAM handler state-machine
	process(
		rstate, dataReg, addrReg, mdOE_sync, mdAddr_sync, mcReady_in, ppCmd_in, ppAddr_in, ppData_in, mcData_in, mcRDV_in,
		mdReset_in)
	begin
		-- Local register defaults
		rstate_next <= rstate;
		dataReg_next <= dataReg;
		addrReg_next <= addrReg;

		-- Memory controller defaults
		mcAutoMode_out <= '0';  -- don't auto-refresh by default.
		mcCmd_out <= MC_NOP;
		mcAddr_out <= (others => 'X');
		mcData_out <= (others => 'X');

		-- Pipe defaults
		ppData_out <= (others => 'X');
		ppReady_out <= '0';
		ppRDV_out <= '0';

		-- Trace defaults
		traceData_out <= (others => 'X');
		traceValid_out <= '0';

		case rstate is
			when R_RESET =>
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
					rstate_next <= R_IDLE;
				end if;

			-- Wait until the in-progress read completes, then register the result and proceed.
			when R_WAIT_READ =>
				if ( mcRDV_in = '1' ) then
					rstate_next <= R_NOP1;
					dataReg_next <= mcData_in;
					traceData_out <= "00" & addrReg & mcData_in;
					traceValid_out <= '1';
				end if;

			when R_NOP1 =>
				ppReady_out <= mcReady_in;  -- Do a host I/O cycle if one is pending
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;
				rstate_next <= R_NOP2;
				
			when R_NOP2 =>
				rstate_next <= R_NOP3;
				
			when R_NOP3 =>
				rstate_next <= R_NOP4;
				
			when R_NOP4 =>
				ppData_out <= mcData_in;
				ppRDV_out <= mcRDV_in;
				rstate_next <= R_EXEC_REFRESH;
				
			-- Start refresh cycle
			when R_EXEC_REFRESH =>
				rstate_next <= R_WAIT_REFRESH;
				mcCmd_out <= MC_REF;
				
			-- Wait for refresh cycle to complete
			when R_WAIT_REFRESH =>
				if ( mcReady_in = '1' ) then
					if ( mdOE_sync = '1' ) then
						rstate_next <= R_IDLE;
					else
						rstate_next <= R_WAIT_MD;
					end if;
				end if;

			when R_WAIT_MD =>
				if ( mdOE_sync = '1' ) then
					rstate_next <= R_IDLE;
				end if;

			-- R_IDLE & others
			when others =>
				if ( mdOE_sync = '0' and mdAddr_sync(22) = '0' ) then
					-- MD is reading from cartridge space
					rstate_next <= R_WAIT_READ;
					mcCmd_out <= MC_RD;
					mcAddr_out <= mdAddr_sync;
					addrReg_next <= mdAddr_sync(21 downto 0);
				end if;
				if ( mdReset_in = '1' ) then
					-- MD back in reset, so give host full control again
					rstate_next <= R_RESET;
				end if;
		end case;
	end process;

	-- MegaDrive bus-handler state-machine
	process(
		mstate, addrReg, dataReg, mdOE_sync, mdAddr_sync(22), mcData_in, mcRDV_in)
	begin
		mstate_next <= mstate;
		mdData_io <= (others => 'Z');
		mdDriveBus_out <= '0';
		
		case mstate is
			when M_WAIT_READ =>
				mdData_io <= mcData_in;
				mdDriveBus_out <= '1';
				if ( mcRDV_in = '1' ) then
					mstate_next <= M_WAIT_END;
				end if;

			when M_WAIT_END =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
				if ( mdOE_sync = '1' ) then
					mstate_next <= M_IDLE;
					mdData_io <= (others => 'Z');
					mdDriveBus_out <= '0';
				end if;

			when M_IDLE =>
				if ( mdOE_sync = '0' and mdAddr_sync(22) = '0' ) then
					mstate_next <= M_WAIT_READ;
				end if;
		end case;
	end process;

	mdDTACK_out <= '0';  -- for now, just rely on MD's auto-DTACK
end architecture;

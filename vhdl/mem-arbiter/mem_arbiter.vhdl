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

		-- Connection to mem_ctrl
		mcAutoMode_out : out std_logic;
		mcReady_in     : in  std_logic;
		mcCmd_out      : out MCCmdType;
		mcAddr_out     : out std_logic_vector(22 downto 0);
		mcData_out     : out std_logic_vector(15 downto 0);
		mcData_in      : in  std_logic_vector(15 downto 0);
		mcRDV_in       : in  std_logic;

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

		-- Trace pipe
		traceEnable_in : in  std_logic;
		traceData_out  : out std_logic_vector(47 downto 0);
		traceValid_out : out std_logic
	);
end entity;

architecture rtl of mem_arbiter is
	type RStateType is (
		R_RESET,  -- MD in reset, host has access to SDRAM
		R_IDLE,   -- wait for mdOE_sync to go low when A22='0', indicating a MD cart read

		-- Owned read
		R_READ_OWNED_WAIT,
		R_READ_OWNED_NOP1,
		R_READ_OWNED_NOP2,
		R_READ_OWNED_NOP3,
		R_READ_OWNED_NOP4,
		R_READ_OWNED_REFRESH,
		R_READ_OWNED_FINISH,

		-- Foreign read
		R_READ_OTHER,

		-- Owned write
		R_WRITE_OWNED_NOP1,
		R_WRITE_OWNED_NOP2,
		R_WRITE_OWNED_NOP3,
		R_WRITE_OWNED_NOP4,
		R_WRITE_OWNED_EXEC,
		R_WRITE_OWNED_FINISH,

		-- Foreign write
		R_WRITE_OTHER_NOP1,
		R_WRITE_OTHER_NOP2,
		R_WRITE_OTHER_NOP3,
		R_WRITE_OTHER_NOP4,
		R_WRITE_OTHER_EXEC,
		R_WRITE_OTHER_FINISH,

		-- Register write
		R_WRITE_REG_NOP1,
		R_WRITE_REG_NOP2,
		R_WRITE_REG_NOP3,
		R_WRITE_REG_NOP4,
		R_WRITE_REG_EXEC,
		R_WRITE_REG_FINISH
	);
	type MStateType is (
		M_IDLE,
		M_READ_WAIT,
		M_END_WAIT
	);
	type BankType is array (0 to 15) of std_logic_vector(4 downto 0);
	
	-- Registers
	signal rstate       : RStateType := R_RESET;
	signal rstate_next  : RStateType;
	signal mstate       : MStateType := M_IDLE;
	signal mstate_next  : MStateType;
	signal dataReg      : std_logic_vector(15 downto 0) := (others => '0');
	signal dataReg_next : std_logic_vector(15 downto 0);
	signal addrReg      : std_logic_vector(22 downto 0) := (others => '0');
	signal addrReg_next : std_logic_vector(22 downto 0);
	signal mdAS         : std_logic;
	signal mdAS_next    : std_logic;
	signal memBank      : BankType := (
		"00000", "00001", "00010", "00011", "00100", "00101", "00110", "00111",
		"01000", "01001", "01010", "01011", "01100", "01101", "01110", "01111"
	);
	signal memBank_next : BankType;

	-- Synchronise MegaDrive signals to sysClk
	signal mdAS_sync    : std_logic := '1';
	signal mdOE_sync    : std_logic := '1';
	signal mdDSW_sync   : std_logic_vector(1 downto 0) := "11";
	signal mdAddr_sync  : std_logic_vector(22 downto 0) := (others => '0');
	signal mdData_sync  : std_logic_vector(15 downto 0) := (others => '0');
	constant TR_RD      : std_logic_vector(1 downto 0) := "11";
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
				mdAS_sync <= '1';
				mdOE_sync <= '1';
				mdDSW_sync <= "11";
				mdData_sync <= (others => '0');
				mdAS <= '1';
				memBank <= (
					"00000", "00001", "00010", "00011", "00100", "00101", "00110", "00111",
					"11111", "01001", "01010", "01011", "01100", "01101", "01110", "01111"
				);
			else
				rstate <= rstate_next;
				mstate <= mstate_next;
				dataReg <= dataReg_next;
				addrReg <= addrReg_next;
				mdAddr_sync <= mdAddr_in;
				mdAS_sync <= mdAS_in;
				mdOE_sync <= mdOE_in;
				mdDSW_sync <= mdUDSW_in & mdLDSW_in;
				mdData_sync <= mdData_io;
				mdAS <= mdAS_next;
				memBank <= memBank_next;
			end if;
		end if;
	end process;

	-- #############################################################################################
	-- ##                           State machine to control the SDRAM                            ##
	-- #############################################################################################
	process(
			rstate, dataReg, addrReg,
			mdOE_sync, mdDSW_sync, mdAddr_sync, mdData_sync, mdAS_sync, mdAS, mdReset_in,
			mcReady_in, mcData_in, mcRDV_in,
			ppCmd_in, ppAddr_in, ppData_in,
			memBank,
			traceEnable_in
		)
		-- Function to generate SDRAM physical address using MD address and memBank (SSF2) regs
		impure function transAddr(addr : std_logic_vector(22 downto 0)) return std_logic_vector is
		begin
			return memBank(to_integer(unsigned(addr(21 downto 18)))) & addr(17 downto 0);
		end function;
	begin
		-- Local register defaults
		rstate_next <= rstate;
		dataReg_next <= dataReg;
		addrReg_next <= addrReg;
		mdAS_next <= mdAS;
		memBank_next <= memBank;

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
			-- -------------------------------------------------------------------------------------
			-- Whilst the MD is in reset, the SDRAM does auto-refresh, and the host has complete
			-- control over it.
			--
			when R_RESET =>
				-- Enable auto-refresh
				mcAutoMode_out <= '1';

				-- Drive mem-ctrl inputs with mem-pipe outputs
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;

				-- Drive mem-pipe inputs with mem-ctrl outputs
				ppData_out <= mcData_in;
				ppReady_out <= mcReady_in;  
				ppRDV_out <= mcRDV_in;

				-- Proceed when host releases MD from reset
				if ( mdReset_in = '0' ) then
					rstate_next <= R_IDLE;
				end if;

			-- -------------------------------------------------------------------------------------
			-- Wait until the in-progress owned read completes, then register the result, send to
			-- the trace FIFO and proceed.
			--
			when R_READ_OWNED_WAIT =>
				if ( mcRDV_in = '1' ) then
					rstate_next <= R_READ_OWNED_NOP1;
					dataReg_next <= mcData_in;
					traceData_out <= "000000" & mdAS & TR_RD & addrReg & mcData_in;
					traceValid_out <= traceEnable_in;
				end if;

			-- Give the host enough time for one I/O cycle, if it wants it.
			--
			when R_READ_OWNED_NOP1 =>
				ppReady_out <= mcReady_in;
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;
				rstate_next <= R_READ_OWNED_NOP2;
			when R_READ_OWNED_NOP2 =>
				rstate_next <= R_READ_OWNED_NOP3;
			when R_READ_OWNED_NOP3 =>
				rstate_next <= R_READ_OWNED_NOP4;
			when R_READ_OWNED_NOP4 =>
				ppData_out <= mcData_in;
				ppRDV_out <= mcRDV_in;
				rstate_next <= R_READ_OWNED_REFRESH;
				
			-- Start a refresh cycle, then wait for it to complete.
			--
			when R_READ_OWNED_REFRESH =>
				rstate_next <= R_READ_OWNED_FINISH;
				mcCmd_out <= MC_REF;
			when R_READ_OWNED_FINISH =>
				if ( mcReady_in = '1' and mdOE_sync = '1' ) then
					rstate_next <= R_IDLE;
				end if;

			-- -------------------------------------------------------------------------------------
			-- Wait for the in-progress foreign read to complete, then send to the trace FIFO and go
			-- back to R_IDLE.
			--
			when R_READ_OTHER =>
				if ( mdOE_sync = '1' ) then
					rstate_next <= R_IDLE;
					traceData_out <= "000000" & mdAS & TR_RD & addrReg & mdData_sync;
					traceValid_out <= traceEnable_in;
				end if;

			-- -------------------------------------------------------------------------------------
			-- An owned write has been requested, but things are not yet stable so give the host
			-- enough time for one I/O cycle, if it wants it - this will provide enough of a delay
			-- for the write masks and data to stabilise.
			--
			when R_WRITE_OWNED_NOP1 =>
				ppReady_out <= mcReady_in;
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;
				rstate_next <= R_WRITE_OWNED_NOP2;
			when R_WRITE_OWNED_NOP2 =>
				rstate_next <= R_WRITE_OWNED_NOP3;
			when R_WRITE_OWNED_NOP3 =>
				rstate_next <= R_WRITE_OWNED_NOP4;
			when R_WRITE_OWNED_NOP4 =>
				ppData_out <= mcData_in;
				ppRDV_out <= mcRDV_in;
				rstate_next <= R_WRITE_OWNED_EXEC;

			-- Now execute the owned write.
			--
			when R_WRITE_OWNED_EXEC =>
				rstate_next <= R_WRITE_OWNED_FINISH;
				traceData_out <= "000000" & mdAS & mdDSW_sync & addrReg & mdData_sync;
				traceValid_out <= traceEnable_in;
				mcCmd_out <= MC_WR;
				mcAddr_out <= transAddr(addrReg);
				mcData_out <= mdData_sync;
			when R_WRITE_OWNED_FINISH =>
				if ( mdDSW_sync = "11" and mcReady_in = '1' ) then
					rstate_next <= R_IDLE;
				end if;

			-- -------------------------------------------------------------------------------------
			-- A foreign write has been requested, but things are not yet stable so give the host
			-- enough time for one I/O cycle, if it wants it - this will provide enough of a delay
			-- for the write masks and data to stabilise.
			--
			when R_WRITE_OTHER_NOP1 =>
				ppReady_out <= mcReady_in;
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;
				rstate_next <= R_WRITE_OTHER_NOP2;
			when R_WRITE_OTHER_NOP2 =>
				rstate_next <= R_WRITE_OTHER_NOP3;
			when R_WRITE_OTHER_NOP3 =>
				rstate_next <= R_WRITE_OTHER_NOP4;
			when R_WRITE_OTHER_NOP4 =>
				ppData_out <= mcData_in;
				ppRDV_out <= mcRDV_in;
				rstate_next <= R_WRITE_OTHER_EXEC;

			-- Now execute the foreign write - it'll be handled by someone else so just copy it over
			-- to the trace FIFO.
			--
			when R_WRITE_OTHER_EXEC =>
				rstate_next <= R_WRITE_OTHER_FINISH;
				traceData_out <= "000000" & mdAS & mdDSW_sync & addrReg & mdData_sync;
				traceValid_out <= traceEnable_in;
			when R_WRITE_OTHER_FINISH =>
				if ( mdDSW_sync = "11" ) then
					rstate_next <= R_IDLE;
				end if;

			-- -------------------------------------------------------------------------------------
			-- A register write has been requested, but things are not yet stable so give the host
			-- enough time for one I/O cycle, if it wants it - this will provide enough of a delay
			-- for the write masks and data to stabilise.
			--
			when R_WRITE_REG_NOP1 =>
				ppReady_out <= mcReady_in;
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;
				rstate_next <= R_WRITE_REG_NOP2;
			when R_WRITE_REG_NOP2 =>
				rstate_next <= R_WRITE_REG_NOP3;
			when R_WRITE_REG_NOP3 =>
				rstate_next <= R_WRITE_REG_NOP4;
			when R_WRITE_REG_NOP4 =>
				ppData_out <= mcData_in;
				ppRDV_out <= mcRDV_in;
				rstate_next <= R_WRITE_REG_EXEC;

			-- Now execute the register write.
			--
			when R_WRITE_REG_EXEC =>
				rstate_next <= R_WRITE_REG_FINISH;
				traceData_out <= "000000" & mdAS & mdDSW_sync & addrReg & mdData_sync;
				traceValid_out <= traceEnable_in;
				memBank_next(to_integer(unsigned(mdData_sync(6) & addrReg(2 downto 0)))) <= mdData_sync(4 downto 0);
			when R_WRITE_REG_FINISH =>
				if ( mdDSW_sync = "11" and mcReady_in = '1' ) then
					rstate_next <= R_IDLE;
				end if;

			-- -------------------------------------------------------------------------------------
			-- R_IDLE & others.
			--
			when others =>
				-- See if the host wants MD back in reset
				if ( mdReset_in = '1' ) then
					-- MD back in reset, so give host full control again
					rstate_next <= R_RESET;
				end if;

				if ( mdOE_sync = '0' ) then
					-- MD is reading
					addrReg_next <= mdAddr_sync;
					mdAS_next <= mdAS_sync;
					if ( mdAddr_sync(22) = '0' ) then
						-- MD is doing an owned read (i.e in our address ranges)
						rstate_next <= R_READ_OWNED_WAIT;
						mcCmd_out <= MC_RD;
						mcAddr_out <= transAddr(mdAddr_sync);
					else
						-- MD is doing a foreign read (i.e not in our address ranges)
						rstate_next <= R_READ_OTHER;
					end if;
				elsif ( mdDSW_sync /= "11" ) then
					-- MD is writing
					addrReg_next <= mdAddr_sync;
					mdAS_next <= mdAS_sync;
					if ( mdAddr_sync(22 downto 3) = x"A130F" ) then
						-- MD is writing 0xA130Fx range (SSF2 registers)
						if ( mdAddr_sync(2 downto 0) = "000" ) then
							-- The 0xA130F0 register is not mapped
							rstate_next <= R_WRITE_OTHER_NOP1;
						else
							-- The 0xA130F2-0xA130FE registers are mapped
							rstate_next <= R_WRITE_REG_NOP1;
						end if;
					elsif ( mdAddr_sync(22) = '0' ) then
						-- MD is doing an owned write (i.e in our address ranges)
						rstate_next <= R_WRITE_OWNED_NOP1;
					else
						-- MD is doing a foreign write (i.e not in our address ranges)
						rstate_next <= R_WRITE_OTHER_NOP1;
					end if;
				end if;
		end case;
	end process;

	-- #############################################################################################
	-- ##                     State machine to control the MD data-bus buffer                     ##
	-- #############################################################################################
	process(
		mstate, addrReg, dataReg, mdOE_sync, mdAddr_sync(22), mcData_in, mcRDV_in)
	begin
		mstate_next <= mstate;
		mdData_io <= (others => 'Z');
		mdDriveBus_out <= '0';
		
		case mstate is
			when M_READ_WAIT =>
				mdData_io <= mcData_in;
				mdDriveBus_out <= '1';
				if ( mcRDV_in = '1' ) then
					mstate_next <= M_END_WAIT;
				end if;

			when M_END_WAIT =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
				if ( mdOE_sync = '1' ) then
					mstate_next <= M_IDLE;
					mdData_io <= (others => 'Z');
					mdDriveBus_out <= '0';
				end if;

			when M_IDLE =>
				if ( mdOE_sync = '0' and mdAddr_sync(22) = '0' ) then
					mstate_next <= M_READ_WAIT;
				end if;
		end case;
	end process;

	mdDTACK_out <= '0';  -- for now, just rely on MD's auto-DTACK
end architecture;

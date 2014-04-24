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
		traceData_out  : out std_logic_vector(71 downto 0);
		traceValid_out : out std_logic;

		-- MegaDrive registers
		regAddr_out     : out std_logic_vector(2 downto 0);
		regWrData_out   : out std_logic_vector(15 downto 0);
		regWrValid_out  : out std_logic;
		regRdData_in    : in  std_logic_vector(15 downto 0);
		regRdStrobe_out : out std_logic;
		regMapRam_in    : in  std_logic
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
	type BankType is array (0 to 15) of std_logic_vector(4 downto 0);
	
	-- Registers
	signal rstate       : RStateType := R_RESET;
	signal rstate_next  : RStateType;
	signal dataReg      : std_logic_vector(15 downto 0) := (others => '0');
	signal dataReg_next : std_logic_vector(15 downto 0);
	signal addrReg      : std_logic_vector(22 downto 0) := (others => '0');
	signal addrReg_next : std_logic_vector(22 downto 0);
	signal mdAS         : std_logic;
	signal mdAS_next    : std_logic;
	signal count48      : unsigned(29 downto 0) := (others => '0');
	signal count48_next : unsigned(29 downto 0);
	signal memBank      : BankType := (
		"00000", "00001", "00010", "00011", "00100", "00101", "00110", "00111",
		"01000", "01001", "01010", "01011", "01100", "01101", "01110", "01111"
	);
	signal memBank_next : BankType;
	signal bootInsn     : std_logic_vector(15 downto 0);

	-- Synchronise MegaDrive signals to sysClk
	signal mdAS_sync    : std_logic := '1';
	signal mdOE_sync    : std_logic := '1';
	signal mdDSW_sync   : std_logic_vector(1 downto 0) := "11";
	signal mdAddr_sync  : std_logic_vector(22 downto 0) := (others => '0');
	signal mdData_sync  : std_logic_vector(15 downto 0) := (others => '0');
	constant TR_RD      : std_logic_vector(1 downto 0) := "11";
	constant TR_WB      : std_logic_vector(1 downto 0) := "00";
begin
	-- Infer registers
	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			if ( reset_in = '1' ) then
				rstate <= R_RESET;
				dataReg <= (others => '0');
				addrReg <= (others => '0');
				mdAddr_sync <= (others => '0');
				mdAS_sync <= '1';
				mdOE_sync <= '1';
				mdDSW_sync <= "11";
				mdData_sync <= (others => '0');
				mdAS <= '1';
				count48 <= (others => '0');
				memBank <= (
					"00000", "00001", "00010", "00011", "00100", "00101", "00110", "00111",
					"11111", "01001", "01010", "01011", "01100", "01101", "01110", "01111"
				);
			else
				rstate <= rstate_next;
				dataReg <= dataReg_next;
				addrReg <= addrReg_next;
				mdAddr_sync <= mdAddr_in;
				mdAS_sync <= mdAS_in;
				mdOE_sync <= mdOE_in;
				mdDSW_sync <= mdUDSW_in & mdLDSW_in;
				mdData_sync <= mdData_io;
				mdAS <= mdAS_next;
				count48 <= count48_next;
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
			regMapRam_in, bootInsn, memBank, regRdData_in,
			count48, traceEnable_in
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

		-- MegaDrive registers
		regAddr_out <= (others => 'X');
		regWrData_out <= (others => 'X');
		regWrValid_out <= '0';
		regRdStrobe_out <= '0';

		-- MegaDrive data bus
		mdData_io <= (others => 'Z');
		mdDriveBus_out <= '0';

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
				mdData_io <= mcData_in;
				mdDriveBus_out <= '1';
				if ( mcRDV_in = '1' ) then
					rstate_next <= R_READ_OWNED_NOP1;
					if ( regMapRam_in = '1' ) then
						dataReg_next <= mcData_in;
						traceData_out <= std_logic_vector(count48) & mdAS & TR_RD & addrReg & mcData_in;
					else
						dataReg_next <= bootInsn;
						traceData_out <= std_logic_vector(count48) & mdAS & TR_RD & addrReg & bootInsn;
					end if;
					traceValid_out <= traceEnable_in;
				end if;

			-- Give the host enough time for one I/O cycle, if it wants it.
			--
			when R_READ_OWNED_NOP1 =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
				ppReady_out <= mcReady_in;
				mcCmd_out <= ppCmd_in;
				mcAddr_out <= ppAddr_in;
				mcData_out <= ppData_in;
				rstate_next <= R_READ_OWNED_NOP2;
			when R_READ_OWNED_NOP2 =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
				rstate_next <= R_READ_OWNED_NOP3;
			when R_READ_OWNED_NOP3 =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
				rstate_next <= R_READ_OWNED_NOP4;
			when R_READ_OWNED_NOP4 =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
				ppData_out <= mcData_in;
				ppRDV_out <= mcRDV_in;
				rstate_next <= R_READ_OWNED_REFRESH;
				
			-- Start a refresh cycle, then wait for it to complete.
			--
			when R_READ_OWNED_REFRESH =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
				rstate_next <= R_READ_OWNED_FINISH;
				mcCmd_out <= MC_REF;
			when R_READ_OWNED_FINISH =>
				mdData_io <= dataReg;
				mdDriveBus_out <= '1';
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
					traceData_out <= std_logic_vector(count48) & mdAS & TR_RD & addrReg & mdData_sync;
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
				traceData_out <= std_logic_vector(count48) & mdAS & mdDSW_sync & addrReg & mdData_sync;
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
				traceData_out <= std_logic_vector(count48) & mdAS & mdDSW_sync & addrReg & mdData_sync;
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
				traceData_out <= std_logic_vector(count48) & mdAS & mdDSW_sync & addrReg & mdData_sync;
				traceValid_out <= traceEnable_in;
				if ( addrReg(6 downto 3) = "1111" ) then
					memBank_next(to_integer(unsigned(mdData_sync(6) & addrReg(2 downto 0)))) <= mdData_sync(4 downto 0);
				elsif ( addrReg(6 downto 3) = "0000" ) then
					regAddr_out <= addrReg(2 downto 0);
					regWrData_out <= mdData_sync;
					regWrValid_out <= '1';
				end if;
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
					if ( mdAddr_sync(22 downto 7) = x"A130" ) then
						-- MD is reading the 0xA130xx range
						rstate_next <= R_READ_OWNED_NOP1;
						regAddr_out <= mdAddr_sync(2 downto 0);
						dataReg_next <= regRdData_in;
						regRdStrobe_out <= '1';
						traceData_out <= std_logic_vector(count48) & mdAS & TR_RD & mdAddr_sync & regRdData_in;
						traceValid_out <= traceEnable_in;
					elsif ( mdAddr_sync(22) = '0' ) then
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
					if ( mdAddr_sync(22 downto 7) = x"A130" ) then
						-- MD is writing 0xA130xx range
						if ( mdAddr_sync(6 downto 0) = "1111000" ) then
							-- The 0xA130F0 register is not mapped
							rstate_next <= R_WRITE_OTHER_NOP1;
						else
							-- All others are mapped
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

	-- Boot ROM - just load the bootblock from flash into onboard RAM and start it running
	with addrReg(4 downto 0) select bootInsn <=
		x"0000" when "00000", -- initial SSP
		x"0000" when "00001",
		x"0000" when "00010", -- initial PC
		x"0008" when "00011",
		x"41F9" when "00100", -- lea 0xA13000, a0
		x"00A1" when "00101",
		x"3000" when "00110",
		x"43F9" when "00111", -- lea 0xFF0000, a1
		x"00FF" when "01000",
		x"0000" when "01001",
		x"317C" when "01010", -- move.w #(TURBO|FLASH), 4(a0)
		x"0005" when "01011",
		x"0004" when "01100",
		x"30BC" when "01101", -- move.w #0x0306, (a0)
		x"0306" when "01110",
		x"30BC" when "01111", -- move.w #0x0000, (a0)
		x"0000" when "10000",
		x"30BC" when "10001", -- move.w #0xFFFF, (a0)
		x"FFFF" when "10010",
		x"707F" when "10011", -- moveq #127, d0 ; copy 128 words = 256 bytes
		x"32D0" when "10100", -- move.w (a0), (a1)+
		x"51C8" when "10101", -- dbra d0, *-4
		x"FFFC" when "10110",
		x"4EF9" when "10111", -- jmp 0xFF0000
		x"00FF" when "11000",
		x"0000" when "11001",
		(others => 'X') when others;

	-- A verifiable test bootblock can be installed like this:
	--
	-- $ printf '\x31\x7C\x00\x00\x00\x04\x33\xFC\x60\xFE\x00\x40\x00\x00\x4E\xF9\x00\x40\x00\x00' > bra.bin
	-- $ $HOME/makestuff/apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -p J:A7A0A3A1:spi-talk.xsvf
	-- $ $HOME/makestuff/apps/gordon/lin.x64/rel/gordon -v 1d50:602b:0002 -t indirect:1 -w bra.bin:0x60000
	-- $ $HOME/makestuff/apps/gordon/lin.x64/rel/gordon -v 1d50:602b:0002 -t indirect:1 -r foo.bin:0x60000:256
	-- $ dis68 foo.bin 0 3
	-- 0x000000  move.w #0, 4(a0)
	-- 0x000006  move.w #0x60FE, 0x400000 ; opcode for bra.s -2
	-- 0x00000E  jmp 0x400000
	-- 
	-- It consists of the opcodes to deselect the flash (and thereby map in the SDRAM), then write
	-- the opcode for bra.s -2 to 0x400000 and jump to it. Reads from onboard RAM don't show on the
	-- trace, but you can see the infinite bra.s -2 loop executing OK.
	
	mdDTACK_out <= '0';  -- for now, just rely on MD's auto-DTACK
	count48_next <= count48 + 1;
	
end architecture;

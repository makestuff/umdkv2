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
use ieee.std_logic_textio.all;
use std.textio.all;
use work.hex_util.all;
use work.mem_ctrl_pkg.all;

entity mem_arbiter_tb is
end entity;

architecture behavioural of mem_arbiter_tb is
	-- Clocks
	signal sysClk      : std_logic;  -- main system clock
	signal dispClk     : std_logic;  -- display version of sysClk, which transitions 4ns before it
	signal reset       : std_logic;
	
	-- I/O pipes
	signal cmdData     : std_logic_vector(15 downto 0);
	signal cmdValid    : std_logic;
	signal cmdReady    : std_logic;
	signal rspData     : std_logic_vector(15 downto 0);
	signal rspValid    : std_logic;
	signal rspReady    : std_logic;
	
	-- Pipe interface
	signal ppReady     : std_logic;
	signal ppCmd       : MCCmdType;
	signal ppAddr      : std_logic_vector(22 downto 0);
	signal ppDataWr    : std_logic_vector(15 downto 0);
	signal ppDataRd    : std_logic_vector(15 downto 0);
	signal ppRDV       : std_logic;
	
	-- Memory controller interface
	signal mcAutoMode  : std_logic;
	signal mcReady     : std_logic;
	signal mcCmd       : MCCmdType;
	signal mcAddr      : std_logic_vector(22 downto 0);
	signal mcDataWr    : std_logic_vector(15 downto 0);
	signal mcDataRd    : std_logic_vector(15 downto 0);
	signal mcRDV       : std_logic;
	
	-- SDRAM signals
	signal ramCmd      : std_logic_vector(2 downto 0);
	signal ramBank     : std_logic_vector(1 downto 0);
	signal ramAddr     : std_logic_vector(11 downto 0);
	signal ramDataIO   : std_logic_vector(15 downto 0);
	signal ramLDQM     : std_logic;
	signal ramUDQM     : std_logic;

	-- MegaDrive external interface
	signal mdDriveBus  : std_logic;
	signal mdReset     : std_logic;
	signal mdDTACK     : std_logic;
	signal mdAddr      : std_logic_vector(22 downto 0);
	signal mdData      : std_logic_vector(15 downto 0);
	signal mdOE        : std_logic;
	signal mdAS        : std_logic;
	signal mdLDSW      : std_logic;
	signal mdUDSW      : std_logic;

	-- Trace pipe
	signal traceEnable : std_logic;
	signal traceData   : std_logic_vector(71 downto 0);
	signal traceValid  : std_logic;
begin
	-- Instantiate the memory pipe
	mem_pipe: entity work.mem_pipe
		port map(
			clk_in       => sysClk,
			reset_in     => reset,

			-- I/O pipes
			cmdData_in   => cmdData,
			cmdValid_in  => cmdValid,
			cmdReady_out => cmdReady,
			rspData_out  => rspData,
			rspValid_out => rspValid,
			rspReady_in  => rspReady,

			-- Connection to mem_arbiter
			mcReady_in   => ppReady,
			mcCmd_out    => ppCmd,
			mcAddr_out   => ppAddr,
			mcData_out   => ppDataWr,
			mcData_in    => ppDataRd,
			mcRDV_in     => ppRDV
		);
	
	-- Instantiate the memory arbiter for testing
	uut: entity work.mem_arbiter
		port map(
			clk_in         => sysClk,
			reset_in       => reset,

			-- Connetion to mem_pipe
			ppReady_out    => ppReady,
			ppCmd_in       => ppCmd,
			ppAddr_in      => ppAddr,
			ppData_in      => ppDataWr,
			ppData_out     => ppDataRd,
			ppRDV_out      => ppRDV,

			-- Connection to mem_ctrl
			mcAutoMode_out => mcAutoMode,
			mcReady_in     => mcReady,
			mcCmd_out      => mcCmd,
			mcAddr_out     => mcAddr,
			mcData_out     => mcDataWr,
			mcData_in      => mcDataRd,
			mcRDV_in       => mcRDV,

			-- Connection to MegaDrive
			mdDriveBus_out => mdDriveBus,
			mdReset_in     => mdReset,
			mdDTACK_out    => mdDTACK,
			mdAddr_in      => mdAddr,
			mdData_io      => mdData,
			mdOE_in        => mdOE,
			mdAS_in        => mdAS,
			mdLDSW_in      => mdLDSW,
			mdUDSW_in      => mdUDSW,

			-- Trace pipe
			traceEnable_in => traceEnable,
			traceData_out  => traceData,
			traceValid_out => traceValid
		);

	-- Instantiate the memory controller
	mem_ctrl: entity work.mem_ctrl
		generic map(
			INIT_COUNT     => "0" & x"004",  --\
			REFRESH_DELAY  => "0" & x"010",  -- Much longer in real hardware!
			REFRESH_LENGTH => "0" & x"002"   --/
		)
		port map(
			clk_in        => sysClk,
			reset_in      => reset,

			-- Connection to mem_arbiter
			mcAutoMode_in => mcAutoMode,
			mcReady_out   => mcReady,
			mcCmd_in      => mcCmd,
			mcAddr_in     => mcAddr,
			mcData_in     => mcDataWr,
			mcData_out    => mcDataRd,
			mcRDV_out     => mcRDV,

			-- Connection to sdram_model
			ramCmd_out    => ramCmd,
			ramBank_out   => ramBank,
			ramAddr_out   => ramAddr,
			ramData_io    => ramDataIO,
			ramLDQM_out   => ramLDQM,
			ramUDQM_out   => ramUDQM
		);

	-- Instantiate the SDRAM model for testing
	sdram_model: entity work.sdram_model
		port map(
			ramClk_in  => sysClk,
			ramCmd_in  => ramCmd,
			ramBank_in => ramBank,
			ramAddr_in => ramAddr,
			ramData_io => ramDataIO
		);

	-- Drive the clocks. In simulation, sysClk lags 4ns behind dispClk, to give a visual hold time
	-- for signals in GTKWave.
	process
	begin
		sysClk <= '0';
		dispClk <= '0';
		wait for 16 ns;
		loop
			dispClk <= not(dispClk);  -- first dispClk transitions
			wait for 4 ns;
			sysClk <= not(sysClk);  -- then sysClk transitions, 4ns later
			wait for 6 ns;
		end loop;
	end process;

	-- Deassert the synchronous reset one cycle after startup.
	--
	process
	begin
		reset <= '1';
		wait until rising_edge(sysClk);
		reset <= '0';
		wait;
	end process;

	-- Drive the unit under test. Read stimulus from stimulus.sim and write results to results.sim
	process
		procedure cmdWord(constant w : in std_logic_vector(15 downto 0)) is
		begin
			cmdData <= w;
			cmdValid <= '1';
			if ( cmdReady = '0' ) then
				wait until rising_edge(cmdReady);
			end if;
			wait until rising_edge(sysClk);
			cmdData <= (others => 'X');
			cmdValid <= '0';
			wait until rising_edge(sysClk);
		end procedure;
	begin
		-- Simulate MD on power-on
		mdReset <= '1';
		mdAddr <= (others => 'Z');
		mdOE <= '1';
		mdAS <= '1';
		mdLDSW <= '1';
		mdUDSW <= '1';
		traceEnable <= '1';

		-- Simulate load of initial program
		cmdData <= (others => 'X');
		cmdValid <= '0';
		rspReady <= '1';
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		cmdWord(x"0012");  -- set pointer = 0x123456
		cmdWord(x"3456");
		cmdWord(x"8000");  -- write 0x10 words
		cmdWord(x"0010");
		cmdWord(x"CAFE");
		cmdWord(x"BABE");
		cmdWord(x"DEAD");
		cmdWord(x"F00D");
		cmdWord(x"0123");
		cmdWord(x"4567");
		cmdWord(x"89AB");
		cmdWord(x"CDEF");
		cmdWord(x"BABA");
		cmdWord(x"B0B0");
		cmdWord(x"BFBF");
		cmdWord(x"1122");
		cmdWord(x"3344");
		cmdWord(x"5566");
		cmdWord(x"7788");
		cmdWord(x"99AA");
		cmdWord(x"0012");  -- set pointer = 0x123456
		cmdWord(x"3456");
		cmdWord(x"4000");  -- read 0x10 words
		cmdWord(x"0010");

		-- Wait for read of 0x10 words to complete
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);

		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);

		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);

		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);
		wait until rising_edge(mcRDV);

		-- Wait a bit longer
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);

		-- Bring the MD out of reset and simulate a read of 0x123456.
		mdReset <= '0';
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		mdAddr <= "001" & x"23456";
		wait until rising_edge(sysClk);
		mdOE <= '0';
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);

		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		mdOE <= '1';
		mdAddr <= (others => 'Z');
		wait;
	end process;
end architecture;

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
use ieee.std_logic_textio.all;
use std.textio.all;

entity businterface_tb is
end businterface_tb;

architecture behavioural of businterface_tb is

	signal reset        : std_logic;
	signal clk          : std_logic;

	-- External connections
	signal mdBufOE      : std_logic;  -- while 'Z', MD is isolated; drive low to enable all three buffers.
	signal mdBufDir     : std_logic;  -- drive high when MegaDrive reading, low when MegaDrive writing.
	signal mdOE         : std_logic;  -- active low output enable: asserted when MD accesses cartridge address space.
	signal mdLDSW       : std_logic;  -- active low lower data strobe write.
	signal mdAddr       : std_logic_vector(20 downto 0);
	signal mdDataIO     : std_logic_vector(15 downto 0);

	-- Internal connections
	signal mdBeginRead  : std_logic;  -- asserted at the beginning of each relevant read
	signal mdBeginWrite : std_logic;  -- asserted at the beginning of each relevant write
	signal mdAccessMem  : std_logic;  -- asserted when this cycle is to our mem region
	signal mdAccessIO   : std_logic;  -- asserted when this cycle is to our I/O region
	signal mdSyncOE     : std_logic;  -- sync version of mdOE_in
	signal mdSyncWE     : std_logic;  -- sync version of mdLDSW_in
	signal mdDataIn     : std_logic_vector(15 downto 0);

begin

	-- Instantiate the unit under test
	uut: entity work.businterface
		port map(
			reset_in  => reset,
			clk_in    => clk,

			-- External connections
			mdBufOE_out => mdBufOE,
			mdBufDir_out => mdBufDir,
			mdOE_in => mdOE,
			mdLDSW_in => mdLDSW,
			mdAddr_in => mdAddr,
			mdData_io => mdDataIO,

			-- Internal connections
			mdBeginRead_out => mdBeginRead,
			mdBeginWrite_out => mdBeginWrite,
			mdAccessMem_out => mdAccessMem,
			mdAccessIO_out => mdAccessIO,
			mdSyncOE_out => mdSyncOE,
			mdSyncWE_out => mdSyncWE,
			mdData_in => mdDataIn
		);

	-- Drive the clock
	process
	begin
		clk <= '0';
		wait for 5 ns;
		clk <= '1';
		wait for 5 ns;
	end process;

	-- Drive the bus interface: send from stimulus.txt
	process
		variable inLine   : line;
		variable inData   : std_logic_vector(55 downto 0);  -- 1+23+16+16
		file inFile       : text open read_mode is "stimulus.txt";
		constant EXT_ADDR : std_logic_vector(22 downto 0) := "111" & x"F8000";
	begin
		mdOE     <= '1';
		mdLDSW   <= '1';
		mdAddr   <= EXT_ADDR(22 downto 2);
		mdDataIn <= (others => 'X');
		mdDataIO <= (others => 'X');
		reset <= '1';
		wait for 10 ns;
		reset <= '0';
		wait for 40 ns;
		loop
			exit when endfile(inFile);
			readline(inFile, inLine);
			read(inLine, inData);
			if ( inData(55) = '1' ) then
				mdAddr <= inData(54 downto 34);  -- Write operation: copy mdDataIO to mdDataOut
				mdDataIn <= (others => 'X');
				mdDataIO <= inData(15 downto 0);
				wait for 10 ns;
				mdOE   <= '1';
				mdLDSW <= '0';
			else
				mdAddr <= inData(54 downto 34);  -- Read operation: copy mdDataIn to mdDataIO
				mdDataIn <= inData(31 downto 16);
				mdDataIO <= (others => 'Z');
				wait for 10 ns;
				mdOE   <= '0';
				mdLDSW <= '1';
			end if;
			wait for 40 ns;
			mdOE     <= '1';
			mdLDSW   <= '1';
			wait for 10 ns;
			mdAddr   <= EXT_ADDR(22 downto 2);
			mdDataIn <= (others => 'X');
			mdDataIO <= (others => 'X');
			wait for 40 ns;
		end loop;
		wait;
		--assert false report "NONE. End of simulation." severity failure;
	end process;
end architecture;

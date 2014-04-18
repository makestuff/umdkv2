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

entity spi_funnel_tb is
end entity;

architecture behavioural of spi_funnel_tb is
	signal sysClk      : std_logic;  -- main system clock
	signal dispClk     : std_logic;  -- display version of sysClk, which transitions 4ns before it
	signal reset       : std_logic;
	signal cpuWrData   : std_logic_vector(15 downto 0);
	signal cpuWrValid  : std_logic;
	signal cpuRdData   : std_logic_vector(15 downto 0);
	signal cpuByteWide : std_logic;
	signal sendData    : std_logic_vector(7 downto 0);
	signal sendValid   : std_logic;
	signal sendReady   : std_logic;
	signal recvData    : std_logic_vector(7 downto 0);
	signal recvValid   : std_logic;
	signal recvReady   : std_logic;
	signal spiClk      : std_logic;
	signal spiDataOut  : std_logic;
	signal spiDataIn   : std_logic;
begin
	-- Instantiate the memory arbiter for testing
	uut: entity work.spi_funnel
		port map(
			clk_in         => sysClk,
			reset_in       => reset,

			-- CPU I/O
			cpuByteWide_in => cpuByteWide,
			cpuWrData_in   => cpuWrData,
			cpuWrValid_in  => cpuWrValid,
			cpuRdData_out  => cpuRdData,
			cpuRdStrobe_in => '0',

			-- Sending SPI data
			sendData_out   => sendData,
			sendValid_out  => sendValid,
			sendReady_in   => sendReady,

			-- Receiving SPI data
			recvData_in    => recvData,
			recvValid_in   => recvValid,
			recvReady_out  => recvReady
		);

	-- SPI master
	spi_master : entity work.spi_master
		generic map(
			SLOW_COUNT => "111011",  -- spiClk = sysClk/120 (400kHz @48MHz)
			FAST_COUNT => "000000",  -- spiClk = sysClk/2 (24MHz @48MHz)
			BIT_ORDER  => '1'        -- MSB first
		)
		port map(
			clk_in        => sysClk,
			reset_in      => reset,
			
			-- Send pipe
			turbo_in      => '1',
			suppress_in   => '0',
			sendData_in   => sendData,
			sendValid_in  => sendValid,
			sendReady_out => sendReady,
			
			-- Receive pipe
			recvData_out  => recvData,
			recvValid_out => recvValid,
			recvReady_in  => recvReady,
			
			-- SPI interface
			spiClk_out    => spiClk,
			spiData_out   => spiDataOut,
			spiData_in    => spiDataIn
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

	process
	begin
		cpuByteWide <= 'X';
		cpuWrData <= (others => 'X');
		cpuWrValid <= '0';
		spiDataIn <= '1';
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		cpuWrData <= x"0306";
		cpuByteWide <= '0';
		cpuWrValid <= '1';
		wait until rising_edge(sysClk);
		cpuWrData <= (others => 'X');
		cpuByteWide <= 'X';
		cpuWrValid <= '0';
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
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);

		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);

		cpuWrData <= x"55CA";
		cpuByteWide <= '1';
		cpuWrValid <= '1';
		wait until rising_edge(sysClk);
		cpuWrData <= (others => 'X');
		cpuByteWide <= 'X';
		cpuWrValid <= '0';
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
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);

		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);
		wait until rising_edge(sysClk);


		--cpuByteWide <= '1';
		--cpuWrData <= x"0055";
		--cpuWrValid <= '1';
		--wait until rising_edge(sysClk);
		--cpuWrData <= (others => 'X');
		--cpuWrValid <= '0';
		wait;
	end process;

end architecture;

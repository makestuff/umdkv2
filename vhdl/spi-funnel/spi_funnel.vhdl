--
-- Copyright (C) 2014 Chris McClelland
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

entity spi_funnel is
	port(
		clk_in         : in  std_logic;
		reset_in       : in  std_logic;

		-- CPU I/O
		cpuByteWide_in : in  std_logic;
		cpuWrData_in   : in  std_logic_vector(15 downto 0);
		cpuWrValid_in  : in  std_logic;
		cpuRdData_out  : out std_logic_vector(15 downto 0);
		cpuRdReady_in  : in  std_logic;
		
		-- Sending SPI data
		sendData_out   : out std_logic_vector(7 downto 0);
		sendValid_out  : out std_logic;
		sendReady_in   : in  std_logic;

		-- Receiving SPI data
		recvData_in    : in  std_logic_vector(7 downto 0);
		recvValid_in   : in  std_logic;
		recvReady_out  : out std_logic
	);
end entity;

architecture rtl of spi_funnel is
	type SStateType is (
		S_WRITE_MSB,
		S_WRITE_LSB
	);
	type RStateType is (
		S_WAIT_MSB,
		S_WAIT_LSB
	);
	signal sstate        : SStateType := S_WRITE_MSB;
	signal sstate_next   : SStateType;
	signal rstate        : RStateType := S_WAIT_MSB;
	signal rstate_next   : RStateType;
	signal byteWide      : std_logic := '0';
	signal byteWide_next : std_logic;
	signal lsb           : std_logic_vector(7 downto 0) := (others => '0');
	signal lsb_next      : std_logic_vector(7 downto 0);
	signal readData      : std_logic_vector(15 downto 0) := (others => '0');
	signal readData_next : std_logic_vector(15 downto 0);
begin
	-- Infer registers
	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			if ( reset_in = '1' ) then
				sstate <= S_WRITE_MSB;
				rstate <= S_WAIT_MSB;
				lsb <= (others => '0');
				readData <= (others => '0');
				byteWide <= '0';
			else
				sstate <= sstate_next;
				rstate <= rstate_next;
				lsb <= lsb_next;
				readData <= readData_next;
				byteWide <= byteWide_next;
			end if;
		end if;
	end process;

	-- Send state machine
	process(sstate, lsb, cpuWrData_in, cpuWrValid_in, sendReady_in, cpuByteWide_in, byteWide)
	begin
		sstate_next <= sstate;
		sendValid_out <= '0';
		lsb_next <= lsb;
		byteWide_next <= byteWide;
		case sstate is
			-- Now send the LSB to SPI and return:
			when S_WRITE_LSB =>
				sendData_out <= lsb;
				if ( sendReady_in = '1' ) then
					sendValid_out <= '1';
					sstate_next <= S_WRITE_MSB;
				end if;
				
			-- When the CPU writes a word, send the MSB to SPI:
			when others =>
				sendData_out <= cpuWrData_in(15 downto 8);
				if ( cpuByteWide_in = '1' ) then
					-- We're sending single bytes rather than 16-bit words
					if ( cpuWrValid_in = '1' and sendReady_in = '1' ) then
						sendValid_out <= '1';
						byteWide_next <= '1';
					end if;
				else
					-- We're sending 16-bit words rather than single bytes
					if ( cpuWrValid_in = '1' and sendReady_in = '1' ) then
						sstate_next <= S_WRITE_LSB;
						sendValid_out <= '1';
						lsb_next <= cpuWrData_in(7 downto 0);
						byteWide_next <= '0';
					end if;
				end if;
		end case;
	end process;

	-- Receive state machine
	process(rstate, readData, recvData_in, recvValid_in, cpuRdReady_in, byteWide)
	begin
		rstate_next <= rstate;
		readData_next <= readData;
		case rstate is
			-- Wait for the LSB to arrive:
			when S_WAIT_LSB =>
				recvReady_out <= '1';  -- ready for data from 8-bit side
				if ( recvValid_in = '1' and cpuRdReady_in = '1' ) then
					rstate_next <= S_WAIT_MSB;
					readData_next(7 downto 0) <= recvData_in;
				end if;
				
			-- When bytes arrive over SPI, present them (as bytes or words) to the CPU
			when others =>
				recvReady_out <= '1';  -- ready for data from 8-bit side
				if ( recvValid_in = '1' ) then
					if ( byteWide = '1' ) then
						-- We're receiving single bytes rather than 16-bit words
						readData_next <= recvData_in & "XXXXXXXX";
					else
						-- We're receiving 16-bit words rather than single bytes
						rstate_next <= S_WAIT_LSB;
						readData_next(15 downto 8) <= recvData_in;
					end if;
				end if;
		end case;
	end process;
	cpuRdData_out <= readData;
end architecture;

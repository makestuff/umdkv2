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
	signal sstate      : SStateType := S_WRITE_MSB;
	signal sstate_next : SStateType;
	signal lsb         : std_logic_vector(7 downto 0) := (others => '0');
	signal lsb_next    : std_logic_vector(7 downto 0);
begin
	-- Infer registers
	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			if ( reset_in = '1' ) then
				sstate <= S_WRITE_MSB;
				lsb <= (others => '0');
			else
				sstate <= sstate_next;
				lsb <= lsb_next;
			end if;
		end if;
	end process;

	-- Next state logic
	process(sstate, lsb, cpuWrData_in, cpuWrValid_in, sendReady_in)
	begin
		sstate_next <= sstate;
		sendValid_out <= '0';
		lsb_next <= lsb;
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
				sendValid_out <= cpuWrValid_in;
				if ( cpuWrValid_in = '1' and sendReady_in = '1' ) then
					lsb_next <= cpuWrData_in(7 downto 0);
					sstate_next <= S_WRITE_LSB;
				end if;
		end case;
	end process;

	-- Receive pipe 8->16 converter
	cmd_conv: entity work.conv_8to16
		port map(
			clk_in       => clk_in,
			reset_in     => reset_in,
			data8_in     => recvData_in,
			valid8_in    => recvValid_in,
			ready8_out   => recvReady_out,
			data16_out   => cpuRdData_out,
			valid16_out  => open,
			ready16_in   => cpuRdReady_in
		);
end architecture;

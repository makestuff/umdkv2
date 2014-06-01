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

entity reset_ctrl is
	generic (
		RESET_DELAY    : integer := 16
	);
	port(
		clk_in         : in  std_logic;
		hardReset_in   : in  std_logic;
		softReset_in   : in  std_logic;
		mdReset_out    : out std_logic
	);
end entity;

architecture rtl of reset_ctrl is
	type StateType is (
		S_IDLE,
		S_WAIT_SOFT
	);
	
	-- Registers
	signal state        : StateType := S_IDLE;
	signal state_next   : StateType;
	signal count        : unsigned(RESET_DELAY-1 downto 0) := (others => '0');
	signal count_next   : unsigned(RESET_DELAY-1 downto 0);
begin
	-- Infer registers
	process(clk_in)
	begin
		if ( rising_edge(clk_in) ) then
			state <= state_next;
			count <= count_next;
		end if;
	end process;

	-- State machine
	process(state, count, hardReset_in, softReset_in)
	begin
		state_next <= state;
		count_next <= count - 1;
		mdReset_out <= hardReset_in;
		case state is
			when S_WAIT_SOFT =>
				mdReset_out <= '1';
				if ( count = 0 ) then
					state_next <= S_IDLE;
				end if;
			
			when others =>
				if ( softReset_in = '1' ) then
					state_next <= S_WAIT_SOFT;
					count_next <= (others => '1');
					mdReset_out <= '1';
				end if;
		end case;
	end process;
end architecture;

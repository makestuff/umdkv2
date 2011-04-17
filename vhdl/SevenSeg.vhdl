--
-- Copyright (C) 2009-2010 Chris McClelland
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

entity SevenSeg is
	port(
		clk     : in    std_logic;
		data    : in    std_logic_vector(15 downto 0);
		segs    : out   std_logic_vector(6 downto 0);
		anodes  : out   std_logic_vector(3 downto 0)
	);
end SevenSeg;

architecture Behavioural of SevenSeg is

	-- Refresh rate 50M/2^18 ~ 190Hz
	-- Refresh rate 8M/2^16 ~ 122Hz
	-- Refresh rate 48M/2^19 ~ 92Hz	
	constant COUNTER_WIDTH : natural := 19;

	signal count        : unsigned(COUNTER_WIDTH-1 downto 0) := (others => '0');
	signal count_next   : unsigned(COUNTER_WIDTH-1 downto 0);
	signal anode_select : std_logic_vector(1 downto 0);
	signal nibble       : std_logic_vector(3 downto 0);

begin

	count_next   <= count + 1;
	anode_select <= std_logic_vector(count(COUNTER_WIDTH-1 downto COUNTER_WIDTH-2));
	
	-- Update counter, drive anodes and select bits to display for each 7-seg
	process(clk)
	begin
		if ( clk'event and clk = '1' ) then
			count <= count_next;
			case anode_select is
				when "00" =>
					anodes <= "0111";
					nibble <= data(15 downto 12);
				when "01" =>
					anodes <= "1011";
					nibble <= data(11 downto 8);
				when "10" =>
					anodes <= "1101";
					nibble <= data(7 downto 4);
				when others =>
					anodes <= "1110";
					nibble <= data(3 downto 0);
			end case;
		end if;
	end process;

	-- Decode selected nibble
	with nibble select
		segs <=
			"1000000" when "0000",
			"1111001" when "0001",
			"0100100" when "0010",
			"0110000" when "0011",
			"0011001" when "0100",
			"0010010" when "0101",
			"0000010" when "0110",
			"1111000" when "0111",
			"0000000" when "1000",
			"0010000" when "1001",
			"0001000" when "1010",
			"0000011" when "1011",
			"1000110" when "1100",
			"0100001" when "1101",
			"0000110" when "1110",
			"0001110" when others;

end Behavioural;

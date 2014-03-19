--
-- Copyright (C) 2009-2012 Chris McClelland
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

library unisim;
use unisim.vcomponents.all;

entity top_level is
	port(
		-- FX2 interface -----------------------------------------------------------------------------
		fx2Clk_in      : in    std_logic;                    -- 48MHz clock from FX2
		fx2FifoSel_out : out   std_logic;                    -- select FIFO: "0" for EP2OUT, "1" for EP6IN
		fx2Data_io     : inout std_logic_vector(7 downto 0); -- 8-bit data to/from FX2

		-- When EP2OUT selected:
		fx2Read_out    : out   std_logic;                    -- asserted (active-low) when reading from FX2
		fx2GotData_in  : in    std_logic;                    -- asserted (active-high) when FX2 has data for us

		-- When EP6IN selected:
		fx2Write_out   : out   std_logic;                    -- asserted (active-low) when writing to FX2
		fx2GotRoom_in  : in    std_logic;                    -- asserted (active-high) when FX2 has room for more data from us
		fx2PktEnd_out  : out   std_logic;                    -- asserted (active-low) when a host read needs to be committed early

		-- SDRAM signals -----------------------------------------------------------------------------
		ramClk_out     : out   std_logic;
		ramCmd_out     : out   std_logic_vector(2 downto 0);
		ramBank_out    : out   std_logic_vector(1 downto 0);
		ramAddr_out    : out   std_logic_vector(11 downto 0);
		ramData_io     : inout std_logic_vector(15 downto 0);
		ramLDQM_out    : out   std_logic;
		ramUDQM_out    : out   std_logic;

		-- MegaDrive interface -----------------------------------------------------------------------
		mdReset_out    : out   std_logic;
		mdDriveBus_out : out   std_logic;
		mdDTACK_out    : out   std_logic;
		mdAddr_in      : in    std_logic_vector(22 downto 0);
		mdData_io      : inout std_logic_vector(15 downto 0);
		mdOE_in        : in    std_logic;
		mdAS_in        : in    std_logic;
		mdLDSW_in      : in    std_logic;
		mdUDSW_in      : in    std_logic
	);
end entity;

architecture structural of top_level is
	-- Channel read/write interface -----------------------------------------------------------------
	signal chanAddr  : std_logic_vector(6 downto 0);  -- the selected channel (0-127)

	-- Host >> FPGA pipe:
	signal h2fData   : std_logic_vector(7 downto 0);  -- data lines used when the host writes to a channel
	signal h2fValid  : std_logic;                     -- '1' means "on the next clock rising edge, please accept the data on h2fData"
	signal h2fReady  : std_logic;                     -- channel logic can drive this low to say "I'm not ready for more data yet"

	-- Host << FPGA pipe:
	signal f2hData   : std_logic_vector(7 downto 0);  -- data lines used when the host reads from a channel
	signal f2hValid  : std_logic;                     -- channel logic can drive this low to say "I don't have data ready for you"
	signal f2hReady  : std_logic;                     -- '1' means "on the next clock rising edge, put your next byte of data on f2hData"
	-- ----------------------------------------------------------------------------------------------

	-- Clock synthesis & reset
	signal sysClk000  : std_logic;
	signal sysClk180  : std_logic;
	signal locked     : std_logic;
	signal reset      : std_logic;
begin
	-- CommFPGA module
	comm_fpga_fx2 : entity work.comm_fpga_fx2
		port map(
			clk_in         => sysClk000,
			reset_in       => reset,
			reset_out      => open,

			-- FX2 interface
			fx2FifoSel_out => fx2FifoSel_out,
			fx2Data_io     => fx2Data_io,
			fx2Read_out    => fx2Read_out,
			fx2GotData_in  => fx2GotData_in,
			fx2Write_out   => fx2Write_out,
			fx2GotRoom_in  => fx2GotRoom_in,
			fx2PktEnd_out  => fx2PktEnd_out,

			-- DVR interface -> Connects to application module
			chanAddr_out   => chanAddr,
			h2fData_out    => h2fData,
			h2fValid_out   => h2fValid,
			h2fReady_in    => h2fReady,
			f2hData_in     => f2hData,
			f2hValid_in    => f2hValid,
			f2hReady_out   => f2hReady
		);

	-- SDRAM application
	umdkv2_app : entity work.umdkv2
		port map(
			clk_in         => sysClk000,
			reset_in       => reset,
			
			-- DVR interface -> Connects to comm_fpga module
			chanAddr_in    => chanAddr,
			h2fData_in     => h2fData,
			h2fValid_in    => h2fValid,
			h2fReady_out   => h2fReady,
			f2hData_out    => f2hData,
			f2hValid_out   => f2hValid,
			f2hReady_in    => f2hReady,
			
			-- SDRAM interface
			ramCmd_out     => ramCmd_out,
			ramBank_out    => ramBank_out,
			ramAddr_out    => ramAddr_out,
			ramData_io     => ramData_io,
			ramLDQM_out    => ramLDQM_out,
			ramUDQM_out    => ramUDQM_out,
			
			-- MegaDrive interface
			mdReset_out    => mdReset_out,
			mdDriveBus_out => mdDriveBus_out,
			mdDTACK_out    => mdDTACK_out,
			mdAddr_in      => mdAddr_in,
			mdData_io      => mdData_io,
			mdOE_in        => mdOE_in,
			mdAS_in        => mdAS_in,
			mdLDSW_in      => mdLDSW_in,
			mdUDSW_in      => mdUDSW_in
		);

	-- Generate the system clock from the FX2LP's 48MHz clock
	clk_gen: entity work.clk_gen
		port map(
			clk_in     => fx2Clk_in,
			clk000_out => sysClk000,
			clk180_out => sysClk180,
			locked_out => locked
		);

	-- Drive system clock and sync reset
	clk_drv: ODDR2
		port map(
			D0 => '1',
			D1 => '0',
			C0 =>	sysClk000,
			C1 => sysClk180,
			Q  => ramClk_out
		);

	reset <= not(locked);

end architecture;

--
-- Copyright (C) 2009-2011 Chris McClelland
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

entity TopLevel is
	port(
		reset_in     : in    std_logic;
		ifclk_in     : in    std_logic;

		-- Unused FX2LP connections configured as inputs
		flagA_in     : in    std_logic;
		int0_in      : in    std_logic;

		-- Data & control from the FX2
		fifoData_io  : inout std_logic_vector(7 downto 0);
		gotData_in   : in    std_logic;                     -- FLAGC=EF (active-low), so '1' when there's data
		gotRoom_in   : in    std_logic;                     -- FLAGB=FF (active-low), so '1' when there's room

		-- Control to the FX2
		sloe_out     : out   std_logic;                    -- PA2
		slrd_out     : out   std_logic;
		slwr_out     : out   std_logic;
		fifoAddr_out : out   std_logic_vector(1 downto 0); -- PA4 & PA5
		pktEnd_out   : out   std_logic;                    -- PA6

		-- RAM signals
		ramClk_out   : out   std_logic;
		ramAddr_out  : out   std_logic_vector(22 downto 0);
		ramData_io   : inout std_logic_vector(15 downto 0);
		ramCE_out    : out   std_logic;
		ramOE_out    : out   std_logic;
		ramWE_out    : out   std_logic;
		ramADV_out   : out   std_logic;
		ramCRE_out   : out   std_logic;
		ramLB_out    : out   std_logic;
		ramUB_out    : out   std_logic;
		ramWait_in   : in    std_logic;

		-- MegaDrive signals
		mdReset_out  : out   std_logic;  -- while 'Z', MD stays in RESET; drive low to bring MD out of RESET.
		mdBufOE_out  : out   std_logic;  -- while 'Z', MD is isolated; drive low to enable all three buffers.
		mdBufDir_out : out   std_logic;  -- drive high when MegaDrive reading, low when MegaDrive writing.
		mdOE_in      : in    std_logic;  -- active low output enable: asserted when MD accesses cartridge address space.
		mdLDSW_in    : in    std_logic;  -- active low lower data strobe write.
		--mdUDSW_in    : in    std_logic;  -- active low upper data strobe write.
		--mdClk_in     : in    std_logic;  -- 7.6MHz MegaDrive clock (not used).
		--mdAS_in      : in    std_logic;  -- active low address strobe from 68000 (not asserted for non-68000 cycles).
		mdAddr_in    : in    std_logic_vector(22 downto 0);
		mdData_io    : inout std_logic_vector(15 downto 0);

		-- Debug ports
		jc2_out   : out   std_logic;
		jc7_out   : out   std_logic;

		-- Onboard peripherals
		sseg_out     : out   std_logic_vector(7 downto 0);
		anode_out    : out   std_logic_vector(3 downto 0);
		led_out      : out   std_logic_vector(7 downto 0)
	);
end TopLevel;

architecture Behavioural of TopLevel is
	
	-- FSM states
	type StateType is (
		STATE_IDLE,
		STATE_GET_COUNT0,
		STATE_GET_COUNT1,
		STATE_GET_COUNT2,
		STATE_GET_COUNT3,
		STATE_BEGIN_WRITE,
		STATE_WRITE,
		STATE_WRITE_WAIT,
		STATE_END_WRITE_ALIGNED,
		STATE_END_WRITE_NONALIGNED,
		STATE_READ,
		STATE_READ_WAIT
	);
	type MStateType is (
		MSTATE_IDLE,
		MSTATE_MEM_READ,
		MSTATE_WAIT_READ,
		MSTATE_MEM_WRITE,
		MSTATE_WAIT_WRITE,
		MSTATE_REG_WRITE
	);
	type HStateType is (
		HSTATE_IDLE,
		HSTATE_WAIT_LOOPING,
		HSTATE_ACK_LOOPING
	);
	
	-- FSM state & clocks
	signal state, state_next                 : StateType;
	signal mstate, mstate_next               : MStateType;
	signal hstate, hstate_next               : HStateType;
	signal clk48, clk96, clk96Valid          : std_logic;
	signal clkDiv, clkDiv_next               : std_logic_vector(7 downto 0);

	-- FX2LP register read/write
	signal fifoOp                            : std_logic_vector(2 downto 0);
	signal regAddr, regAddr_next             : std_logic_vector(2 downto 0);  -- up to seven bits available
	signal isWrite, isWrite_next             : std_logic;
	signal isAligned, isAligned_next         : std_logic;
	signal hostByteCount, hostByteCount_next : unsigned(31 downto 0);  -- Read/Write count
	signal r4, r4_next                       : std_logic_vector(7 downto 0);
	
	-- Memory read/write
	signal hostAddr, hostAddr_next           : std_logic_vector(22 downto 0);
	signal hostData, hostData_next           : std_logic_vector(15 downto 0);
	signal selectByte, selectByte_next       : std_logic;
	signal hostMemOp, mdMemOp, memOp         : std_logic_vector(1 downto 0);
	signal memAddr                           : std_logic_vector(22 downto 0);
	signal memInData, memOutData             : std_logic_vector(15 downto 0);
	signal memBusy                           : std_logic;
	signal memReset                          : std_logic;
	signal hostOwnsMemory                    : std_logic;
	
	-- MegaDrive signals
	signal mdReset                           : std_logic;
	signal mdRead, mdReadSync                : std_logic;
	signal mdWrite, mdWriteSync              : std_logic;
	signal mdReg00, mdReg01                  : std_logic;
	signal mdDriveBus                        : std_logic;
	signal mdDataOut                         : std_logic_vector(15 downto 0);
	signal mdReg00Sync, mdReg01Sync          : std_logic;
	signal mdDataSync                        : std_logic_vector(15 downto 0);
	signal sevenSegData, sevenSegData_next   : std_logic_vector(15 downto 0);
	signal mdOpcode                          : std_logic_vector(15 downto 0);
	signal mdIsLooping                       : std_logic;
	signal brkAddr, brkAddr_next             : std_logic_vector(22 downto 0);
	signal brkEnabled, brkEnabled_next       : std_logic;
	
	-- Constants
	constant MEM_READ                        : std_logic_vector(1 downto 0) := "11";   -- read, req
	constant MEM_WRITE                       : std_logic_vector(1 downto 0) := "01";   -- write, req
	constant MEM_NOP                         : std_logic_vector(1 downto 0) := "00";   -- xxx, no req
	constant FIFO_READ                       : std_logic_vector(2 downto 0) := "100";  -- assert slrd_out & sloe_out
	constant FIFO_WRITE                      : std_logic_vector(2 downto 0) := "011";  -- assert slwr_out
	constant FIFO_NOP                        : std_logic_vector(2 downto 0) := "111";  -- assert nothing
	constant OUT_FIFO                        : std_logic_vector(1 downto 0) := "10";   -- EP6OUT
	constant IN_FIFO                         : std_logic_vector(1 downto 0) := "11";   -- EP8IN
	constant MD_READING                      : std_logic := '1';
	constant MD_WRITING                      : std_logic := '0';
	constant FLAG_RESET                      : integer := 0;
	constant FLAG_PAUSE                      : integer := 1;
	
begin

	-- All change!
	process(clk48, reset_in)
	begin
		if ( reset_in = '1' ) then
			state         <= STATE_IDLE;
			hostByteCount <= (others => '0');
			regAddr       <= (others => '0');
			isWrite       <= '0';
			selectByte    <= '0';
			isAligned     <= '0';
			hostData      <= (others => '0');
			hostAddr      <= (others => '0');
			brkAddr       <= (others => '0');
			brkEnabled    <= '0';
			r4            <= (others => '0');
			clkDiv        <= (others => '0');
			mstate        <= MSTATE_IDLE;
			mdReadSync    <= '1';
			mdWriteSync    <= '1';
			mdReg00Sync   <= '0';
			mdReg01Sync   <= '0';
			mdDataSync    <= (others => '0');
			sevenSegData  <= (others => '0');
			hstate        <= HSTATE_IDLE;
		elsif ( clk48'event and clk48 = '1' ) then
			state         <= state_next;
			hostByteCount <= hostByteCount_next;
			regAddr       <= regAddr_next;
			isWrite       <= isWrite_next;
			selectByte    <= selectByte_next;
			isAligned     <= isAligned_next;
			hostData      <= hostData_next;
			hostAddr      <= hostAddr_next;
			brkAddr       <= brkAddr_next;
			brkEnabled    <= brkEnabled_next;
			r4            <= r4_next;
			clkDiv        <= clkDiv_next;
			mstate        <= mstate_next;
			mdReadSync    <= mdRead;
			mdWriteSync   <= mdWrite;
			mdReg00Sync   <= mdReg00;
			mdReg01Sync   <= mdReg01;
			mdDataSync    <= mdData_io;
			sevenSegData  <= sevenSegData_next;
			hstate        <= hstate_next;
		end if;
	end process;

	-- Next state logic
	process(
		state, fifoData_io, gotData_in, gotRoom_in, hostByteCount, regAddr, r4, mdIsLooping,
		hostData, hostAddr, brkAddr, brkEnabled, isAligned, isWrite, memBusy, memOutData, selectByte)
	begin
		state_next         <= state;
		hostByteCount_next <= hostByteCount;    -- how many bytes of this operation remain?
		regAddr_next       <= regAddr;          -- which register is this operation for?
		isWrite_next       <= isWrite;          -- is this to be a FIFO write?
		selectByte_next    <= selectByte;       -- are we dealing with the odd or even byte of this word?
		isAligned_next     <= isAligned;        -- does this FIFO write end on a block (512-byte) boundary?
		hostData_next      <= hostData;         -- keep host data unless explicitly updated
		hostAddr_next      <= hostAddr;         -- keep host address unless explicitly updated
		brkAddr_next       <= brkAddr;          -- keep breakpoint address unless explicitly updated
		brkEnabled_next    <= brkEnabled;       -- keep breakpoint enabled flag unless explicitly updated
		r4_next            <= r4;
		fifoData_io        <= (others => 'Z');  -- tristated unless explicitly driven
		hostMemOp          <= MEM_NOP;          -- memctrl idle by default
		fifoOp             <= FIFO_READ;        -- read the FX2LP FIFO by default
		pktEnd_out         <= '1';              -- inactive: FPGA does not commit a short packet.

		case state is

			-- Idle: wait for a command from the host
			when STATE_IDLE =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The read/write flag and a seven-bit register address will be available on the
					-- next clock edge.
					regAddr_next <= fifoData_io(2 downto 0);
					isWrite_next <= fifoData_io(7);
					state_next   <= STATE_GET_COUNT0;
				end if;
				
			-- Get most-significant byte of the count
			when STATE_GET_COUNT0 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count high word high byte will be available on the next clock edge.
					hostByteCount_next(31 downto 24) <= unsigned(fifoData_io);
					state_next <= STATE_GET_COUNT1;
				end if;

			-- Get next byte of the count
			when STATE_GET_COUNT1 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count high word low byte will be available on the next clock edge.
					hostByteCount_next(23 downto 16) <= unsigned(fifoData_io);
					state_next <= STATE_GET_COUNT2;
				end if;

			-- Get next byte of the count
			when STATE_GET_COUNT2 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count low word high byte will be available on the next clock edge.
					hostByteCount_next(15 downto 8) <= unsigned(fifoData_io);
					state_next <= STATE_GET_COUNT3;
				end if;

			-- Get least-significant byte of the count
			when STATE_GET_COUNT3 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count low word low byte will be available on the next clock edge.
					hostByteCount_next(7 downto 0) <= unsigned(fifoData_io);
					if ( isWrite = '1' ) then
						state_next <= STATE_BEGIN_WRITE;
					else
						state_next <= STATE_READ;
					end if;
				end if;

			-- Give FX2LP a chance to put fifoData_io in hi-Z state
			when STATE_BEGIN_WRITE =>
				fifoAddr_out <= IN_FIFO;  -- writing to FX2LP
				fifoOp       <= FIFO_NOP;
				isAligned_next <= not(hostByteCount(0) or hostByteCount(1) or hostByteCount(2) or hostByteCount(3) or hostByteCount(4) or hostByteCount(5) or hostByteCount(6) or hostByteCount(7) or hostByteCount(8));
				state_next   <= STATE_WRITE;

			-- Read from registers or RAM and write to FIFO
			when STATE_WRITE =>
				fifoAddr_out <= IN_FIFO;  -- writing to FX2LP
				if ( gotRoom_in = '1' ) then
					-- The state of fifoData_io will be pushed to the FIFO on the next clock edge.
					fifoOp      <= FIFO_WRITE;
					hostByteCount_next  <= hostByteCount - 1;  -- dec count by default
					if ( hostByteCount = 1 ) then
						if ( isAligned = '1' ) then
							state_next <= STATE_END_WRITE_ALIGNED;  -- don't assert pktEnd
						else
							state_next <= STATE_END_WRITE_NONALIGNED;  -- assert pktEnd to commit small packet
						end if;
					end if;
					case regAddr is
						when "000" =>
							if ( selectByte = '0' ) then
								-- Even byte - we need to get a new word from memctrl
								fifoOp              <= FIFO_NOP;          -- insert wait state
								hostByteCount_next  <= hostByteCount;     -- stop the countdown
								state_next          <= STATE_WRITE_WAIT;  -- wait until memctrl read has finished
								hostMemOp           <= MEM_READ;          -- start memctrl reading...
							else
								-- Odd byte - use MSB of previously-read word
								fifoData_io <= memOutData(7 downto 0);
							end if;
							selectByte_next <= not(selectByte);
						when "001" =>
							fifoData_io <= "0" & hostAddr(22 downto 16);  -- get addr high byte
						when "010" =>
							fifoData_io <= hostAddr(15 downto 8);   -- get addr mid byte
						when "011" =>
							fifoData_io <= hostAddr(7 downto 0);    -- get addr low byte
						when "100" =>
							fifoData_io <= "00000" & mdIsLooping & r4(1 downto 0);
						when "101" =>
							fifoData_io <= brkEnabled & brkAddr(22 downto 16);  -- get addr high byte
						when "110" =>
							fifoData_io <= brkAddr(15 downto 8);   -- get addr mid byte
						when others =>
							fifoData_io <= brkAddr(7 downto 0);    -- get addr low byte
					end case;
				else
					fifoOp <= FIFO_NOP;
				end if;

			-- Wait until the RAM read completes
			when STATE_WRITE_WAIT =>
				fifoAddr_out <= IN_FIFO;  -- writing to FX2LP
				if ( memBusy = '1' ) then
					state_next <= STATE_WRITE_WAIT;  -- loopback
					fifoOp     <= FIFO_NOP;  -- insert wait state
				else
					state_next         <= STATE_WRITE;
					fifoOp             <= FIFO_WRITE;
					hostAddr_next      <= std_logic_vector(unsigned(hostAddr) + 1);
					fifoData_io        <= memOutData(15 downto 8);
					hostByteCount_next <= hostByteCount - 1;
				end if;
				
			-- The FIFO write ends on a block boundary, so no pktEnd assertion
			when STATE_END_WRITE_ALIGNED =>
				fifoAddr_out <= IN_FIFO;   -- writing to FX2LP
				fifoOp       <= FIFO_NOP;
				pktEnd_out   <= '1';       -- inactive: aligned write, don't commit early.
				state_next   <= STATE_IDLE;

			-- The FIFO write does not end on a block boundary, so we must assert pktEnd
			when STATE_END_WRITE_NONALIGNED =>
				fifoAddr_out <= IN_FIFO;   -- writing to FX2LP
				fifoOp       <= FIFO_NOP;
				pktEnd_out   <= '0';       -- active: FPGA commits the short packet.
				state_next   <= STATE_IDLE;

			-- Read from FIFO and write to registers or RAM
			when STATE_READ =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- There is a data byte available on fifoData_io.
					hostByteCount_next  <= hostByteCount - 1;
					if ( hostByteCount = 1 ) then
						state_next <= STATE_IDLE;
					end if;
					case regAddr is
						when "000" =>
							if ( selectByte = '0' ) then
								-- Even byte - remember LSB
								hostData_next(15 downto 8) <= fifoData_io;   -- remember LSB
							else
								-- Odd byte - tell memctrl to write this byte & LSB (previously-remembered)
								fifoOp             <= FIFO_NOP;             -- insert wait state
								hostData_next(7 downto 0) <= fifoData_io;   -- remember MSB
								hostByteCount_next <= hostByteCount;        -- don't decrement count
								hostMemOp          <= MEM_WRITE;            -- ask memctrl to write hostData to hostAddr
								state_next         <= STATE_READ_WAIT;      -- and wait until it has finished
							end if;
							selectByte_next <= not(selectByte);
						when "001" =>
							hostAddr_next(22 downto 16) <= fifoData_io(6 downto 0);  -- set addr high byte
						when "010" =>
							hostAddr_next(15 downto 8) <= fifoData_io;   -- set addr mid byte
						when "011" =>
							hostAddr_next(7 downto 0) <= fifoData_io;    -- set addr low byte
						when "100" =>
							r4_next <= fifoData_io;
						when "101" =>
							brkAddr_next(22 downto 16) <= fifoData_io(6 downto 0);  -- set addr high byte
							brkEnabled_next <= fifoData_io(7);
						when "110" =>
							brkAddr_next(15 downto 8) <= fifoData_io;   -- set addr mid byte
						when others =>
							brkAddr_next(7 downto 0) <= fifoData_io;    -- set addr low byte
					end case;
				end if;

			-- Wait until the RAM write completes
			when STATE_READ_WAIT =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( memBusy = '1' ) then
					state_next    <= STATE_READ_WAIT;  -- loopback
					fifoOp        <= FIFO_NOP;  -- insert wait state
				else
					state_next    <= STATE_READ;
					fifoOp        <= FIFO_READ;
					hostAddr_next <= std_logic_vector(unsigned(hostAddr) + 1);
					hostByteCount_next <= hostByteCount - 1;
					if ( hostByteCount = 1 ) then
						state_next <= STATE_IDLE;  -- This was the final word
					end if;
				end if;
		end case;
	end process;

	-- Bus arbitration state machine
	process(mstate, mdReadSync, mdWriteSync, mdReg00Sync, mdReg01Sync, mdDataSync, sevenSegData) begin
		mdMemOp <= MEM_NOP;
		sevenSegData_next <= sevenSegData;
		case mstate is
			when MSTATE_IDLE =>
				if ( mdReadSync = '1' ) then
					mstate_next <= MSTATE_MEM_READ;
				elsif ( mdWriteSync = '1' ) then
					mstate_next <= MSTATE_MEM_WRITE;
				elsif ( mdReg01Sync = '1' ) then
					mstate_next <= MSTATE_REG_WRITE;
				else
					mstate_next <= MSTATE_IDLE;
				end if;

			-- Memory read
			when MSTATE_MEM_READ =>
				mdMemOp <= MEM_READ;
				mstate_next <= MSTATE_WAIT_READ;
			when MSTATE_WAIT_READ =>
				if ( mdReadSync = '1' ) then
					mstate_next <= MSTATE_WAIT_READ;
				else
					mstate_next <= MSTATE_IDLE;
				end if;

			-- Memory write
			when MSTATE_MEM_WRITE =>
				mdMemOp <= MEM_WRITE;
				mstate_next <= MSTATE_WAIT_WRITE;
			when MSTATE_WAIT_WRITE =>
				if ( mdWriteSync = '1' ) then
					mstate_next <= MSTATE_WAIT_WRITE;
				else
					mstate_next <= MSTATE_IDLE;
				end if;

			-- Register write
			when MSTATE_REG_WRITE =>
				if ( mdReg01Sync = '1' ) then
					mstate_next <= MSTATE_REG_WRITE;
				else
					sevenSegData_next <= mdDataSync;
					mstate_next <= MSTATE_IDLE;
				end if;
		end case;
	end process;

	-- Host communication state machine
	process(hstate, mdReg00Sync, r4) begin
		hstate_next <= hstate;
		mdOpcode <= x"60FE";  -- "bra.s -2"
		mdIsLooping <= '0';
		case hstate is
			when HSTATE_IDLE =>
				mdOpcode <= x"4E75";  -- "rts" by default
				if ( r4(FLAG_PAUSE) = '1' and mdReg00Sync = '0' ) then
					hstate_next <= HSTATE_WAIT_LOOPING;
				end if;
			when HSTATE_WAIT_LOOPING =>
				if ( mdReg00Sync = '1' ) then
					hstate_next <= HSTATE_ACK_LOOPING;
				end if;
			when HSTATE_ACK_LOOPING =>
				mdIsLooping <= '1';
				if ( r4(FLAG_PAUSE) = '0' and mdReg00Sync = '0' ) then
					hstate_next <= HSTATE_IDLE;
				end if;
		end case;
	end process;
				
	-- Divide the clock
	clkDiv_next <= std_logic_vector((unsigned(clkDiv) + 1));

	-- Mop up all the unused signals to prevent synthesis warnings
	sseg_out(7) <= flagA_in and int0_in;
	led_out <= r4;
	
	-- Outputs to MegaDrive
	mdReset <= not(r4(FLAG_RESET));  -- Keep MD in reset unless sw(0) is on.
	mdReset_out <= mdReset;

	-- Unsynchronised strobes for accesses to RAM and registers
	mdRead <= not(mdOE_in) and not(mdAddr_in(22));  -- Reads to cart & expansion address space.
	mdWrite <= not(mdLDSW_in) and not (mdAddr_in(22));  -- Writes to cart & expansion address space.
	mdReg00 <=
		'1' when mdAddr_in = "101" & x"09800" and mdOE_in = '0'  -- read 0xA13000
		else '0';
	mdReg01 <=
		'1' when mdAddr_in = "101" & x"09801" and mdLDSW_in = '0'  -- write 0xA13002
		else '0';

	-- The actual value to write on the data bus, and whether or not to drive it
	mdDataOut <=
		mdOpcode when mdReg00Sync = '1' else
		x"4afc" when mdAddr_in = brkAddr and brkEnabled = '1' else
		memOutData;
	mdDriveBus <= mdReadSync or mdReg00Sync;
	
	-- Drive bus & buffers. Must use same signal for muxing mdData_io as for mdBufDir_out
	mdBufOE_out <= '0';  -- Level converters always on (pulled up during FPGA programming)
	mdBufDir_out <= mdDriveBus;  -- Drive data bus only when MD reading from us.
	mdData_io <=
		mdDataOut when mdDriveBus = '1' else
		(others=>'Z');  -- Do not drive data bus

	-- Host owns the memory bus when MegaDrive is in RESET and also when it's looping
	hostOwnsMemory <= mdReset or mdIsLooping;
	
	-- Address and command for memctrl
	memAddr <=
		hostAddr when hostOwnsMemory = '1' else
		mdAddr_in;

	memInData <=
		hostData when hostOwnsMemory = '1' else		
		mdDataSync;

	memOp <=
		hostMemOp when hostOwnsMemory = '1' else
		mdMemOp;

	-- Debug lines
	jc2_out <= mdReg00;
	jc7_out <= mdReg01;
		
	-- Memory controller assignments
	memReset  <= reset_in or not clk96Valid;
	ramLB_out <= '0';  -- Never do byte operations
	ramUB_out <= '0';

	-- Breakout fifoOp
	sloe_out <= fifoOp(0);
	slrd_out <= fifoOp(1);
	slwr_out <= fifoOp(2);
	
	-- Double the IFCLK to get 96MHz
	clockGenerator: entity work.ClockGenerator
		port map (
			CLKIN_IN   => ifclk_in,    -- input 48MHz IFCLK from FX2LP
			RST_IN     => reset_in,    -- reset input
			CLKFX_OUT  => clk96,       -- output 96MHz clock for memctrl
			LOCKED_OUT => clk96Valid,  -- goes high when clk96 is stable
			CLK0_OUT   => clk48        -- buffered IFCLK
		);

	-- Generate signals for driving the CellularRAM in synchronous mode @48MHz
	memoryController: entity work.MemoryController(Behavioural)
		port map(
			-- Internal interface
			clk           => clk96,        -- clocked at 2x rate you want ramClk
			reset         => memReset,     -- reset memctrl
			memRequest    => memOp(0),     -- ask memctrl to begin an operation
			readNotWrite  => memOp(1),     -- tell memctrl whether to read or write
			busy          => memBusy,      -- mem controller is busy
			mcrAddrInput  => memAddr,      -- address you want to access
			mcrDataInput  => memInData,    -- data to be written to RAM
			mcrDataOutput => memOutData,   -- data read from RAM (registered)

			-- External interface
			ramClk        => ramClk_out,   -- driven at clk/2
			nWait         => ramWait_in,   -- RAM busy pin
			addressBus    => ramAddr_out,  -- RAM address pins
			dataBus       => ramData_io,   -- RAM data pins (bidirectional)
			nADV          => ramADV_out,   -- RAM address valid pin
			nCE           => ramCE_out,    -- RAM chip enable pin
			CRE           => ramCRE_out,   -- Control Register Enable pin
			nWE           => ramWE_out,    -- Write Enable
			nOE           => ramOE_out     -- Output Enable
		);

	-- Display the RAM data
	sevenSeg : entity work.SevenSeg
		port map(
			clk    => clkDiv(7),
			data   => sevenSegData,
			segs   => sseg_out(6 downto 0),
			anodes => anode_out
		);
end Behavioural;

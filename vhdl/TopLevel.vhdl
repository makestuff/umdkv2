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

entity TopLevel is
	port(
		ifclk_in     : in    std_logic;

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

		-- SD-Card signals
		sdDO_in      : in    std_logic;  -- Serial data from the SD card
		sdClk_out    : out   std_logic;  -- Serial clock to the SD card
		sdDI_out     : out   std_logic;  -- Serial data to the SD card
		sdCS_out     : out   std_logic;  -- SD card chip select (active-low)
		sdCD_in      : in    std_logic;  -- SD card detect (low when card inserted)

		-- MegaDrive signals
		mdReset_out  : out   std_logic;  -- while 'Z', MD stays in RESET; drive low to bring MD out of RESET.
		mdBufOE_out  : out   std_logic;  -- while 'Z', MD is isolated; drive low to enable all three buffers.
		mdBufDir_out : out   std_logic;  -- drive high when MegaDrive reading, low when MegaDrive writing.
		mdOE_in      : in    std_logic;  -- active low output enable: asserted when MD reads.
		mdLDSW_in    : in    std_logic;  -- active low output enable: asserted when MD writes.
		mdAddr_in    : in    std_logic_vector(22 downto 0);
		mdData_io    : inout std_logic_vector(15 downto 0)
	);
end TopLevel;

architecture Behavioural of TopLevel is
	
	type HStateType is (
		HSTATE_IDLE,
		HSTATE_GET_COUNT0,
		HSTATE_GET_COUNT1,
		HSTATE_GET_COUNT2,
		HSTATE_GET_COUNT3,
		HSTATE_BEGIN_WRITE,
		HSTATE_WRITE,
		HSTATE_WRITE_HOLD,
		HSTATE_WRITE_WAIT,
		HSTATE_END_WRITE_ALIGNED,
		HSTATE_END_WRITE_NONALIGNED,
		HSTATE_READ,
		HSTATE_READ_HOLD,
		HSTATE_READ_WAIT
	);
	type MStateType is (
		MSTATE_IDLE,
		MSTATE_WAIT_READ,
		MSTATE_WAIT_WRITE
	);
	type AStateType is (
		ASTATE_IDLE,
		ASTATE_RESET,
		ASTATE_WAIT_LOOPING,
		ASTATE_ACK_LOOPING
	);

	-- Constants
	constant MEM_READ         : std_logic_vector(1 downto 0) := "11";   -- read, req
	constant MEM_WRITE        : std_logic_vector(1 downto 0) := "01";   -- write, req
	constant MEM_NOP          : std_logic_vector(1 downto 0) := "00";   -- xxx, no req
	constant FIFO_READ        : std_logic_vector(2 downto 0) := "100";  -- assert slrd_out & sloe_out
	constant FIFO_WRITE       : std_logic_vector(2 downto 0) := "011";  -- assert slwr_out
	constant FIFO_NOP         : std_logic_vector(2 downto 0) := "111";  -- assert nothing
	constant OUT_FIFO         : std_logic_vector(1 downto 0) := "10";   -- EP6OUT
	constant IN_FIFO          : std_logic_vector(1 downto 0) := "11";   -- EP8IN
	constant FLAG_RUN         : integer := 0;
	constant FLAG_PAUSE       : integer := 1;
	constant OPCODE_ILLEGAL   : std_logic_vector(15 downto 0) := x"4AFC";
	constant OPCODE_RTS       : std_logic_vector(15 downto 0) := x"4E75";
	constant OPCODE_BRA       : std_logic_vector(15 downto 0) := x"60FE";
	constant MDREG_YIELDBUS   : std_logic_vector(1 downto 0) := "00";
	constant MDREG_RAMPAGE    : std_logic_vector(1 downto 0) := "01";
	constant MDREG_SERDATA    : std_logic_vector(1 downto 0) := "10";
	constant MDREG_SERCTRL    : std_logic_vector(1 downto 0) := "11";

	-- FSM state & clocks
	signal hstate             : HStateType := HSTATE_IDLE; -- host interface state
	signal hstate_next        : HStateType := HSTATE_IDLE;
	signal mstate             : MStateType := MSTATE_IDLE; -- m68k interface state
	signal mstate_next        : MStateType := MSTATE_IDLE;
	signal astate             : AStateType := ASTATE_IDLE; -- arbitration state
	signal astate_next        : AStateType := ASTATE_IDLE;
	signal clk48              : std_logic;
	signal clk96              : std_logic;
	signal clk96Valid         : std_logic;

	-- FX2LP register read/write
	signal fifoOp             : std_logic_vector(2 downto 0);
	signal regAddr            : std_logic_vector(2 downto 0) := "000";  -- up to seven bits available
	signal regAddr_next       : std_logic_vector(2 downto 0) := "000";  -- up to seven bits available
	signal isWrite            : std_logic := '0';
	signal isWrite_next       : std_logic := '0';
	signal isAligned          : std_logic := '0';
	signal isAligned_next     : std_logic := '0';
	signal hostByteCount      : unsigned(31 downto 0) := x"00000000";  -- Read/Write count
	signal hostByteCount_next : unsigned(31 downto 0) := x"00000000";  -- Read/Write count
	signal r4                 : std_logic_vector(1 downto 0) := "00";
	signal r4_next            : std_logic_vector(1 downto 0) := "00";

	-- Memory read/write
	signal memOp              : std_logic_vector(1 downto 0);
	signal mdMemOp            : std_logic_vector(1 downto 0) := MEM_NOP;
	signal mdMemOp_next       : std_logic_vector(1 downto 0) := MEM_NOP;
	signal hostMemOp          : std_logic_vector(1 downto 0) := MEM_NOP;
	signal hostMemOp_next     : std_logic_vector(1 downto 0) := MEM_NOP;
	signal memAddr            : std_logic_vector(22 downto 0);
	signal memInData          : std_logic_vector(15 downto 0);
	signal memOutData         : std_logic_vector(15 downto 0);
	signal memBusy            : std_logic;
	signal memReset           : std_logic;
	signal hostAddr           : std_logic_vector(22 downto 0) := "000" & x"00000";
	signal hostAddr_next      : std_logic_vector(22 downto 0) := "000" & x"00000";
	signal hostData           : std_logic_vector(15 downto 0) := x"0000";
	signal hostData_next      : std_logic_vector(15 downto 0) := x"0000";
	signal selectByte         : std_logic := '0';
	signal selectByte_next    : std_logic := '0';

	-- SD card signals
	signal sdSendData         : std_logic_vector(7 downto 0);
	signal sdRecvData         : std_logic_vector(7 downto 0);
	signal sdLoad             : std_logic;
	signal sdTurbo            : std_logic;
	signal sdBusy             : std_logic;
	signal serCtrl            : std_logic_vector(1 downto 0) := "00";
	signal serCtrl_next       : std_logic_vector(1 downto 0) := "00";

	-- MegaDrive signals
	signal mdAccessMem        : std_logic;
	signal mdAccessIO         : std_logic;
	signal mdBeginRead        : std_logic;
	signal mdBeginWrite       : std_logic;
	signal mdSyncOE           : std_logic;
	signal mdSyncWE           : std_logic;
	signal mdHasYielded       : std_logic;
	signal mdReadData         : std_logic_vector(15 downto 0);
	signal mdOpcode           : std_logic_vector(15 downto 0);
	signal brkAddr            : std_logic_vector(22 downto 0) := "000" & x"00000";
	signal brkAddr_next       : std_logic_vector(22 downto 0) := "000" & x"00000";
	signal brkEnabled         : std_logic := '0';
	signal brkEnabled_next    : std_logic := '0';

begin

	------------------------------------------------------------------------------------------------
	-- All change!
	process(clk48)
	begin
		if ( clk48'event and clk48 = '1' ) then
			mstate        <= mstate_next;
			mdMemOp       <= mdMemOp_next;
			hostMemOp     <= hostMemOp_next;
			serCtrl       <= serCtrl_next;
			hstate        <= hstate_next;
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
			astate        <= astate_next;
		end if;
	end process;

	
	------------------------------------------------------------------------------------------------
	-- Host interface state machine
	process(
		hstate,
		fifoData_io,
		gotData_in,
		gotRoom_in,
		hostByteCount,
		regAddr,
		r4,
		mdHasYielded,
		hostData,
		hostAddr,
		brkAddr,
		brkEnabled,
		isAligned,
		isWrite,
		memBusy,
		memOutData,
		selectByte,
		hostMemOp
	)
	begin
		hstate_next        <= hstate;
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
		hostMemOp_next     <= MEM_NOP;          -- memctrl idle by default
		fifoOp             <= FIFO_READ;        -- read the FX2LP FIFO by default
		pktEnd_out         <= '1';              -- inactive: FPGA does not commit a short packet.

		case hstate is

			-- Idle: wait for a command from the host
			when HSTATE_IDLE =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The read/write flag and a seven-bit register address will be available on the
					-- next clock edge.
					regAddr_next <= fifoData_io(2 downto 0);
					isWrite_next <= fifoData_io(7);
					hstate_next  <= HSTATE_GET_COUNT0;
				end if;
				
			-- Get most-significant byte of the count
			when HSTATE_GET_COUNT0 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count high word high byte will be available on the next clock edge.
					hostByteCount_next(31 downto 24) <= unsigned(fifoData_io);
					hstate_next <= HSTATE_GET_COUNT1;
				end if;

			-- Get next byte of the count
			when HSTATE_GET_COUNT1 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count high word low byte will be available on the next clock edge.
					hostByteCount_next(23 downto 16) <= unsigned(fifoData_io);
					hstate_next <= HSTATE_GET_COUNT2;
				end if;

			-- Get next byte of the count
			when HSTATE_GET_COUNT2 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count low word high byte will be available on the next clock edge.
					hostByteCount_next(15 downto 8) <= unsigned(fifoData_io);
					hstate_next <= HSTATE_GET_COUNT3;
				end if;

			-- Get least-significant byte of the count
			when HSTATE_GET_COUNT3 =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- The count low word low byte will be available on the next clock edge.
					hostByteCount_next(7 downto 0) <= unsigned(fifoData_io);
					if ( isWrite = '1' ) then
						hstate_next <= HSTATE_BEGIN_WRITE;
					else
						hstate_next <= HSTATE_READ;
					end if;
				end if;

			-- Give FX2LP a chance to put fifoData_io in hi-Z state
			when HSTATE_BEGIN_WRITE =>
				fifoAddr_out <= IN_FIFO;  -- writing to FX2LP
				fifoOp       <= FIFO_NOP;
				isAligned_next <= not(hostByteCount(0) or hostByteCount(1) or hostByteCount(2) or hostByteCount(3) or hostByteCount(4) or hostByteCount(5) or hostByteCount(6) or hostByteCount(7) or hostByteCount(8));
				hstate_next  <= HSTATE_WRITE;

			-- Read from registers or RAM and write to FIFO
			when HSTATE_WRITE =>
				fifoAddr_out <= IN_FIFO;  -- writing to FX2LP
				if ( gotRoom_in = '1' ) then
					-- The state of fifoData_io will be pushed to the FIFO on the next clock edge.
					fifoOp      <= FIFO_WRITE;
					hostByteCount_next  <= hostByteCount - 1;  -- dec count by default
					if ( hostByteCount = 1 ) then
						if ( isAligned = '1' ) then
							hstate_next <= HSTATE_END_WRITE_ALIGNED;  -- don't assert pktEnd
						else
							hstate_next <= HSTATE_END_WRITE_NONALIGNED;  -- assert pktEnd to commit small packet
						end if;
					end if;
					case regAddr is
						when "000" =>
							if ( selectByte = '0' ) then
								-- Even byte - we need to get a new word from memctrl
								fifoOp             <= FIFO_NOP;          -- insert wait state
								hostByteCount_next <= hostByteCount;     -- stop the countdown
								hstate_next        <= HSTATE_WRITE_HOLD; -- wait until memctrl read has finished
								hostMemOp_next     <= MEM_READ;          -- start memctrl reading...
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
							fifoData_io <= "00000" & mdHasYielded & r4;
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

			-- Hold a while...
			when HSTATE_WRITE_HOLD =>
				fifoAddr_out   <= IN_FIFO;           -- writing to FX2LP
				fifoOp         <= FIFO_NOP;          -- insert wait state
				hstate_next    <= HSTATE_WRITE_WAIT; -- wait until memctrl read has finished
				hostMemOp_next <= MEM_READ;          -- start memctrl reading...
			
			-- Wait until the RAM read completes
			when HSTATE_WRITE_WAIT =>
				fifoAddr_out <= IN_FIFO;  -- writing to FX2LP
				if ( memBusy = '1' ) then
					hstate_next <= HSTATE_WRITE_WAIT;  -- loopback
					fifoOp      <= FIFO_NOP;  -- insert wait state
				else
					hstate_next        <= HSTATE_WRITE;
					fifoOp             <= FIFO_WRITE;
					hostAddr_next      <= std_logic_vector(unsigned(hostAddr) + 1);
					fifoData_io        <= memOutData(15 downto 8);
					hostByteCount_next <= hostByteCount - 1;
				end if;
				
			-- The FIFO write ends on a block boundary, so no pktEnd assertion
			when HSTATE_END_WRITE_ALIGNED =>
				fifoAddr_out <= IN_FIFO;   -- writing to FX2LP
				fifoOp       <= FIFO_NOP;
				pktEnd_out   <= '1';       -- inactive: aligned write, don't commit early.
				hstate_next  <= HSTATE_IDLE;

			-- The FIFO write does not end on a block boundary, so we must assert pktEnd
			when HSTATE_END_WRITE_NONALIGNED =>
				fifoAddr_out <= IN_FIFO;   -- writing to FX2LP
				fifoOp       <= FIFO_NOP;
				pktEnd_out   <= '0';       -- active: FPGA commits the short packet.
				hstate_next  <= HSTATE_IDLE;

			-- Read from FIFO and write to registers or RAM
			when HSTATE_READ =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( gotData_in = '1' ) then
					-- There is a data byte available on fifoData_io.
					hostByteCount_next  <= hostByteCount - 1;
					if ( hostByteCount = 1 ) then
						hstate_next <= HSTATE_IDLE;
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
								hostMemOp_next     <= MEM_WRITE;            -- ask memctrl to write hostData to hostAddr
								hstate_next        <= HSTATE_READ_HOLD;     -- and wait until it has finished
							end if;
							selectByte_next <= not(selectByte);
						when "001" =>
							hostAddr_next(22 downto 16) <= fifoData_io(6 downto 0);  -- set addr high byte
						when "010" =>
							hostAddr_next(15 downto 8) <= fifoData_io;   -- set addr mid byte
						when "011" =>
							hostAddr_next(7 downto 0) <= fifoData_io;    -- set addr low byte
						when "100" =>
							r4_next <= fifoData_io(1 downto 0);
						when "101" =>
							brkAddr_next(22 downto 16) <= fifoData_io(6 downto 0);  -- set addr high byte
							brkEnabled_next <= fifoData_io(7);
						when "110" =>
							brkAddr_next(15 downto 8) <= fifoData_io;   -- set addr mid byte
						when others =>
							brkAddr_next(7 downto 0) <= fifoData_io;    -- set addr low byte
					end case;
				end if;

			-- Hold a while...
			when HSTATE_READ_HOLD =>
				fifoAddr_out   <= OUT_FIFO;         -- reading from FX2LP
				fifoOp         <= FIFO_NOP;         -- insert wait state
				hostMemOp_next <= MEM_WRITE;        -- ask memctrl to write hostData to hostAddr
				hstate_next    <= HSTATE_READ_WAIT; -- and wait until it has finished
				
			-- Wait until the RAM write completes
			when HSTATE_READ_WAIT =>
				fifoAddr_out <= OUT_FIFO;  -- reading from FX2LP
				if ( memBusy = '1' ) then
					hstate_next   <= HSTATE_READ_WAIT;  -- loopback
					fifoOp        <= FIFO_NOP;  -- insert wait state
				else
					hstate_next   <= HSTATE_READ;
					fifoOp        <= FIFO_READ;
					hostAddr_next <= std_logic_vector(unsigned(hostAddr) + 1);
					hostByteCount_next <= hostByteCount - 1;
					if ( hostByteCount = 1 ) then
						hstate_next <= HSTATE_IDLE;  -- This was the final word
					end if;
				end if;
		end case;
	end process;

	-- Breakout fifoOp
	sloe_out <= fifoOp(0);
	slrd_out <= fifoOp(1);
	slwr_out <= fifoOp(2);

	
	------------------------------------------------------------------------------------------------
	-- M68K state machine
	process(
		mstate,
		mdSyncOE,
		mdSyncWE,
		mdAccessMem,
		mdAccessIO,
		mdBeginRead,
		mdBeginWrite,
		mdMemOp,
		serCtrl,
		mdData_io,
		memOutData,
		mdAddr_in,
		brkAddr,
		brkEnabled,
		mdOpcode,
		sdCD_in,
		sdBusy,
		sdRecvData
	)
	begin
		mstate_next <= mstate;
		mdMemOp_next <= MEM_NOP;
		serCtrl_next <= serCtrl;
		mdReadData <= (others => '0');
		sdSendData <= (others => '0');
		sdLoad <= '0';
		case mstate is
			when MSTATE_IDLE =>
				if ( mdAccessMem = '1' ) then
					if ( mdBeginRead = '1' ) then
						mstate_next <= MSTATE_WAIT_READ;
						mdMemOp_next <=	MEM_READ;
					elsif ( mdBeginWrite = '1' ) then
						mstate_next <= MSTATE_WAIT_WRITE;
						mdMemOp_next <= MEM_WRITE;
					end if;
				elsif ( mdAccessIO = '1' ) then
					if ( mdBeginRead = '1' ) then
						mstate_next <= MSTATE_WAIT_READ;
						mdMemOp_next <=	MEM_READ;
					elsif ( mdBeginWrite = '1' ) then
						mstate_next <= MSTATE_WAIT_WRITE;
						case mdAddr_in(1 downto 0) is
							--when MDREG_YIELDBUS =>
								-- Writing to YIELDBUS updates ssData
							--	ssData_next <= mdData_io;
							when MDREG_SERDATA =>
								sdSendData <= mdData_io(7 downto 0);
								sdLoad <= '1';
							when MDREG_SERCTRL =>
								serCtrl_next <= mdData_io(1 downto 0);
							when MDREG_RAMPAGE =>
							when others =>
						end case;
					end if;
				end if;

			when MSTATE_WAIT_READ =>
				if ( mdAccessMem = '1' ) then
					if ( mdAddr_in = brkAddr and brkEnabled = '1' ) then
						mdReadData <= OPCODE_ILLEGAL;
					else
						mdReadData <= memOutData;
					end if;
				elsif ( mdAccessIO = '1' ) then
					case mdAddr_in(1 downto 0) is
						when MDREG_YIELDBUS =>
							-- Reading from YIELDBUS gives the 68k opcode for rts or bra.s -2
							mdReadData <= mdOpcode;
						when MDREG_SERCTRL =>
							-- Reading from SERCTRL gives flags & status
							mdReadData <= x"000" & not(sdCD_in) & sdBusy & serCtrl;
						when MDREG_SERDATA =>
							mdReadData <= x"00" & sdRecvData;
						when MDREG_RAMPAGE =>
						when others =>
							mdReadData <= x"DEAD";
					end case;
				end if;
				if ( mdSyncOE = '0' ) then
					mstate_next <= MSTATE_IDLE;
				end if;

			when MSTATE_WAIT_WRITE =>
				if ( mdSyncWE = '0' ) then
					mstate_next <= MSTATE_IDLE;
				end if;
		end case;
	end process;

	sdTurbo <= serCtrl(0);
	sdCS_out <= not(serCtrl(1));

	
	------------------------------------------------------------------------------------------------
	-- Arbitration state machine
	process(
		astate, r4, mdSyncOE, mdAccessIO,
		mdAddr_in, mdData_io, mdMemOp,
		hostAddr, hostData, hostMemOp
	)
	begin
		astate_next <= astate;
		mdOpcode <= OPCODE_BRA;  -- "bra.s -2"
		mdHasYielded <= '0';
		memAddr <= mdAddr_in;
		memInData <= mdData_io;
		memOp <= mdMemOp;
		case astate is
			when ASTATE_IDLE =>
				mdOpcode <= OPCODE_RTS;  -- "rts" by default
				if ( r4(FLAG_RUN) = '0' ) then
					astate_next <= ASTATE_RESET;
				elsif ( r4(FLAG_PAUSE) = '1' and mdSyncOE = '0' ) then
					astate_next <= ASTATE_WAIT_LOOPING;
				end if;

			-- The 68k is in RESET, so the host interface has free reign
			when ASTATE_RESET =>
				mdHasYielded <= '1';
				memAddr <= hostAddr;
				memInData <= hostData;
				memOp <= hostMemOp;
				if ( r4(FLAG_RUN) = '1' ) then
					astate_next <= ASTATE_IDLE;
				end if;

			-- The we've started serving bra.s -2 at reg00, wait until the 68k accesses it
			when ASTATE_WAIT_LOOPING =>
				if ( mdSyncOE = '1' and mdAccessIO = '1' and mdAddr_in(1 downto 0) = "00" ) then
					astate_next <= ASTATE_ACK_LOOPING;
				end if;

			-- The 68k is looping at reg00, so host interface has free reign
			when ASTATE_ACK_LOOPING =>
				mdHasYielded <= '1';
				memAddr <= hostAddr;
				memInData <= hostData;
				memOp <= hostMemOp;
				if ( r4(FLAG_PAUSE) = '0' and mdSyncOE = '0' ) then
					astate_next <= ASTATE_IDLE;
				end if;
		end case;
	end process;

	
	------------------------------------------------------------------------------------------------
	-- Double the IFCLK to get 96MHz
	clockGenerator: entity work.ClockGenerator
		port map (
			RST_IN     => '0',
			CLKIN_IN   => ifclk_in,    -- input 48MHz IFCLK from FX2LP
			CLKFX_OUT  => clk96,       -- output 96MHz clock for memctrl
			LOCKED_OUT => clk96Valid,  -- goes high when clk96 is stable
			CLK0_OUT   => clk48        -- buffered IFCLK
		);

	
	------------------------------------------------------------------------------------------------
	-- Generate signals for driving the CellularRAM in synchronous mode @48MHz
	memReset  <= not clk96Valid;
	ramLB_out <= '0';  -- Never do byte operations
	ramUB_out <= '0';
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

	
	------------------------------------------------------------------------------------------------
	-- Generate signals for the 68000 bus interface and UMDKv2 buffers
	mdReset_out <= not(r4(FLAG_RUN));
	businterface: entity work.businterface
		port map(
			clk_in           => clk48,

			-- External connections
			mdBufOE_out      => mdBufOE_out,
			mdBufDir_out     => mdBufDir_out,
			mdOE_in          => mdOE_in,
			mdLDSW_in        => mdLDSW_in,
			mdAddr_in        => mdAddr_in(22 downto 2),
			mdData_io        => mdData_io,

			-- Internal connections
			mdBeginRead_out  => mdBeginRead,
			mdBeginWrite_out => mdBeginWrite,
			mdAccessMem_out  => mdAccessMem,
			mdAccessIO_out   => mdAccessIO,
			mdSyncOE_out     => mdSyncOE,
			mdSyncWE_out     => mdSyncWE,
			mdData_in        => mdReadData
		);

	
	------------------------------------------------------------------------------------------------
	-- Talk to the SD card
	serialio: entity work.serialio
		port map(
			clk_in    => clk48,
			data_in   => sdSendData,
			data_out  => sdRecvData,
			load_in   => sdLoad,
			turbo_in  => sdTurbo,
			busy_out  => sdBusy,
			sData_out => sdDI_out,
			sData_in  => sdDO_in,
			sClk_out  => sdClk_out
		);

end Behavioural;

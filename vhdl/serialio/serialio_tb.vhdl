library ieee;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all;
use std.textio.all;

entity serialio_tb is
end serialio_tb;

architecture behavioural of serialio_tb is

	signal clk      : std_logic;
	signal sendData : std_logic_vector(7 downto 0);
	signal recvData : std_logic_vector(7 downto 0);
	signal load     : std_logic;
	signal busy     : std_logic;
	signal sDataOut : std_logic;
	signal sDataIn  : std_logic;
	signal sClk     : std_logic;

begin

	-- Instantiate the unit under test
	uut: entity work.serialio
		port map(
			clk_in    => clk,
			data_in   => sendData,
			data_out  => recvData,
			load_in   => load,
			turbo_in  => '1',
			busy_out  => busy,
			sData_out => sDataOut,
			sData_in  => sDataIn,
			sClk_out  => sClk
		);

	-- Drive the clock
	process
	begin
		clk <= '0';
		wait for 5 ns;
		clk <= '1';
		wait for 5 ns;
	end process;

	-- Drive the serial interface: send from s/send.txt and receive into r/recv.txt
	process
		variable inLine, outLine : line;
		variable inData, outData : std_logic_vector(7 downto 0);
		file inFile              : text open read_mode is "stimulus/send.txt";
		file outFile             : text open write_mode is "results/recv.txt";
	begin
		sendData <= (others => 'X');
		load <= '0';
		wait for 50 ns;
		loop
			exit when endfile(inFile);
			readline(inFile, inLine);
			read(inLine, inData);
			sendData <= inData;
			load <= '1';
			wait for 10 ns;
			sendData <= (others => 'X');
			load <= '0';
			wait until busy = '0';
			outData := recvData;
			write(outLine, outData);
			writeline(outFile, outLine);
		end loop;
		wait;
		--assert false report "NONE. End of simulation." severity failure;
	end process;

	-- Mock the serial interface's interlocutor: send from s/recv.txt and receive into r/send.txt
	process
		variable inLine, outLine : line;
		variable inData, outData : std_logic_vector(7 downto 0);
		file inFile              : text open read_mode is "stimulus/recv.txt";
		file outFile             : text open write_mode is "results/send.txt";
	begin
		sDataIn <= 'X';
		loop
			exit when endfile(inFile);
			readline(inFile, inLine);
			read(inLine, inData);
			wait until sClk = '0';
			sDataIn <= inData(7);
			wait until sClk = '1';
			outData(7) := sDataOut;
			wait until sClk = '0';
			sDataIn <= inData(6);
			wait until sClk = '1';
			outData(6) := sDataOut;
			wait until sClk = '0';
			sDataIn <= inData(5);
			wait until sClk = '1';
			outData(5) := sDataOut;
			wait until sClk = '0';
			sDataIn <= inData(4);
			wait until sClk = '1';
			outData(4) := sDataOut;
			wait until sClk = '0';
			sDataIn <= inData(3);
			wait until sClk = '1';
			outData(3) := sDataOut;
			wait until sClk = '0';
			sDataIn <= inData(2);
			wait until sClk = '1';
			outData(2) := sDataOut;
			wait until sClk = '0';
			sDataIn <= inData(1);
			wait until sClk = '1';
			outData(1) := sDataOut;
			wait until sClk = '0';
			sDataIn <= inData(0);
			wait until sClk = '1';
			outData(0) := sDataOut;
			write(outLine, outData);
			writeline(outFile, outLine);
		end loop;
		wait for 10 ns;
		sDataIn <= 'X';
		wait;
	end process;
end architecture;

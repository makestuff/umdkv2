#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <argtable2.h>
#include <arg_uint.h>

#include <hal.h>
#include <reset.h>
#include <bus.h>
#include <memory.h>
#include <breakpoint.h>
#include <monitor.h>
#include <readfile.h>
#include <bigendian.h>
#include <error.h>

#define VID 0x04B4
#define PID 0x8613

void parseLine(char *line);
void parseCommand(char *command);

static char m_elfPath[4096];  // TODO: Fix, somehow.

int main(int argc, char *argv[]) {
	struct arg_uint *vidOpt  = arg_uint0("v", "vid", "<vendorID>", "  vendor ID");
	struct arg_uint *pidOpt  = arg_uint0("p", "pid", "<productID>", " product ID");
	struct arg_lit  *helpOpt = arg_lit0("h", "help", "            print this help and exit\n");
	struct arg_end  *endOpt  = arg_end(20);
	void* argTable[] = {vidOpt, pidOpt, helpOpt, endOpt};
	const char *progName = "mdsh";
	uint32 exitCode = 0;
	int numErrors;
	uint16 vid, pid;
	char *line = NULL;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		exitCode = 1;
		goto cleanup;
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("MegaDrive Command Shell Copyright (C) 2011 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with MegaDrive.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		exitCode = 0;
		goto cleanup;
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		exitCode = 2;
		goto cleanup;
	}

	vid = vidOpt->count ? (uint16)vidOpt->ival[0] : VID;
	pid = pidOpt->count ? (uint16)pidOpt->ival[0] : PID;

	if ( umdkOpen(vid, pid) != UMDK_SUCCESS ) {
		fprintf(stderr, "%s\n", umdkStrError());
		exitCode = 1; goto cleanup;
	}

	m_elfPath[0] = '\0';

	line = readline("> ");
	while ( line && strcmp(line, "quit") ) {
		add_history(line);

		parseLine(line);

		free(line);
		line = readline("> ");
	}

cleanup:
	free(line);
	umdkClose();
	arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));

	return exitCode;
}

void doReset(void) {
	umdkSetRunStatus(false);
	umdkSetRunStatus(true);
}

static void makeElfPath(const char *filename) {
	char *extension;
	strncpy(m_elfPath, filename, 4096);
	extension = m_elfPath + strlen(filename) - 4;
	if ( *extension == '.' ) {
		*++extension = 'e';
		*++extension = 'l';
		*++extension = 'f';
	} else {
		m_elfPath[0] = '\0';
	}
}

void doLoad(char **savePtr) {
	const char *filename = strtok_r(NULL, " ", savePtr);
	uint32 length;
	uint8 *buf;
	if ( !filename ) {
		fprintf(stderr, "Synopsis: load <file>\n");
		return;
	}
	makeElfPath(filename);
	buf = umdkReadFile(filename, &length);
	if ( !buf ) {
		fprintf(stderr, "Unable to load file %s!\n", filename);
		return;
	}
	umdkSetRunStatus(false);
	umdkSetBreakpointEnabled(false);

	umdkWriteLong(0x400000, buf+0x10); // illegal instruction vector
	umdkWriteLong(0x400000, buf+0x24); // trace vector
	umdkWriteMemory(0x000000, length, buf);
	free(buf);

	buf = umdkReadFile("../m68k/monitor/monitor.bin", &length);
	if ( !buf ) {
		fprintf(stderr, "Unable to load monitor file!\n");
		return;
	}
	umdkWriteMemory(0x400000, length, buf);
	free(buf);

	umdkSetRunStatus(true);
}

void dumpMachineState(const MachineState *machineState) {
	char command[4096+100];
	int returnCode;
	printf("D0=%08lX; D1=%08lX; D2=%08lX; D3=%08lX; D4=%08lX; D5=%08lX; D6=%08lX; D7=%08lX\n",
		   machineState->d0, machineState->d1, machineState->d2, machineState->d3,
		   machineState->d4, machineState->d5, machineState->d6, machineState->d7);
	printf("A0=%08lX; A1=%08lX; A2=%08lX; A3=%08lX; A4=%08lX; A5=%08lX; A6=%08lX; A7=%08lX\n",
		   machineState->a0, machineState->a1, machineState->a2, machineState->a3,
		   machineState->a4, machineState->a5, machineState->a6, machineState->a7);
	printf("\nPC=%08lX:\n", machineState->pc);
	if ( m_elfPath[0] ) {
		fflush(stdout);
		sprintf(command, "../util/scripts/disasm.sh %s 0x%08lX", m_elfPath, machineState->pc);
		returnCode = system(command);
	}
}

void doBreak(char **savePtr) {
	const char *addrStr = strtok_r(NULL, " ", savePtr);
	uint32 addrVal;
	if ( !addrStr ) {
		fprintf(stderr, "Synopsis: break <address>\n");
		return;
	}
	addrVal = strtoul(addrStr, NULL, 16);
	umdkSetBreakpointAddress(addrVal);
}

void doDisable(void) {
	umdkSetBreakpointEnabled(false);
}

void doStep(void) {
	MachineState machineState;
	umdkMonitorSingleStep();
	if ( umdkMonitorGetMachineState(&machineState) != UMDK_SUCCESS ) {
		fprintf(stderr, "Timed out waiting for control of the bus!\n");
		return;
	}
	dumpMachineState(&machineState);
}

void doCont(void) {
	umdkMonitorContinue();
}

void doDump(void) {
	MachineState machineState;
	if ( umdkMonitorGetMachineState(&machineState) != UMDK_SUCCESS ) {
		fprintf(stderr, "Timed out waiting for control of the bus!\n");
		return;
	}
	dumpMachineState(&machineState);
}

void doGet(char **savePtr) {
	const char *filename = strtok_r(NULL, " ", savePtr);
	uint8 buf[0x10000];
	FILE *file;
	if ( !filename ) {
		fprintf(stderr, "Synopsis: get <file>\n");
		return;
	}
	file = fopen(filename, "wb");
	if ( !file ) {
		fprintf(stderr, "Unable to open file %s for writing!\n", filename);
		return;
	}
	umdkMonitorGetRam(buf);
	fwrite(buf, 1, 0x10000, file);
	fclose(file);
}

void doPut(char **savePtr) {
	const char *filename = strtok_r(NULL, " ", savePtr);
	uint32 length;
	uint8 *buf;
	if ( !filename ) {
		fprintf(stderr, "Synopsis: put <file>\n");
		return;
	}
	buf = umdkReadFile(filename, &length);
	if ( !buf ) {
		fprintf(stderr, "Unable to load file %s!\n", filename);
		return;
	}
	umdkMonitorPutRam(buf);
	free(buf);
}

void doPatch(char **savePtr) {
	const char *addrStr = strtok_r(NULL, " ", savePtr);
	const char *valStr = strtok_r(NULL, " ", savePtr);
	uint32 addrVal;
	uint8 value;
	uint8 buf[0x10000];
	if ( !addrStr || !valStr ) {
		fprintf(stderr, "Synopsis: patch <address> <value>\n");
		return;
	}
	addrVal = strtoul(addrStr, NULL, 16);
	value = strtoul(valStr, NULL, 16);
	umdkMonitorGetRam(buf);
	buf[addrVal] = value;
	umdkMonitorPutRam(buf);
}

void doHelp(void) {
	printf("USB MegaDrive DevKit Debugger (C) 2011 Chris McClelland\n");
	printf("\nMegaDrive control:\n");
	printf("  reset -- Reset the MegaDrive\n");
	printf("  load <file> -- Load the ROM file into the cartridge address space\n");

	printf("\nBreakpoint commands:\n");
	printf("  break <address> -- Set the breakpoint at the given address\n");
	printf("  disable -- Disable the breakpoint\n");

	printf("\nDebugging commands:\n");
	printf("  dump -- Dump the 68000 registers\n");
	printf("  cont -- Continue execution\n");
	printf("  step -- Execute one instruction\n");
	printf("  get <file> -- Save the MegaDrive's 64kbytes of on-board RAM to a file\n");
	printf("  put <file> -- Load the MegaDrive's 64kbytes of on-board RAM from a file\n");
	printf("  patch <address> <value> -- Alter one byte in the MegaDrive's on-board RAM\n");

	printf("\nMisc commands:\n");
	printf("  help -- This help page\n");
	printf("  quit -- Exit the debugger\n");

	printf("\n");
}

void parseLine(char *line) {
	char *savePtr = NULL;
	char *command = strtok_r(line, ";", &savePtr);
	while ( command ) {
		parseCommand(command);
		command = strtok_r(NULL, ";", &savePtr);
	}
}

void parseCommand(char *command) {
	char *savePtr = NULL;
	char *op = strtok_r(command, " ", &savePtr);
	if ( op ) {
		if ( !strcmp(op, "reset") ) {
			doReset();
		} else if ( !strcmp(op, "load") ) {
			doLoad(&savePtr);
		} else if ( !strcmp(op, "break") ) {
			doBreak(&savePtr);
		} else if ( !strcmp(op, "disable") ) {
			doDisable();
		} else if ( !strcmp(op, "dump") ) {
			doDump();
		} else if ( !strcmp(op, "step") ) {
			doStep();
		} else if ( !strcmp(op, "cont") ) {
			doCont();
		} else if ( !strcmp(op, "get") ) {
			doGet(&savePtr);
		} else if ( !strcmp(op, "put") ) {
			doPut(&savePtr);
		} else if ( !strcmp(op, "patch") ) {
			doPatch(&savePtr);
		} else if ( !strcmp(op, "help") ) {
			doHelp();
		} else {
			fprintf(stderr, "Unrecognised command: %s\n", op);
		}
	}
}

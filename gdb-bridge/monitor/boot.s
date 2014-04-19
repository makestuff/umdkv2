/*
 * Copyright (C) 2014 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
	.text
	.org    0x000000
vectors:
	dc.l	0x0		/* 00: Initial SSP */
	dc.l	start		/* 04: Initial PC */
	dc.l	interrupt	/* 08: Bus error */
	dc.l	interrupt	/* 0C: Address error */
	dc.l	0x400000	/* 10: Illegal instruction */
	dc.l	interrupt	/* 14: Division by zero */
	dc.l	interrupt	/* 18: CHK exception */
	dc.l	interrupt	/* 1C: TRAPV exception */
	dc.l	interrupt	/* 20: Privilege violation */
	dc.l	interrupt	/* 24: Trace */
	dc.l	interrupt	/* 28: Line 1010 emulator */
	dc.l	interrupt	/* 2C: Line 1111 emulator */
	dc.l	interrupt	/* 30: Reserved */
	dc.l	interrupt	/* 34: Reserved */
	dc.l	interrupt	/* 38: Reserved */
	dc.l	interrupt	/* 3C: Uninitialized interrupt */
	dc.l	interrupt	/* 40: Reserved */
	dc.l	interrupt	/* 44: Reserved */
	dc.l	interrupt	/* 48: Reserved */
	dc.l	interrupt	/* 4C: Reserved */
	dc.l	interrupt	/* 50: Reserved */
	dc.l	interrupt	/* 54: Reserved */
	dc.l	interrupt	/* 58: Reserved */
	dc.l	interrupt	/* 5C: Reserved */
	dc.l	interrupt	/* 60: Spurious interrupt */
	dc.l	interrupt	/* 64: IRQ 1 */
	dc.l	interrupt	/* 68: IRQ 2 */
	dc.l	interrupt	/* 6C: IRQ 3 */
	dc.l	interrupt	/* 70: IRQ 4 */
	dc.l	interrupt	/* 74: IRQ 5 */
	dc.l	interrupt	/* 78: IRQ 6 */
	dc.l	interrupt	/* 7C: IRQ 7 */
	dc.l	interrupt	/* 80: TRAP #0 */
	dc.l	interrupt	/* 84: TRAP #1 */
	dc.l	interrupt	/* 88: TRAP #2 */
	dc.l	interrupt	/* 8C: TRAP #3 */
	dc.l	interrupt	/* 90: TRAP #4 */
	dc.l	interrupt	/* 94: TRAP #5 */
	dc.l	interrupt	/* 98: TRAP #6 */
	dc.l	interrupt	/* 9C: TRAP #7 */
	dc.l	interrupt	/* A0: TRAP #8 */
	dc.l	interrupt	/* A4: TRAP #9 */
	dc.l	interrupt	/* A8: TRAP #10 */
	dc.l	interrupt	/* AC: TRAP #11 */
	dc.l	interrupt	/* B0: TRAP #12 */
	dc.l	interrupt	/* B4: TRAP #13 */
	dc.l	interrupt	/* B8: TRAP #14 */
	dc.l	interrupt	/* BC: TRAP #15 */
	dc.l	interrupt	/* C0: Reserved */
	dc.l	interrupt	/* C4: Reserved */
	dc.l	interrupt	/* C8: Reserved */
	dc.l	interrupt	/* CC: Reserved */
	dc.l	interrupt	/* D0: Reserved */
	dc.l	interrupt	/* D4: Reserved */
	dc.l	interrupt	/* D8: Reserved */
	dc.l	interrupt	/* DC: Reserved */
	dc.l	interrupt	/* E0: Reserved */
	dc.l	interrupt	/* E4: Reserved */
	dc.l	interrupt	/* E8: Reserved */
	dc.l	interrupt	/* EC: Reserved */
	dc.l	interrupt	/* F0: Reserved */
	dc.l	interrupt	/* F4: Reserved */
	dc.l	interrupt	/* F8: Reserved */
	dc.l	interrupt	/* FC: Reserved */
	dc.l	0x53454741	/* "SEGA" */
start:
	move.b	0xa10001, d0
	andi.b	#0x0f, d0
	beq.s	noTMSS
	move.l	#0x53454741, 0xa14000	/* write "SEGA" to TMSS register */
noTMSS:
	illegal				/* give control to monitor */

interrupt:
	rte

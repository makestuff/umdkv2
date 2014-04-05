/*
 * Copyright (c) 2009-2011 Emmanuel Vadot <elbarto@homebrewdev.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: crt0.s,v 1.12 2011/01/25 16:21:30 elbarto Exp $ */

.text

.global start
.global _hblank
.global _vblank

.extern htimer
.extern vtimer

Vectors:
	.org    0x0000

	dc.l	0x0		/* Initial Stack Address */
	dc.l	start		/* Start of program Code */
	dc.l	INT		/* Bus error */
	dc.l	INT		/* Address error */
	dc.l	INT		/* Illegal instruction */
	dc.l	INT		/* Division by zero */
	dc.l	INT		/* CHK exception */
	dc.l	INT		/* TRAPV exception */
	dc.l	INT		/* Privilage violation */
	dc.l	INT		/* TRACE exception */
	dc.l	INT		/* Line-A emulator */
	dc.l	INT		/* Line-F emulator */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Co-processor protocol violation */
	dc.l	INT		/* Format error */
	dc.l	INT		/* Uninitialized Interrupt */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Spurious Interrupt */
	dc.l	INT		/* IRQ Level 1 */
	dc.l	INT		/* IRQ Level 2 */
	dc.l	INT		/* IRQ Level 3 */
	dc.l	_hblank		/* IRQ Level 4 (VDP Horizontal Interrupt) */
	dc.l	INT		/* IRQ Level 5 */
	dc.l	_vblank		/* IRQ Level 6 (VDP Vertical Interrupt) */
	dc.l	INT		/* IRQ Level 7 */
	dc.l	INT		/* TRAP #00 Exception */
	dc.l	INT		/* TRAP #01 Exception */
	dc.l	INT		/* TRAP #02 Exception */
	dc.l	INT		/* TRAP #03 Exception */
	dc.l	INT		/* TRAP #04 Exception */
	dc.l	INT		/* TRAP #05 Exception */
	dc.l	INT		/* TRAP #06 Exception */
	dc.l	INT		/* TRAP #07 Exception */
	dc.l	INT		/* TRAP #08 Exception */
	dc.l	INT		/* TRAP #09 Exception */
	dc.l	INT		/* TRAP #10 Exception */
	dc.l	INT		/* TRAP #11 Exception */
	dc.l	INT		/* TRAP #12 Exception */
	dc.l	INT		/* TRAP #13 Exception */
	dc.l	INT		/* TRAP #14 Exception */
	dc.l	INT		/* TRAP #15 Exception */
	dc.l	INT		/* (FP) Branch or Set on Unordered Condition */
	dc.l	INT		/* (FP) Inexact Result */
	dc.l	INT		/* (FP) Divide by Zero */
	dc.l	INT		/* (FP) Underflow */
	dc.l	INT		/* (FP) Operand Error */
	dc.l	INT		/* (FP) Overflow */
	dc.l	INT		/* (FP) Signaling NAN */
	dc.l	INT		/* (FP) Unimplemented Data Type */
	dc.l	INT		/* MMU Configuration Error */
	dc.l	INT		/* MMU Illegal Operation Error */
	dc.l	INT		/* MMU Access Violation Error */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */
	dc.l	INT		/* Reserved (NOT USED) */

Header:
	.ascii  "SEGA MEGA DRIVE " 					/* Console Name (16) */
	.ascii  "(C)SEGA 2009.DEC"					/* Copyright Information (16) */
	.ascii  "MY PROG                                         "	/* Domestic Name (48) */
	.ascii  "MY PROG                                         "	/* Overseas Name (48) */
	.ascii  "GM 00000000-00"					/* Serial Number (2, 14) */
	dc.w    0x0000							/* Checksum (2) */
	.ascii  "JD              "					/* I/O Support (16) */
	dc.l    0x00000000						/* ROM Start Address (4) */
	dc.l    0x20000							/* ROM End Address (4) */
	dc.l    0x00FF0000						/* Start of Backup RAM (4) */
	dc.l    0x00FFFFFF						/* End of Backup RAM (4) */
	.ascii  "                        "				/* Modem Support (12) */
	.ascii  "                                        "		/* Memo (40) */
	.ascii  "JUE             "					/* Country Support (16) */

start:
        move.b  0xa10001,d0
        andi.b  #0x0f,d0
        beq     WrongVersion
* Sega Security Code (SEGA)
        move.l  #0x53454741,0xa14000
WrongVersion:

* set stack pointer
        move.w   #0,sp

* user mode
	move.w  #0x2300,sr

* clear the megadrive ram
	lea     0xff0000,a0
	move.w   #0,d0
_clrram:
	move.w  #0,(a0)+
	subq.w  #2,d0
	bne     _clrram

	jmp     main

INT:
	rte

_hblank:
	addq.w	#1, htimer
	movem.l d0-d1/a0-a1,-(sp)
	bsr    hblank
	movem.l (sp)+,d0-d1/a0-a1
	rte

_vblank:
	addq.w	#1, vtimer
	movem.l d0-d1/a0-a1,-(sp)
	bsr    vblank
	movem.l (sp)+,d0-d1/a0-a1
	rte

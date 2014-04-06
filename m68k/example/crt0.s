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

*.global start
.global _hblank
.global _vblank

.extern htimer
.extern vtimer
.extern my_func

monitor	= 0x400000
	
	.org    0x0000

vectors:
	dc.l	0x0		/* Initial Stack Address */
	dc.l	start		/* Start of program Code */
	dc.l	interrupt	/* Bus error */
	dc.l	interrupt	/* Address error */
	dc.l	monitor		/* Illegal instruction */
	dc.l	interrupt	/* Division by zero */
	dc.l	interrupt	/* CHK exception */
	dc.l	interrupt	/* TRAPV exception */
	dc.l	interrupt	/* Privilage violation */
	dc.l	interrupt	/* TRACE exception */
	dc.l	interrupt	/* Line-A emulator */
	dc.l	interrupt	/* Line-F emulator */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Co-processor protocol violation */
	dc.l	interrupt	/* Format error */
	dc.l	interrupt	/* Uninitialized Interrupt */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Spurious Interrupt */
	dc.l	interrupt	/* IRQ Level 1 */
	dc.l	interrupt	/* IRQ Level 2 */
	dc.l	interrupt	/* IRQ Level 3 */
	dc.l	_hblank		/* IRQ Level 4 (VDP Horizontal Interrupt) */
	dc.l	interrupt	/* IRQ Level 5 */
	dc.l	my_func		/* IRQ Level 6 (VDP Vertical Interrupt) */
	dc.l	interrupt	/* IRQ Level 7 */
	dc.l	interrupt	/* TRAP #00 Exception */
	dc.l	interrupt	/* TRAP #01 Exception */
	dc.l	interrupt	/* TRAP #02 Exception */
	dc.l	interrupt	/* TRAP #03 Exception */
	dc.l	interrupt	/* TRAP #04 Exception */
	dc.l	interrupt	/* TRAP #05 Exception */
	dc.l	interrupt	/* TRAP #06 Exception */
	dc.l	interrupt	/* TRAP #07 Exception */
	dc.l	interrupt	/* TRAP #08 Exception */
	dc.l	interrupt	/* TRAP #09 Exception */
	dc.l	interrupt	/* TRAP #10 Exception */
	dc.l	interrupt	/* TRAP #11 Exception */
	dc.l	interrupt	/* TRAP #12 Exception */
	dc.l	interrupt	/* TRAP #13 Exception */
	dc.l	interrupt	/* TRAP #14 Exception */
	dc.l	interrupt	/* TRAP #15 Exception */
	dc.l	interrupt	/* (FP) Branch or Set on Unordered Condition */
	dc.l	interrupt	/* (FP) Inexact Result */
	dc.l	interrupt	/* (FP) Divide by Zero */
	dc.l	interrupt	/* (FP) Underflow */
	dc.l	interrupt	/* (FP) Operand Error */
	dc.l	interrupt	/* (FP) Overflow */
	dc.l	interrupt	/* (FP) Signaling NAN */
	dc.l	interrupt	/* (FP) Unimplemented Data Type */
	dc.l	interrupt	/* MMU Configuration Error */
	dc.l	interrupt	/* MMU Illegal Operation Error */
	dc.l	interrupt	/* MMU Access Violation Error */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */
	dc.l	interrupt	/* Reserved (NOT USED) */

header:
	.ascii	"SEGA MEGA DRIVE " 					/* Console Name (16) */
	.ascii	"(C)SEGA 2009.DEC"					/* Copyright Information (16) */
	.ascii	"MY PROG                                         "	/* Domestic Name (48) */
	.ascii	"MY PROG                                         "	/* Overseas Name (48) */
	.ascii	"GM 00000000-00"					/* Serial Number (2, 14) */
	dc.w	0x0000							/* Checksum (2) */
	.ascii	"JD              "					/* I/O Support (16) */
	dc.l	0x00000000						/* ROM Start Address (4) */
	dc.l	0x20000							/* ROM End Address (4) */
	dc.l	0x00FF0000						/* Start of Backup RAM (4) */
	dc.l	0x00FFFFFF						/* End of Backup RAM (4) */
	.ascii	"                        "				/* Modem Support (12) */
	.ascii	"                                        "		/* Memo (40) */
	.ascii	"JUE             "					/* Country Support (16) */

start:
	move.b	0xa10001, d0
	andi.b	#0x0f, d0
	beq.s	noTMSS
	move.l	#0x53454741, 0xa14000	| write "SEGA" to TMSS register

noTMSS:
	move.w	#0, sp			| set stack pointer
	move.w	#0x2300, sr		| switch to user mode
	jmp	main			| start main(), which must not return
	
interrupt:
	rte

_hblank:
	addq.w	#1, htimer
	movem.l	d0-d1/a0-a1,-(sp)
	bsr	hblank
	movem.l	(sp)+,d0-d1/a0-a1
	rte

_vblank:
	addq.w	#1, vtimer
	movem.l	d0-d1/a0-a1,-(sp)
	bsr	vblank
	movem.l	(sp)+,d0-d1/a0-a1
	rte

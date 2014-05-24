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

	.psize	0
	.text

	.global	_hblank
	.global	_vblank
	.global	readBlock
	.global	foo

	.extern	htimer
	.extern	vtimer
	.extern	my_func

	.org	0x0000
vectors:
	dc.l	0x0		/* Initial Stack Address */
	dc.l	start		/* Start of program Code */
	dc.l	interrupt	/* Bus error */
	dc.l	interrupt	/* Address error */
	dc.l	interrupt	/* Illegal instruction */
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
	move.l	#0x53454741, 0xa14000	/* write "SEGA" to TMSS register */

noTMSS:
	move.l	#0x3FFF, d0
	move.l	#0xCCCCCCCC, d1
	lea	0xFF0000, a0
ccMem:
	move.l	d1, (a0)+
	dbra	d0, ccMem
	moveq	#0, d0
	moveq	#0, d1
	moveq	#0, d2
	moveq	#0, d3
	moveq	#0, d4
	moveq	#0, d5
	moveq	#0, d6
	moveq	#0, d7
	suba.l	a0, a0
	suba.l	a1, a1
	suba.l	a2, a2
	suba.l	a3, a3
	suba.l	a4, a4
	suba.l	a5, a5
	suba.l	fp, fp
	suba.l	sp, sp			/* set stack pointer = 0 */
	move.w	#0x2300, sr		/* switch to user mode */
	jmp	main			/* start main(), which must not return */
	
interrupt:
	rte

	TURBO = 0x0001
	SUPPRESS = 0x0002
	FLASH = 0x0004
	SDCARD = 0x0008
	
	CMD_GO_IDLE_STATE        = 0
	CMD_SEND_OP_COND         = 1
	CMD_STOP_TRANSMISSION    = 12
	CMD_READ_SINGLE_BLOCK    = 17
	CMD_READ_MULTIPLE_BLOCKS = 18
	CMD_APP_SEND_OP_COND     = 41
	CMD_APP_CMD              = 55

	TOKEN_SUCCESS            = 0x00
	TOKEN_READ_SINGLE        = 0xFE
	IN_IDLE_STATE            = (1<<0)
	
delay:	move.w	d1, -(sp)
dLoop:	dbra	d1, dLoop
	move.w	(sp)+, d1
	rts

/*------------------------------------------------------------------------------
 * slowCmd() sends the command in d0.b with zero argument, and with sufficient
 * delay between bytes to work in the 400kHz mode (necessary during init). It
 * relies on d1.w containing the delay calibration (should be 7). On exit, d0.b
 * contains the response byte.
 */
slowCmd:
	move.b	#0xFF, 2(a0)	/* dummy byte */
	bsr.w	delay
	or.b	#0x40, d0
	move.b	d0, 2(a0)	/* command byte */
	bsr.w	delay
	move.b	#0x00, 2(a0)	/* param MSB */
	bsr.w	delay
	move.b	#0x00, 2(a0)	/* param */
	bsr.w	delay
	move.b	#0x00, 2(a0)	/* param */
	bsr.w	delay
	move.b	#0x00, 2(a0)	/* param LSB */
	bsr.w	delay
	move.b	#0x95, 2(a0)	/* CRC */
	bsr.w	delay
	move.b	#0xFF, 2(a0)	/* ignore return byte */
	bsr.w	delay
L0:	move.b	#0xFF, 2(a0)	/* get response */
	bsr.w	delay
	move.b	2(a0), d0
	cmp.b	#0xFF, d0
	beq.s	L0
	rts

/*---------------------- Issue a command in turbo mode -----------------------*/
fastCmd:
	or.w	#0xFF40, d0
	move.w	d0, 0(a0)
	swap	d1
	move.w	d1, 0(a0)
	swap	d1
	move.w	d1, 0(a0)
	move.w	#0x95FF, 0(a0)	/* dummy CRC & return byte */
1:	move.b	#0xFF, 2(a0)	/* get response */
	move.b	2(a0), d0
	cmp.b	#0xFF, d0
	beq.s	1b
	rts

/*---------------------- Read the MBR from the SD-card -----------------------*/
readBlock:
	movem.l a0-a1/d0-d1, -(sp)
	lea	0xA13000, a0
	lea	0xFF1000, a1

	/* 256 clocks at 400kHz with DI low */
tryInit:
	move.w	#0, 4(a0)
	moveq	#31, d0		/* 32*8 = 256 */
	moveq	#7, d1		/* enough delay for one byte @400kHz */
L3:	move.b	#0x00, 2(a0)
	bsr.w	delay
	dbra	d0, L3

	/* 80 clocks at 400kHz with DI high */
	moveq	#9, d0		/* 10*8 = 80 */
L4:	move.b	#0xFF, 2(a0)
	bsr.w	delay
	dbra	d0, L4

	/* Bring CS low and send CMD0 to reset and put the card in SPI mode */
	move.w	#SDCARD, 4(a0)
	move.b	#CMD_GO_IDLE_STATE, d0
	bsr.w	slowCmd
	cmp.b	#IN_IDLE_STATE, d0
	beq.s	okIdle
	
	/* Send a couple of dummy bytes */
	move.b	#0xFF, 2(a0)
	bsr.w	delay
	move.b	#0xFF, 2(a0)
	bsr.w	delay
	bra.w	tryInit
	
	/* Tell the card to initialize itself with ACMD41 */
okIdle:
	move.b	#CMD_APP_CMD, d0
	bsr.w	slowCmd
	move.b	#CMD_APP_SEND_OP_COND, d0
	bsr.w	slowCmd
L5:	move.b	#CMD_SEND_OP_COND, d0
	bsr.w	slowCmd
	tst.b	d0
	bne.s	L5

	/* Send a couple of dummy bytes */
	move.b	#0xFF, 2(a0)
	bsr.w	delay
	move.b	#0xFF, 2(a0)
	bsr.w	delay

	/* Deselect SD-card */
	move.w	#0, 4(a0)
	*movem.l (sp)+, a0-a1/d0-d1
	*rts
	

doRead:
	*movem.l a0-a1/d0-d1, -(sp)
	*lea	0xA13000, a0
	*lea	0xFF1000, a1

	move.w	#(SDCARD | TURBO), 4(a0)
	*move.b	#CMD_READ_SINGLE_BLOCK, d0
	move.b	#CMD_READ_MULTIPLE_BLOCKS, d0
	*move.l	#0, d1
	*move.l	#(0x800<<9), d1
	move.l	#(9832<<9), d1
	bsr.w	fastCmd

	moveq	#15, d1
waitReadToken:
	lea	0xFF1000, a1
	move.b	#0xFF, 2(a0)
	cmp.b	#TOKEN_READ_SINGLE, 2(a0)
	bne.s	waitReadToken

foo:
	move.w	#0xFFFF, 0(a0)
	moveq	#31, d0
getBlk:	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	dbra	d0, getBlk
	dbra	d1, waitReadToken
	*bra.s	waitReadToken
	
	move.b	#CMD_STOP_TRANSMISSION, d0
	bsr.w	fastCmd
	
	move.w	#TURBO, 4(a0)
	movem.l (sp)+, a0-a1/d0-d1
	rts

/*----------------- Horizontal & vertical interrupt wrappers -----------------*/
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

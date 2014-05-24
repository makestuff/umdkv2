	.text

	monBase	= 0x400000
	cmdFlag	= monBase + 0x400
	cmdIdx	= cmdFlag + 2
	address	= cmdFlag + 4*1
	length	= cmdFlag + 4*2
	svBase	= cmdFlag + 4*3
	d0Save	= svBase + 4*0
	d1Save	= svBase + 4*1
	d2Save	= svBase + 4*2
	d3Save	= svBase + 4*3
	d4Save	= svBase + 4*4
	d5Save	= svBase + 4*5
	d6Save	= svBase + 4*6
	d7Save	= svBase + 4*7
	a0Save	= svBase + 4*8
	a1Save	= svBase + 4*9
	a2Save	= svBase + 4*10
	a3Save	= svBase + 4*11
	a4Save	= svBase + 4*12
	a5Save	= svBase + 4*13
	fpSave	= svBase + 4*14
	spSave	= svBase + 4*15
	srSave	= svBase + 4*16
	pcSave	= svBase + 4*17
	ramSave	= svBase + 4*18

	SPIDATW	= 0x0000 /* SPI word read/write */
	SPIDATB	= 0x0002 /* SPI byte read/write */
	SPICON	= 0x0004 /* SPI interface control */
	bmTURBO	= (1<<0) /*0x0001*/
	bmFLASH	= (1<<2) /*0x0004*/

	/* The second-stage bootloader may be up to 256 bytes long and is loaded
	 * into onboard RAM at 0xFF0000 by the first-stage bootloader, which is
	 * 50 words of ROM built into the FPGA. The first instruction is
	 * important because (as an intentional side-effect) it triggers a
	 * switch inside the FPGA which maps the SDRAM into the address-map in
	 * place of the first-stage bootloader ROM.
	 *
	 * Since this code executes from onboard RAM, the instruction fetches
	 * will not appear in any trace-log. Also, take care not to do TOO much
	 * here, because for as long as instructions are being read from onboard
	 * RAM, the SDRAM is not being refreshed.
	 */
	.org    0x000000
boot:
	move.w	#0x0000, SPICON(a0)	/* deselect flash & page in SDRAM */
	move.b	0xA10001, d0
	andi.b	#0x0f, d0
	beq.s	1f
	move.l	#0x53454741, 0xA14000	/* write "SEGA" to TMSS register */
1:
	lea	monBase, a1
	move.w	#0x0000, 0x400(a1)
	move.w	#(bmTURBO | bmFLASH), SPICON(a0)
	move.w	#0x0306, SPIDATW(a0)	/* load 0x200 words (1024 bytes) of */
	move.w	#0x0100, SPIDATW(a0)	/* monitor code from flash address  */
	move.w	#0xFFFF, SPIDATW(a0)	/* 0x60100 */

	/* Load monitor */
	move.w	#0x01FF, d0
2:
	move.w	SPIDATW(a0), (a1)+
	dbra	d0, 2b

	/* Load menu program vectors + cart metadata (0x200 bytes) */
	move.b	#0x40, 0xA130F3
	lea	0x480000, a1
	move.w	#0x00ff, d0
3:
	move.w	SPIDATW(a0), (a1)+
	dbra	d0, 3b

	/* Load menu program code (64KiB less 0x200 bytes) */
	lea	0x420200, a1
	move.w	#0x7eff, d0
4:
	move.w	SPIDATW(a0), (a1)+
	dbra	d0, 4b
	move.w	#0x0000, SPICON(a0)
	suba.l	sp, sp
	lea	0x420200, a0
	jmp	(a0)


	/* The monitor code may be up to 0x400 (1024) bytes long and is loaded
	 * into SDRAM at logical address 0x400000 (physical address 0xF80000) by
	 * the second stage bootloader (above). It is assembled at 0x100 so that
	 * the second-stage bootloader and monitor can be flash-loaded as one
	 * piece. It is padded at the end bringing the overall .bin file up to
	 * 0x500 (1280) bytes, ready to be loaded into flash at 0x60000 using
	 * the "gordon" tool and the spi-talk.xsvf FPGA logic.
	 */
	.org    0x000100
main:
	move.w	#0x2700, sr		/* disable interrupts */
	move.l	d0, d0Save		/* save all registers in host-accessible memory */
	move.l	d1, d1Save
	move.l	d2, d2Save
	move.l	d3, d3Save
	move.l	d4, d4Save
	move.l	d5, d5Save
	move.l	d6, d6Save
	move.l	d7, d7Save
	move.l	a0, a0Save
	move.l	a1, a1Save
	move.l	a2, a2Save
	move.l	a3, a3Save
	move.l	a4, a4Save
	move.l	a5, a5Save
	move.l	fp, fpSave
	move.l	sp, d0
	addq.l	#6, d0
	move.l	d0, spSave		/* should look at saved status reg to decide whether to save USP or SSP */
	move.l	2(sp), pcSave
	move.w	#0x0000, srSave
	move.w	0(sp), srSave+2
	and.w	#0x7FFF, srSave+2	/* clear TRACE bit */
	move.w	#1, cmdFlag		/* tell host we're ready to accept commands (comment out for tracing a command) */
	
commandLoop:
	cmp.w	#2, cmdFlag		/* see if host has left us... */
	bne.s	commandLoop		/* ...a command to execute */
	move.w	cmdIdx, d0		/* yes...get command index */
	and.w	#0x07, d0		/* mask command to stop random jumps off the end of the jump table */
	asl.w	#2, d0			/* multiply by four: the offset is in longwords */
lda1:	lea	(jTab-lda1-2)(pc), a0	/* load jump table */
	move.l	(a0, d0.w), a0		/* load offset of requested command */
lda2:	jsr	0(pc, a0)		/* ...and jump to it */
	move.w	#1, cmdFlag		/* notify host of command completion */
	bra.s	commandLoop		/* loop back again */

quit:
	move.w	#0, cmdFlag		/* tell host we're running */
	move.l	d0Save, d0		/* restore registers from (possibly host-modified) memory */
	move.l	d1Save, d1
	move.l	d2Save, d2
	move.l	d3Save, d3
	move.l	d4Save, d4
	move.l	d5Save, d5
	move.l	d6Save, d6
	move.l	d7Save, d7
	move.l	a0Save, a0
	move.l	a1Save, a1
	move.l	a2Save, a2
	move.l	a3Save, a3
	move.l	a4Save, a4
	move.l	a5Save, a5
	move.l	fpSave, a6
	move.w	srSave+2, 0(sp)
	move.l	pcSave, 2(sp)
	rte

step:
	or.w	#0x8000, srSave+2	/* set TRACE bit */
continue:
	lea	(quit-continue-2)(pc), a0
	move.l	a0, (sp)		/* redirect rts */
doNothing:
	rts

read:
	move.l	address, a0
	lea	ramSave, a1
	move.l	length, d0
	asr	#1, d0
	subq	#1, d0
rdLoop:	move.w	(a0)+, (a1)+
	dbra	d0, rdLoop
	rts

write:
	move.l	address, a1
	lea	ramSave, a0
	move.l	length, d0
	asr	#1, d0
	subq	#1, d0
wrLoop:	move.w	(a0)+, (a1)+
	dbra	d0, wrLoop
	rts

reset:
	reset				/* reset peripherals */
	move.l	0x000000, sp		/* set supervisor stack pointer */
	move.l	0x000004, a0		/* get reset vector */
	move.w	#0, cmdFlag		/* tell host we're running */
	jmp	(a0)			/* and...GO! */

jTab:
	dc.l	step-lda2-2
	dc.l	continue-lda2-2
	dc.l	read-lda2-2
	dc.l	write-lda2-2
	dc.l	reset-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2

	.org    0x0004FE
	dc.w	0x0000

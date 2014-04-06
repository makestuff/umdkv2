.text

cmdFlag		= 0x400400
cmdIndex	= cmdFlag + 2
address		= cmdFlag + 4*1
length		= cmdFlag + 4*2
saveBase	= cmdFlag + 4*3
d0Save		= saveBase + 4*0
d1Save		= saveBase + 4*1
d2Save		= saveBase + 4*2
d3Save		= saveBase + 4*3
d4Save		= saveBase + 4*4
d5Save		= saveBase + 4*5
d6Save		= saveBase + 4*6
d7Save		= saveBase + 4*7
a0Save		= saveBase + 4*8
a1Save		= saveBase + 4*9
a2Save		= saveBase + 4*10
a3Save		= saveBase + 4*11
a4Save		= saveBase + 4*12
a5Save		= saveBase + 4*13
fpSave		= saveBase + 4*14
spSave		= saveBase + 4*15
srSave		= saveBase + 4*16
pcSave		= saveBase + 4*17
ramSave		= saveBase + 4*18

main:
	move.l	d0, d0Save		| save all registers in host-accessible memory
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
	move.l	d0, spSave		| should look at saved status reg to decide whether to save USP or SSP
	move.l	2(sp), pcSave
	move.w	#0x0000, srSave
	move.w	0(sp), srSave+2
	and.w	#0x7FFF, srSave+2	| clear TRACE bit
	move.w	#1, cmdFlag		| tell host we're ready to accept commands
	
commandLoop:
	cmp.w	#2, cmdFlag		| see if host has left us...
	bne.s	commandLoop		| ...a command to execute
	move.w	cmdIndex, d0		| yes...get command index
	and.w	#0x07, d0		| mask command to stop random jumps off the end of the jump table
	asl.w	#2, d0			| multiply by four: the offset is in longwords
lda1:	lea	(jTab-lda1-2)(pc), a0	| load jump table
	move.l	(a0, d0.w), a0		| load offset of requested command
lda2:	jsr	0(pc,a0)		| ...and jump to it
	move.w	#1, cmdFlag		| notify host of command completion
	bra.s	commandLoop		| loop back again

quit:
	move.w	#0, cmdFlag		| tell host we're running
	move.l	d0Save, d0		| restore registers from (possibly host-modified) memory
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
	or.w	#0x8000, srSave+2	| set TRACE bit
continue:
	lea	(quit-continue-2)(pc), a0
	move.l	a0, (sp)		| redirect rts
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

jTab:
	dc.l	step-lda2-2
	dc.l	continue-lda2-2
	dc.l	read-lda2-2
	dc.l	write-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2

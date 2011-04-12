.text

.global	_start

yieldBus	= 0xA13000
command		= 0x400400
saveBase	= command+2
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
a6Save		= saveBase + 4*14
a7Save		= saveBase + 4*15
pcSave		= saveBase + 4*16
srSave		= saveBase + 4*17
ramSave		= saveBase + 4*17 + 2

CMD_LOOP	= 0

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
	move.l	a6, a6Save
	move.l	a7, a7Save
	move.l	2(ssp), pcSave
	move.w	0(ssp), srSave
	and.w	#0x7FFF, srSave		| clear TRACE bit

commandLoop:
	jsr	yieldBus		| offer bus to host
	move.w	command, d0		| see if host has left us...
	beq.s	commandLoop		| ...a command to execute
	move.w	#CMD_LOOP, command	| yes...zero command for next loop
	subq.w	#1, d0			| remove one so jump table can begin at command one rather than zero
	and.w	#0x07, d0		| mask command to stop random jumps off the end of the jump table
	asl.w	#2, d0			| multiply by four: the offset is in longwords
lda1:	lea	(jumpTable-lda1-2)(pc), a0		| load jump table
	move.l	(a0, d0.w), a0		| load offset of requested command
lda2:	jsr	0(pc,a0)		| ...and jump to it
	bra.s	commandLoop		| loop back again

quit:
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
	move.l	a6Save, a6
	move.l	a7Save, a7
	move.w	srSave, 0(ssp)
	move.l	pcSave, 2(ssp)
	rte

step:
	or.w	#0x8000, srSave		| set TRACE bit
continue:
	lea	(quit-continue-2)(pc), a0
	move.l	a0, (ssp)		| redirect rts
doNothing:
	rts

saveRam:
	move.w	#0x3FFF, d0
	lea	0xFF0000, a0
	lea	ramSave, a1
svCopy:	move.l	(a0)+, (a1)+
	dbra	d0, svCopy
	rts

loadRam:
	move.w	#0x3FFF, d0
	lea	ramSave, a0
	lea	0xFF0000, a1
ldCopy:	move.l	(a0)+, (a1)+
	dbra	d0, ldCopy
	rts

jumpTable:
	dc.l	step-lda2-2
	dc.l	continue-lda2-2
	dc.l	saveRam-lda2-2
	dc.l	loadRam-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2
	dc.l	doNothing-lda2-2

.text

.global	main
.global	htimer
.global	vtimer
.global	hblank
.global	vblank
		
htimer		= 0x000000
vtimer		= 0x000000

ioBase		= 0xA13000
mdReg00		= 0
mdReg01		= 2

main:
	moveq	#0, d2
	move.l	#-1, d0
brk:	asl.l	#1, d0
	move.l	#0, d0
	nop
	nop
	nop
	addq.l	#1, d2
	bra.s	brk

hblank:
vblank:
	rts

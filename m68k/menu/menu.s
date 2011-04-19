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
mdReg02		= 4

main:	lea	ioBase, a0		| base of I/O
	moveq	#0, d0			| init counter
loop:
	move.w	d0, mdReg00(a0)		| write counter to register 1
	addq.w	#1, d0			| inc counter
	move.l	#65535, d1		| init delay count
delay:	nop
	nop
	nop
	nop
	dbra	d1, delay		| loop until d0 = -1
	bra.s	loop			| ...and repeat

hblank:
vblank:
	rts

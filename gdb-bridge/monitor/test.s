	.text
test:
	moveq	#0, d0			| 0x200
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
	nop

	move.l	#0x410F9343, d0		| 0x220 + 6*0
	move.l	#0x0E64E2F4, d1		| 0x220 + 6*1
	move.l	#0x3371A5B6, d2		| 0x220 + 6*2
	move.l	#0x83B174BA, d3		| 0x220 + 6*3
	move.l	#0x118C9F22, d4		| 0x220 + 6*4
	move.l	#0x9E175F2A, d5		| 0x220 + 6*5
	move.l	#0x804D5E0E, d6		| 0x220 + 6*6
	move.l	#0x22CA3C6F, d7		| 0x220 + 6*7
	move.l	#0xAC44C7F0, a0		| 0x220 + 6*8
	move.l	#0xEFBC0062, a1		| 0x220 + 6*9
	move.l	#0xC2A6A7A4, a2		| 0x220 + 6*10
	move.l	#0x99DA044E, a3		| 0x220 + 6*11
	move.l	#0x073763D6, a4		| 0x220 + 6*12
	move.l	#0x32E5A6C1, a5		| 0x220 + 6*13
	move.l	#0x3BCA8003, fp		| 0x220 + 6*14

	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	illegal

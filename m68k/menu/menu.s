.text

.global	main
.global	htimer
.global	vtimer
.global	hblank
.global	vblank
		
htimer		= 0x000000
vtimer		= 0x000000

# UMDKv2 registers:
ioBase		= 0xA13000
YIELDBUS	= 0
RAMPAGE		= 2
SERDATA		= 5
SERCTRL		= 7

* Bits in SERCTRL:
SD_TURBO	= 0
SD_CS		= 1
SD_BUSY		= 2
SD_CD		= 3

* SD card tokens
TOKEN_SUCCESS             = 0x00
TOKEN_READ_SINGLE         = 0xFE
TOKEN_READ_MULTIPLE       = 0xFE

* R1 response bits: see 7.3.2.1
R1_IN_IDLE_STATE          = (1<<0)

* SD card command codes
CMD_GO_IDLE_STATE         = 0
CMD_STOP_TRANSMISSION     = 12
CMD_READ_SINGLE_BLOCK     = 17
CMD_READ_MULTIPLE_BLOCKS  = 18
CMD_APP_SEND_OP_COND      = 41
CMD_APP_CMD               = 55

* SD card function return codes
SD_SUCCESS                = 0
SD_INIT_NOT_IDLE_ERROR    = 1
SD_INIT_TIMEOUT_ERROR     = 2
SD_READ_SINGLE_ERROR      = 3

* Macros for manipulating SERCTRL bits
.macro	sdEnable
	or.b	#(1 << SD_CS), SERCTRL(a6)
.endm
.macro	sdDisable
	and.b	#~(1 << SD_CS), SERCTRL(a6)
.endm
.macro	sdTurbo
	or.b	#(1 << SD_TURBO), SERCTRL(a6)
.endm

main:
	lea	ioBase, a6		| for easy access to I/O registers
	bsr.w	sdInit
	bsr.s	sdReadSingle
	
	moveq	#0, d0			| init counter
loop0:
	move.w	d0, YIELDBUS(a6)	| write counter to YIELDBUS
	addq.w	#1, d0			| inc counter
	move.w	#0xFFFF, d1		| init delay count
delay:
	nop
	dbra	d1, delay		| loop until d1 = -1
	bra.s	loop0			| ...and repeat

* Read a single block
sdReadSingle:
	sdEnable
	move.b	#CMD_READ_SINGLE_BLOCK, d0
	move.l	#32425, d1
	bsr.w	sendCommand

	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	


	sdDisable
	cmp.b	#SD_SUCCESS, d0
	beq.s	good
	moveq	#SD_READ_SINGLE_ERROR, d0
	rts
good:
	moveq	#SD_SUCCESS, d0
	rts
	
* Initialise SD card
sdInit:
	move.w	#999, d1
goIdle:
	* De-assert CS and send 400 clocks (1.000ms @400kHz) with DI low while power stabilises
	move.b	#0x00, SERCTRL(a6)		| de-assert CS & force slow (400kHz) mode
	move.w  #49, d0
loop1:
	bsr.w	waitReady
	move.b	#0x00, SERDATA(a6)
	dbra	d0, loop1

	* Keep CS de-asserted and send 80 (spec says "at least 74") clocks (0.2ms @400kHz) with DI
	* high to get the card ready to accept commands
	move.w	#9, d0
loop2:
	bsr.w	waitReady
	move.b	#0xFF, SERDATA(a6)
	dbra	d0, loop2
	bsr.s	waitReady

	* Assert CS and send CMD0 to reset and put the card in SPI mode
	sdEnable
	move.b	#CMD_GO_IDLE_STATE, d0
	bsr.s	initCommand
	cmp.b	#R1_IN_IDLE_STATE, d0
	dbeq	d1, goIdle
	beq.s	inIdleState
	moveq	#SD_INIT_NOT_IDLE_ERROR, d0
	move.b	#0xFF, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0xFF, SERDATA(a6)
	bsr.s	waitReady
	sdDisable
	rts

inIdleState:
	* Tell the card to initialize itself with ACMD41
	move.w	#0xFFFF, d1
waitForInit:
	move.b	#CMD_APP_CMD, d0
	bsr.s	initCommand
	move.b	#CMD_APP_SEND_OP_COND, d0
	bsr.s	initCommand
	cmp.b	#TOKEN_SUCCESS, d0
	dbeq	d1, waitForInit
	move.b	#0xFF, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0xFF, SERDATA(a6)
	bsr.s	waitReady
	sdDisable
	tst.b	d0
	beq.s	initComplete
	moveq	#SD_INIT_TIMEOUT_ERROR, d0
	rts

initComplete:
	sdDisable
	sdTurbo
	moveq	#SD_SUCCESS, d0
	rts

* Wait for the current SPI operation to complete
waitReady:
	btst	#SD_BUSY, SERCTRL(a6)
	bne.s	waitReady
	rts

* Accepts command in d0.b, returns token in d0.b
initCommand:
	or.b	#0x40, d0
	move.b	#0xFF, SERDATA(a6)
	bsr.s	waitReady
	move.b	d0, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0x00, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0x00, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0x00, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0x00, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0x95, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0xFF, SERDATA(a6)
	bsr.s	waitReady
	move.b	#0xFF, SERDATA(a6)
	bsr.s	waitReady
	move.b	SERDATA(a6), d0
	rts

arg:	dc.l	0
	
* Accepts command in d0.b, arg in d1.l, returns token in d0.b, corrupts d1.w
sendCommand:
	or.b	#0x40, d0
	move.b	#0xFF, SERDATA(a6)
	move.b	d0, SERDATA(a6)
	move.b	#0, SERDATA(a6)
	move.b	#0, SERDATA(a6)
	move.b	#0, SERDATA(a6)
	move.b	#0, SERDATA(a6)
	move.b	#0x95, SERDATA(a6)
	move.w	#0xFFFF, d1
loop3:
	move.b	#0xFF, SERDATA(a6)
	move.b	SERDATA(a6), d0
	cmp.b	#0xFF, d0
	dbne	d1, loop3
	rts

hblank:
vblank:
	rts

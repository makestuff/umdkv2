	.psize	0
	.text
	.global	sdInit
	.global	sdReadBlocks

/*--------------------------- Registers & Commands ---------------------------*/
	TURBO                    = (1<<0)
	SUPPRESS                 = (1<<1)
	FLASH                    = (1<<2)
	SDCARD                   = (1<<3)

	SPIDATW                  = 0
	SPIDATB                  = 2
	SPICON                   = 4
	
	CMD_GO_IDLE_STATE        = 0
	CMD_SEND_OP_COND         = 1
	CMD_STOP_TRANSMISSION    = 12
	CMD_READ_SINGLE_BLOCK    = 17
	CMD_READ_MULTIPLE_BLOCKS = 18
	CMD_APP_SEND_OP_COND     = 41
	CMD_APP_CMD              = 55

	TOKEN_SUCCESS            = 0x00
	TOKEN_READ_SINGLE        = 0xFE
	TOKEN_READ_MULTIPLE      = 0xFE
	IN_IDLE_STATE            = (1<<0)

/*---------------------- Delay for when in 400kHz mode -----------------------*/
delay:	move.w	d1, -(sp)
0:	dbra	d1, 0b
	move.w	(sp)+, d1
	rts

/*------------------------------------------------------------------------------
 * slowCmd() sends the command in d0.b with zero argument, and with sufficient
 * delay between bytes to work in the 400kHz mode (necessary during init). It
 * relies on d1.w containing the delay calibration (should be 7). On exit, d0.b
 * contains the response byte.
 */
slowCmd:
	move.b	#0xFF, SPIDATB(a0)	/* dummy byte */
	bsr.w	delay
	or.b	#0x40, d0
	move.b	d0, SPIDATB(a0)		/* command byte */
	bsr.w	delay
	move.b	#0x00, SPIDATB(a0)	/* param MSB */
	bsr.w	delay
	move.b	#0x00, SPIDATB(a0)	/* param */
	bsr.w	delay
	move.b	#0x00, SPIDATB(a0)	/* param */
	bsr.w	delay
	move.b	#0x00, SPIDATB(a0)	/* param LSB */
	bsr.w	delay
	move.b	#0x95, SPIDATB(a0)	/* CRC */
	bsr.w	delay
	move.b	#0xFF, SPIDATB(a0)	/* ignore return byte */
	bsr.w	delay
0:	move.b	#0xFF, SPIDATB(a0)	/* get response */
	bsr.w	delay
	move.b	SPIDATB(a0), d0
	cmp.b	#0xFF, d0
	beq.s	0b
	rts

/*---------------------- Issue a command in turbo mode -----------------------*/
fastCmd:
	or.w	#0xFF40, d0
	move.w	d0, SPIDATW(a0)
	swap	d1
	move.w	d1, SPIDATW(a0)
	swap	d1
	move.w	d1, SPIDATW(a0)
	move.w	#0x95FF, SPIDATW(a0)	/* dummy CRC & return byte */
0:	move.b	#0xFF, SPIDATB(a0)	/* get response */
	move.b	SPIDATB(a0), d0
	cmp.b	#0xFF, d0
	beq.s	0b
	rts

/*---------------------------- Initialise SD-Card ----------------------------*/
sdInit:
	lea	0xA13000, a0

initRetry:
	/* 256 clocks at 400kHz with DI low */
	move.w	#0, SPICON(a0)
	moveq	#31, d0		/* 32*8 = 256 */
	moveq	#7, d1		/* enough delay for one byte @400kHz */
0:	move.b	#0x00, SPIDATB(a0)
	bsr.w	delay
	dbra	d0, 0b

	/* 80 clocks at 400kHz with DI high */
	moveq	#9, d0		/* 10*8 = 80 */
0:	move.b	#0xFF, SPIDATB(a0)
	bsr.w	delay
	dbra	d0, 0b

	/* Bring CS low and send CMD0 to reset and put the card in SPI mode */
	move.w	#SDCARD, SPICON(a0)
	move.b	#CMD_GO_IDLE_STATE, d0
	bsr.w	slowCmd
	cmp.b	#IN_IDLE_STATE, d0
	beq.s	initIdle
	
	/* Send a couple of dummy bytes */
	move.b	#0xFF, SPIDATB(a0)
	bsr.w	delay
	move.b	#0xFF, SPIDATB(a0)
	bsr.w	delay
	bra.s	initRetry

initIdle:
	/* Tell the card to initialize itself with ACMD41 */
	move.b	#CMD_APP_CMD, d0
	bsr.w	slowCmd
	move.b	#CMD_APP_SEND_OP_COND, d0
	bsr.w	slowCmd
0:	move.b	#CMD_SEND_OP_COND, d0
	bsr.w	slowCmd
	tst.b	d0
	bne.s	0b

	/* Send a couple of dummy bytes */
	move.b	#0xFF, SPIDATB(a0)
	bsr.w	delay
	move.b	#0xFF, SPIDATB(a0)
	bsr.w	delay

	/* Deselect SD-card */
	move.w	#0, SPICON(a0)
	rts

/*------------------------- Read one or more blocks --------------------------*/
sdReadBlocks:
	/* Send the "read multiple blocks" command */
	lea	0xA13000, a0	/* UMDKv2 register base */
	move.l	12(sp), a1	/* buffer address */
	move.l	4(sp), d1	/* block start */
	asl.l	#8, d1		/* convert block addr to byte addr */
	add.l	d1, d1
	move.w	#(SDCARD | TURBO), SPICON(a0)
	move.b	#CMD_READ_MULTIPLE_BLOCKS, d0
	bsr.w	fastCmd

	/* Receive data from the card, saving to (a1)+ */
	move.w	10(sp), d1	/* block count */
	subq.w	#1, d1		/* dbra needs N-1 */
waitReadToken:
	move.b	#0xFF, SPIDATB(a0)
	cmp.b	#TOKEN_READ_MULTIPLE, SPIDATB(a0)
	bne.s	waitReadToken

	move.w	#0xFFFF, 0(a0)	/* read one 512-byte block */
	moveq	#31, d0
0:	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	move.w	0(a0), (a1)+
	dbra	d0, 0b
	dbra	d1, waitReadToken

	/* Stop reading */
	move.b	#CMD_STOP_TRANSMISSION, d0
	bsr.w	fastCmd

	/* Deselect card and exit */
	move.w	#TURBO, SPICON(a0)
	rts

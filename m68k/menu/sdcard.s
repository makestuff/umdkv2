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
	CMD_SEND_IF_COND         = 8
	CMD_STOP_TRANSMISSION    = 12
	CMD_READ_SINGLE_BLOCK    = 17
	CMD_READ_MULTIPLE_BLOCKS = 18
	CMD_APP_SEND_OP_COND     = 41
	CMD_APP_CMD              = 55
	CMD_READ_OCR             = 58

	TOKEN_SUCCESS            = 0x00
	TOKEN_READ_SINGLE        = 0xFE
	TOKEN_READ_MULTIPLE      = 0xFE
	IN_IDLE_STATE            = (1<<0)

/*---------------------- Delay for when in 400kHz mode -----------------------*/
delay:
	move.l %d1, -(%sp)
	moveq  #8, %d1                             /* enough delay for one byte @400kHz */
0:	dbra   %d1, 0b
	move.l (%sp)+, %d1
	rts

/*------------------------------------------------------------------------------
 * slowCmd() sends the command in d0.b with zero argument, and with sufficient
 * delay between bytes to work in the 400kHz mode (necessary during init). It
 * relies on d1.w containing the delay calibration (should be 8). On exit, d0.b
 * contains the response byte.
 */
slowCmd:
	move.b #0xFF, SPIDATB(%a0)                 /* dummy byte */
	bsr.w  delay
	or.b   #0x40, %d0
	move.b %d0, SPIDATB(%a0)                   /* command byte */
	bsr.w  delay
	rol.l  #8, %d1
	move.b %d1, SPIDATB(%a0)                   /* param MSB */
	bsr.w  delay
	rol.l  #8, %d1
	move.b %d1, SPIDATB(%a0)                   /* param */
	bsr.w  delay
	rol.l  #8, %d1
	move.b %d1, SPIDATB(%a0)                   /* param */
	bsr.w  delay
	rol.l  #8, %d1
	move.b %d1, SPIDATB(%a0)                   /* param LSB */
	bsr.w  delay
	move.b #0x95, SPIDATB(%a0)                 /* CRC */
	bsr.w  delay
	move.b #0xFF, SPIDATB(%a0)                 /* ignore return byte */
	bsr.w  delay
0:	move.b #0xFF, SPIDATB(%a0)                 /* get response */
	bsr.w  delay
	move.b SPIDATB(%a0), %d0
	cmp.b  #0xFF, %d0
	beq.s  0b
	rts


/*------------------------------------------------------------------------------
 * sendCMD8() - Same as slowCmd() but only sends CMD_SEND_IF_COND with the
 * necessary paremeters and CRC for enabling SDHC cards
 */
sendCMD8:
	move.b #0xFF, SPIDATB(%a0)                 /* dummy byte */
	bsr.w  delay
	move.b #0x48, SPIDATB(%a0)                 /* command byte */
	bsr.w  delay
	move.b #0x00, SPIDATB(%a0)                 /* param MSB */
	bsr.w  delay
	move.b #0x00, SPIDATB(%a0)                 /* param */
	bsr.w  delay
	move.b #0x01, SPIDATB(%a0)                 /* param */
	bsr.w  delay
	move.b #0xAA, SPIDATB(%a0)                 /* param LSB */
	bsr.w  delay
	move.b #0x87, SPIDATB(%a0)                 /* CRC */
	bsr.w  delay
	move.b #0xFF, SPIDATB(%a0)                 /* ignore return byte */
	bsr.w  delay
0:	move.b #0xFF, SPIDATB(%a0)                 /* get response */
	bsr.w  delay
	move.b SPIDATB(%a0), %d0
	cmp.b  #0xFF, %d0
	beq.s  0b
	rts

/*---------------------- Issue a command in turbo mode -----------------------*/
fastCmd:
	or.w   #0xFF40, %d0
	move.w %d0, SPIDATW(%a0)
	swap   %d1
	move.w %d1, SPIDATW(%a0)
	swap   %d1
	move.w %d1, SPIDATW(%a0)
	move.w #0x95FF, SPIDATW(%a0)               /* dummy CRC & return byte */
	/* continue into getFastResponse */

/*---------------- Get additional response byte from command -----------------*/
getFastResponse:
	move.b #0xFF, SPIDATB(%a0)
	move.b SPIDATB(%a0), %d0
	cmp.b  #0xFF, %d0
	beq.s  getFastResponse
	rts

getFastResponseNextByte:
	move.b #0xFF, SPIDATB(%a0)
	move.b SPIDATB(%a0), %d0
	rts

/*---------------------------- Initialise SD-Card ----------------------------*/
sdInit:
	lea    0xA13000, %a0

initRetry:
	/* 256 clocks at 400kHz with DI low */
	move.w #0, SPICON(%a0)
	moveq  #31, %d0                            /* 32*8 = 256 */
0:	move.b #0x00, SPIDATB(%a0)
	bsr.w  delay
	dbra   %d0, 0b

	/* 80 clocks at 400kHz with DI high */
	moveq  #9, %d0                             /* 10*8 = 80 */
0:	move.b #0xFF, SPIDATB(%a0)
	bsr.w  delay
	dbra   %d0, 0b

	/* Bring CS low and send CMD0 to reset and put the card in SPI mode */
	move.w #SDCARD, SPICON(%a0)
	move.b #CMD_GO_IDLE_STATE, %d0
	moveq  #0, %d1
	bsr.w  slowCmd
	cmp.b  #IN_IDLE_STATE, %d0
	beq.s  initIdle
	
	/* Send a couple of dummy bytes */
	move.b #0xFF, SPIDATB(%a0)
	bsr.w  delay
	move.b #0xFF, SPIDATB(%a0)
	bsr.w  delay
	bra.s  initRetry

initIdle:
	/* Tell the card to initialize itself with ACMD41 */
	bsr.w  sendCMD8
	moveq  #0, %d1
0:	move.b #CMD_APP_CMD, %d0
	bsr.w  slowCmd
	move.b #CMD_APP_SEND_OP_COND, %d0
	move.l #0x40000000, %d1
	bsr.w  slowCmd
	tst.b  %d0
	bne.s  0b

	/* Send a couple of dummy bytes */
	move.b #0xFF, SPIDATB(%a0)
	bsr.w  delay
	move.b #0xFF, SPIDATB(%a0)
	bsr.w  delay

	/* Deselect SD-card */
	move.w #0, SPICON(%a0)
	rts

/*------------------------- Read one or more blocks --------------------------*/
sdReadBlocks:
	lea    0xA13000, %a0                       /* UMDKv2 register base */
	move.l 12(%sp), %a1                        /* buffer address */
	move.w #(TURBO | SDCARD), SPICON(%a0)      /* Set SPI mode */

	/* Check if we have a standard or high capacity card */
	/* SDSC cards have their address in bytes, SDHC has */
	/* them in blocks (512bytes / block) */
	move.b #CMD_READ_OCR, %d0
	moveq  #0, %d1
	bsr.w  fastCmd
	tst.b  %d0
	bne.s  error

	bsr.w  getFastResponseNextByte             /* We only care about this byte of the response */
	move.l 4(%sp), %d1                         /* block start address */
	btst.b #6, %d0                             /* Bit is set for SDHC cards */
	bne.s  skidAddressConvert
	asl.l  #8, %d1                             /* SDSC - convert block addr to byte addr */
	add.l  %d1, %d1
skidAddressConvert:
	bsr.w  getFastResponseNextByte             /* Remaning response from CMD_READ_OCR is ignored */
	bsr.w  getFastResponseNextByte
	bsr.w  getFastResponseNextByte

	move.b #CMD_READ_MULTIPLE_BLOCKS, %d0 
	bsr.w  fastCmd
	tst.b  %d0
	bne.s  error

	/* Receive data from the card, saving to (a1)+ */
	move.w 10(%sp), %d1                        /* block count */
	subq.w #1, %d1                             /* dbra needs N-1 */

waitReadToken:
	bsr.w  getFastResponse
	cmp.b  #TOKEN_READ_MULTIPLE, %d0
	bne.s  error

	/* read one 512-byte block */
	move.w #0xFFFF, 0(%a0)
	moveq  #31, %d0
0:
	move.w 0(%a0), (%a1)+
	move.w 0(%a0), (%a1)+
	move.w 0(%a0), (%a1)+
	move.w 0(%a0), (%a1)+
	move.w 0(%a0), (%a1)+
	move.w 0(%a0), (%a1)+
	move.w 0(%a0), (%a1)+
	move.w 0(%a0), (%a1)+
	dbra   %d0, 0b
	dbra   %d1, waitReadToken

error:
	/* Stop reading */
	move.b #CMD_STOP_TRANSMISSION, %d0
	bsr.w  fastCmd

	/* Wait for idle */
1:	move.b #0xFF, SPIDATB(%a0)
	cmp.b  #0xFF, SPIDATB(%a0)
	bne.s  1b

	/* Deselect card and exit */
	move.w #TURBO, SPICON(%a0)
	rts

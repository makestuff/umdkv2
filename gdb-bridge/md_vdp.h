#ifndef MD_VDP_H
#define MD_VDP_H


/*******************************************************************
 *      UMDKv2
 *******************************************************************/

/*******************************************************************
 *			Name:				md_vdp.h
 *			Author:				C.Smith - MintyTheCat.
 *			Purpose:			None.
 *			Date-First:			08/01/2016.
 *			Date-Last:			08/01/2016.
 *			Notes:				TS=4.
 *			Changes:			08/01/2016:
 *
 *								Initial Work.
 *
 *******************************************************************/

/*******************************************************************
 *	[Includes]
 *******************************************************************/

#include <libfpgalink.h>
#include <liberror.h>
#include "common.h"

/*******************************************************************
 *	[Data Types]
 *******************************************************************/

/**************************************************
 *	[MD VDP Registers]
 **************************************************/

/*	VDP Status:	*/
struct	t_ugb_vdpstat_st
{
	unsigned int	fill:6;		/*	Filler, should read as 0b001101.	*/
	unsigned int	f1:1;		/*	FIFO Status 1.						*/
	unsigned int	f2:1;		/*	FIFO Status 2.						*/
	unsigned int	vip:1;		/*	V-Int occurred.						*/
	unsigned int	so:1;		/*	scanline-overload.					*/
	unsigned int	sc:1;		/*	sprite-collision.					*/
	unsigned int	of:1;		/*	interlace: odd-frame.				*/
	unsigned int	vb:1;		/*	V-Blank Status.						*/
	unsigned int	hb:1;		/*	H-Blank Status.						*/
	unsigned int	dma:1;		/*	DMA Status.							*/
	unsigned int	pal:1;		/*	PAL Mode.							*/
};

union	t_ugb_vdpstat_u
{
	struct	t_ugb_vdpstat_st	vdpStatAg;
	uint16						vdpStatRaw;
};

/*	Mode 1:	*/
struct	t_ugd_vdpreg00_mode1_st
{
	unsigned int	vram_128KB:1;					/*	Additional 64KB of external VRAM for 128KB. */
	unsigned int	fill3:1;
	unsigned int	leftmost8PixBlank:1;			/*	Left most 8 Pixels are blanked.				*/
	unsigned int	ie1_hint:1;						/*	Horizontal-Interrupt enabled.				*/
	unsigned int	fill2:1;
	unsigned int	fill1:1;
	unsigned int	hvCounter:1;					/*	H/V counter enabled.						*/
	unsigned int	dispEnabled1:1;					/*	Display enabled 1.							*/
};

union	t_ugd_vdpreg00_mode1_u
{
	struct	t_ugd_vdpreg00_mode1_st		mode1Ag;
	uint8								mode1Raw;
};

//TODO: add in the rest of the regs.

/*******************************************************************
 *	[Constants]
 *******************************************************************/

/*******************************************************************
 *	[Globals]
 *******************************************************************/

/*******************************************************************
 *	[Macros]
 *******************************************************************/

/*******************************************************************
 *	[Function Prototypes]
 *******************************************************************/

/*int umdkReadBytes(
	struct FLContext *handle, uint32 address, const uint32 count, uint8 *const data,
	const char **error)

	int umdkDumpRAM(struct FLContext *handle, const char *fileName, const char **error) {

		unsigned int * vdpStat = regBuf+3;
*/

/**************************************************
 *	[MD VDP Status]
 **************************************************/
enum	t_ugb_moncmd_return_code_e
UGB_VDP_VDPStatus(struct FLContext *handle,char * statStr,uint16 statStrMaxLen,const char **error);

/**************************************************
 *	[MD VDP HV Counter]
 **************************************************/
enum	t_ugb_moncmd_return_code_e
UGB_VDP_GetVDPHVCnt(struct FLContext *handle,char * hvCntStr,uint16 hvCntStrMaxLen,enum	t_ugb_bool_e vdpinterlace,const char **error);

/**************************************************
 *	[MD VDP Debug]
 **************************************************/

/**************************************************
 *	[MD VDP Standard Registers]
 **************************************************/
enum	t_ugb_moncmd_return_code_e
UGB_VDP_GetVDPModeReg1(struct FLContext *handle,uint16 *vdpStat,const char **error);

/**************************************************
 *	[MD VDP CRAM]
 **************************************************/
enum	t_ugb_moncmd_return_code_e
UGB_VDP_DumpCRAM(struct FLContext *handle,char * cramDumpStr,uint16 cramDumpStrMaxLen,uint16 palNum,const char **error);


/*	TODO: Remove dummy.	*/

enum	t_ugb_moncmd_return_code_e
UGB_VDP_DummyTest(struct FLContext *handle,char * cramDumpStr,uint16 cramDumpStrMaxLen,uint16 palNum,const char **error);

/*******************************************************************
 *	[Functions]
 *******************************************************************/

/*******************************************************************
 *   FunctionName:   None.
 *   Synopsis:       None.
 *   Input:          None.
 *   Output:         None.
 *   Returns:        None.
 *   Description:    None.
 *   Cautions:       None.
 *   Comments:       None.
 *******************************************************************/

#endif	/*	MD_VDP_H	*/

/*******************************************************************
 *      UMDKv2
 *******************************************************************/

/*******************************************************************
 *			Name:				md_vdp.c
 *			Author:				C.Smith - MintyTheCat.
 *			Purpose:			Megadrive VDP module for the UMDKv2 GDB-Bridge's Monitor.
 *			Date-First:			08/01/2016.
 *			Date-Last:			23/01/2016.
 *			Notes:				TS=4.
 *			Changes:			08/01/2016:
 *
 *								Initial Work.
 *
 *								Added UGB_VDP_VDPStatus.
 *
 * 								23/01/2016:
 *
 *								issue with UGB_VDP_DumpCRAM - dumps incorrect.
 *
 *******************************************************************/

/*******************************************************************
 *	[Includes]
 *******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libfpgalink.h>
#include <liberror.h>
#include "md_vdp.h"
#include "mem.h"

/*******************************************************************
 *	[Data Types]
 *******************************************************************/

/*******************************************************************
 *	[Constants]
 *******************************************************************/

#define MD_BUFF_SIZE1	1024

/**************************************************
 *	[MD VDP Registers]
 **************************************************/

#define MD_VDP_DATA		0xC00000
#define MD_VDP_CNTL		0xC00004
#define MD_VDP_STAT		0xC00004
#define MD_VDP_HVCNT	0xC00008
#define MD_VDP_DBG		0xC0001C

/**************************************************
 *	[MD VDP H/V Counter]
 **************************************************/

#define MD_HVCNT_HMASK		0x00FF
#define MD_HVCNT_VMASK		0xFF00

/**************************************************
 *	[MD VDP CRAM]
 **************************************************/

#define MD_CRAM_CRAMBUFLEN	0x80
#define MD_CRAM_RD_MODE		0x8
#define MD_CRAM_SIZE		0x80
#define MD_CRAM_SIZE_W		(MD_CRAM_SIZE/2)

/*******************************************************************
 *	[Globals]
 *******************************************************************/

/*******************************************************************
 *	[Macros]
 *******************************************************************/

/*******************************************************************
 *	[Function Prototypes]
 *******************************************************************/

/*******************************************************************
 *	[Functions]
 *******************************************************************/

/*******************************************************************
 *   FunctionName:   NUGB_VDP_VDPStatus.
 *   Synopsis:       None.
 *   Input:          None.
 *   Output:         None.
 *   Returns:        None.
 *   Description:    None.
 *   Cautions:       None.
 *   Comments:       None.
 *******************************************************************/
enum	t_ugb_moncmd_return_code_e
UGB_VDP_VDPStatus(struct FLContext *handle,char *statStr,uint16 statStrMaxLen,const char **error)
{
	int							status;
	union	t_ugb_vdpstat_u		vdpStat;

	if(0 ==statStrMaxLen)
	{
		return(ugb_moncmd_fail_nullargs);
		/*	TODO: error.	*/
	}
	if(NULL	==statStr)
	{
		return(ugb_moncmd_fail_args);
		/*	TODO: error.	*/
	}
	status = umdkReadWord(handle,MD_VDP_STAT,&vdpStat.vdpStatRaw, error);	/*	TODO: handle status.	*/

	snprintf(statStr,statStrMaxLen, "VDP status (raw) = 0x%.2X\n", vdpStat.vdpStatRaw);	/*	Raw VDP status-register.	*/

	/*	VDP FIFO Status:	*/
	if((0 ==vdpStat.vdpStatAg.f1)	&&(0 ==vdpStat.vdpStatAg.f2))
	{	strcat(statStr,"VDP FIFO: frozen\n");	}
	else if(1	==vdpStat.vdpStatAg.f1)
	{	strcat(statStr,"VDP FIFO: empty\n");	}
	else if(1	==vdpStat.vdpStatAg.f2)
	{	strcat(statStr,"VDP FIFO: full\n");	}

	/*	VDP V-Int Status:	*/
	if(0 ==vdpStat.vdpStatAg.vip)
	{	strcat(statStr,"VDP V-Int: not occurred\n");	}
	else
	{	strcat(statStr,"VDP V-Int: occurred\n");	}

	/*	VDP Scanline-Overload Status:	*/
	if(0 ==vdpStat.vdpStatAg.so)
	{	strcat(statStr,"VDP SO: undetected\n");	}
	else
	{	strcat(statStr,"VDP SO: detected\n");	}
		
	/*	VDP Sprite-Collision Status:	*/
	if(0 ==vdpStat.vdpStatAg.sc)
	{	strcat(statStr,"VDP SC: undetected\n");	}
	else
	{	strcat(statStr,"VDP SC: detected\n");	}

	/*	VDP Frame Status:	*/
	if(0 ==vdpStat.vdpStatAg.of)
	{	strcat(statStr,"VDP OF: even frame\n");	}
	else
	{	strcat(statStr,"VDP OF: odd frame\n");	}
		
	/*	VDP V-Blank Status:	*/
	if(0 ==vdpStat.vdpStatAg.vb)
	{	strcat(statStr,"VDP V-Blank: non blanking\n");	}
	else
	{	strcat(statStr,"VDP V-Blank: blanking\n");	}

	/*	VDP H-Blank Status:	*/
	if(0 ==vdpStat.vdpStatAg.hb)
	{	strcat(statStr,"VDP H-Blank: non blanking\n");	}
	else
	{	strcat(statStr,"VDP H-Blank: blanking\n");	}

	/*	VDP DMA Status:	*/
	if(0 ==vdpStat.vdpStatAg.dma)
	{	strcat(statStr,"VDP DMA: inactive\n");	}
	else
	{	strcat(statStr,"VDP DMA: active\n");	}

	/*	VDP PAL Status:	*/
	if(0 ==vdpStat.vdpStatAg.pal)
	{	strcat(statStr,"VDP PAL: set\n");	}
	else
	{	strcat(statStr,"VDP PAL: not set\n");	}

	return(ugb_moncmd_success);
}

/*******************************************************************
 *   FunctionName:   UGB_VDP_GetVDPHVCnt.
 *   Synopsis:       None.
 *   Input:          None.
 *   Output:         None.
 *   Returns:        None.
 *   Description:    None.
 *   Cautions:       None.
 *   Comments:       None.
 *******************************************************************/

enum	t_ugb_moncmd_return_code_e
UGB_VDP_GetVDPHVCnt(struct FLContext *handle,char * hvCntStr,uint16 hvCntStrMaxLen,enum	t_ugb_bool_e vdpinterlace,const char **error)
{
	int							status;
	uint16						vdpHVCnt	=0;

	/*	Checks:	*/
	if((NULL ==hvCntStr)	||(NULL ==handle))
	{
		return(ugb_moncmd_fail_nullargs);
	}

	status = umdkReadWord(handle,MD_VDP_HVCNT,&vdpHVCnt,error);	/*	TODO: handle status.	*/

	*hvCntStr	='\0';
	if(f_false	==vdpinterlace)
	{
		snprintf(hvCntStr,hvCntStrMaxLen, "VDP HV Cnt (raw) = 0x%.2X\n(non-interlace) H counter = 0x%.2X, V counter = 0x%.2X\n",vdpHVCnt,(MD_HVCNT_HMASK &vdpHVCnt),((MD_HVCNT_VMASK &vdpHVCnt)>>0x8));
	}
	else
	{
		snprintf(hvCntStr,hvCntStrMaxLen, "VDP HV Cnt (raw) = 0x%.2X\n(interlace) H counter = 0x%.2X, V counter = 0x%.2X\n",vdpHVCnt,(MD_HVCNT_HMASK &vdpHVCnt),((MD_HVCNT_VMASK &vdpHVCnt)>>0x8));
	}

	return(ugb_moncmd_success);
}
/**************************************************
 *	[MD VDP CRAM]
 **************************************************/
enum	t_ugb_moncmd_return_code_e
UGB_VDP_DumpCRAM(struct FLContext *handle,char * cramDumpStr,uint16 cramDumpStrMaxLen,uint16 palNum,const char **error)
{
	uint16	cramTmpBuf[MD_CRAM_CRAMBUFLEN];
	char	tmpBuf1[MD_BUFF_SIZE1];
	char	tmpBuf2[MD_BUFF_SIZE1];
	uint16	cramAddr		=0;
	uint8	iter_cram		=0;
	int		status			=0;
	uint16 dummyCRAM =0xFF;
	uint16	W1=0,W2=0;

	/*	Checks:	*/
	if(palNum>3)
	{
		/*	TODO: error handling/etc.	*/
		return(ugb_moncmd_fail_args);
	}

	if((NULL ==cramDumpStr)	||(NULL ==handle))
	{
		return(ugb_moncmd_fail_nullargs);
	}

	/*	Init:	*/
	memset(cramTmpBuf,0,MD_CRAM_CRAMBUFLEN);
	memset(tmpBuf1,0,MD_BUFF_SIZE1);
	memset(tmpBuf2,0,MD_BUFF_SIZE1);
	memset(cramDumpStr,0,cramDumpStrMaxLen);

	/*
	 * Read CRAM:
	 *
	 * Note: we are not permitted to read back the VDP's standard registers without using a local shadow and we-
	 * cannot alter the auto-increment settings for the VDP's read accesses and as such must read each colour slot-
	 * individually.
	 *
	 */

	strcpy(cramDumpStr,"\n--------------------------------------------------------------------------------------------------------");
	strcat(cramDumpStr,"\n|Pal # |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |");
	strcat(cramDumpStr,"\n--------------------------------------------------------------------------------------------------------");

	cramAddr=(palNum*0x20);	/*	Palettes are 16*2 Bytes in length, set the palette to dump's start address.	*/

	//TODO: There are problems here, I cannot read back some CRAM addresses correctly.
	for(iter_cram=0;iter_cram<16;iter_cram++)
	{

		if(0	==(iter_cram %16))
		{
			snprintf(tmpBuf2,MD_BUFF_SIZE1, "\n|  %d   |",(iter_cram/16));
			strcat(cramDumpStr,tmpBuf2);
		}

		W1=(((MD_CRAM_RD_MODE<<14)&0x3FFF)|(cramAddr&0x3FFF));
		W2=(((MD_CRAM_RD_MODE&0x3C)<<2)|((cramAddr&0xC000)>>14));
		status = umdkWriteWord(handle,MD_VDP_CNTL,W1, error);
		status = umdkWriteWord(handle,MD_VDP_CNTL,W2, error);
		status = umdkReadWord(handle,MD_VDP_DATA,&dummyCRAM, error);	

		snprintf(tmpBuf1,MD_BUFF_SIZE1, "%.4X |",dummyCRAM);	//OK.
		strcat(cramDumpStr,tmpBuf1);

		cramAddr+=2;
	}

	//TODO: !!!! breaks on socket size being only 1024 - concrete?
//	strcat(cramDumpStr,"\n--------------------------------------------------------------------------------------------------------");

	return(ugb_moncmd_success);
}


enum	t_ugb_moncmd_return_code_e
UGB_VDP_DummyTest(struct FLContext *handle,char * cramDumpStr,uint16 cramDumpStrMaxLen,uint16 palNum,const char **error)
{		
	uint16	cramTmpBuf[MD_CRAM_CRAMBUFLEN];
	char	tmpBuf1[MD_BUFF_SIZE1];
	char	tmpBuf2[MD_BUFF_SIZE1];
	uint16	cramAddr		=0;
	uint8	iter_cram		=0;
	int		status			=0;
	uint16 dummyCRAM =0xFF;
	uint16	W1=0,W2=0;
	unsigned long	iter_dummyLoop	=0;

	/*	Checks:	*/

	/*	Init:	*/
	memset(cramTmpBuf,0,MD_CRAM_CRAMBUFLEN);
	memset(tmpBuf1,0,MD_BUFF_SIZE1);
	memset(tmpBuf2,0,MD_BUFF_SIZE1);
	memset(cramDumpStr,0,cramDumpStrMaxLen);

	/*
	 * Read CRAM:
	 *
	 * Note: we are not permitted to read back the VDP's standard registers without using a local shadow and we-
	 * cannot alter the auto-increment settings for the VDP's read accesses and as such must read each colour slot-
	 * individually.
	 *
	 */


	strcpy(cramDumpStr,"\n---------------------------------------");
	strcat(cramDumpStr,"\n|Pal #|0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|");
	strcat(cramDumpStr,"\n---------------------------------------");

	cramAddr=0;

	//TODO: There are problems here, I cannot read back the expected CRAM values correctly.
//	for(iter_cram=0;iter_cram<MD_CRAM_SIZE_W;iter_cram++)
	for(iter_cram=0;iter_cram<16;iter_cram++)
	{
		if(0	==(iter_cram %16))
		{
			snprintf(tmpBuf2,MD_BUFF_SIZE1, "\n| %d |",(iter_cram/16));
			strcat(cramDumpStr,tmpBuf2);
		}
/*		
		W1=(((MD_CRAM_RD_MODE<<14)&0x3FFF)|(cramAddr&0x3FFF));
		W2=(((MD_CRAM_RD_MODE&0x3C)<<2)|((cramAddr&0xC000)>>14));
		status = umdkWriteWord(handle,MD_VDP_CNTL,W1, error);
		status = umdkWriteWord(handle,MD_VDP_CNTL,W2, error);
		status = umdkReadWord(handle,MD_VDP_DATA,&dummyCRAM, error);	
//zz		snprintf(tmpBuf1,MD_BUFF_SIZE1, "\nW1=%.4X, W2=%.4X|",W1,W2);	//W2.
//zz		strcat(cramDumpStr,tmpBuf1);
*/

//		for(iter_dummyLoop=0;iter_dummyLoop<0xFFFF; iter_dummyLoop++);
//		for(iter_dummyLoop=0;iter_dummyLoop<0xFFFF; iter_dummyLoop++);
//		for(iter_dummyLoop=0;iter_dummyLoop<0xFFFF; iter_dummyLoop++);
//		status = umdkWriteLong(handle,MD_VDP_CNTL,( 0x00000020 ), error);		/*	W2:	TODO: handle status.	*/
		status = umdkWriteLong(handle,MD_VDP_CNTL,(0x00020020), error);
	
//		for(iter_dummyLoop=0;iter_dummyLoop<0xFFFF; iter_dummyLoop++);
//		for(iter_dummyLoop=0;iter_dummyLoop<0xFFFF; iter_dummyLoop++);
//		for(iter_dummyLoop=0;iter_dummyLoop<0xFFFF; iter_dummyLoop++);


		status = umdkReadWord(handle,MD_VDP_DATA,&dummyCRAM, error);

		snprintf(tmpBuf1,MD_BUFF_SIZE1, "\n: %.4X|",dummyCRAM);	//W2.
		strcat(cramDumpStr,tmpBuf1);
		
		if(0	==(iter_cram %16))
		{
//			strcat(tmpBuf,"\n");
		//	snprintf(cramDumpStr,cramDumpStrMaxLen, "|%d|%s|\n",(iter_cram/16),tmpBuf);
		}

	cramAddr+=2;
	}

	//TODO: !!!! breaks on socket size being only 1024 - concrete?
	strcat(cramDumpStr,"\n---------------------------------------");
	
//	snprintf(cramDumpStr,cramDumpStrMaxLen, "0x%.2X\n", vdpStat.vdpStatRaw);	/*	Raw VDP status-register.	*/


	return(ugb_moncmd_success);
}
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

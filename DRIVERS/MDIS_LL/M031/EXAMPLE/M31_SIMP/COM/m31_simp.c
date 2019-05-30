/****************************************************************************
 ************                                                    ************
 ************                   m31_simp.c                       ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ds
 *
 *  Description: Simple example program for the M31 MDIS driver
 *                      
 *     Required: -
 *     Switches: NO_MAIN_FUNC
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 1998-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>	/* men type definitions */
#include <MEN/usr_oss.h>	/* user mode system services */
#include <MEN/mdis_api.h>	/* mdis user interface */
#include <MEN/mdis_err.h>	/* mdis error definitions */
#include <MEN/m31_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int M31_Simple( char *devName );
static void ShowError( void );


#ifndef NO_MAIN_FUNC
/******************************** main **************************************
 *
 *  Description:  main() - function
 *
 *---------------------------------------------------------------------------
 *  Input......:  argc		number of arguments
 *				  *argv		pointer to arguments
 *				  argv[1]	device name	 
 *
 *  Output.....:  return	0	if no error
 *							1	if error
 *
 *  Globals....:  -
 ****************************************************************************/
 int main( int argc, char *argv[ ] )
 {
	if( argc < 2){
		printf("usage: m31_simp <device name>\n");
		return 1;
	}
	M31_Simple(argv[1]);
	return 0;
 }
#endif/* NO_MAIN_FUNC */


/******************************* M31_Simple *********************************
 *
 *  Description:  - open the device
 *				  - configure the device
 *				  - read operations
 *                - produce an error
 *				  - close the device
 *
 *---------------------------------------------------------------------------
 *  Input......:  DeviceName
 *
 *  Output.....:  return	0	if no error
 *							1	if error
 *
 *  Globals....:  -
 ****************************************************************************/
int M31_Simple( char *devName )
{
	MDIS_PATH	devHdl;		/* device handle */
	int32	    data;
	u_int16     buf;
	int16	    ch;
	int32	    byteCount;

    printf("\n");
	printf("m31_simp - simple example program for the M31 module\n");
    printf("====================================================\n\n");

    printf("%s\n\n", IdentString);

    /*----------------------------------+  
    | M_open - open the device          |
    +----------------------------------*/
	printf("M_open() - open the device\n");
	printf("--------------------------\n");
	if( (devHdl = M_open(devName)) < 0 ) goto CLEANUP;
	printf(" device %s opened\n\n", devName);

    /*----------------------------------+  
    | M_getstat - get device properties |
    +----------------------------------*/
	printf("M_getstat() - get device properties\n");
	printf("-----------------------------------\n");
	/* number of channels */
	if ( M_getstat(devHdl,M_LL_CH_NUMBER, &data) < 0 ) goto CLEANUP;
	printf(" number of channels:      %ld\n", data);
	/* channel length */
	if ( M_getstat(devHdl,M_LL_CH_LEN, &data) < 0 ) goto CLEANUP;
	printf(" channel length:          %ldBit\n", data);
	/* channel direction */
	if ( M_getstat(devHdl,M_LL_CH_DIR, &data) < 0 ) goto CLEANUP;
	printf(" channel direction type:  %ld\n", data);
	/* channel type */
	if ( M_getstat(devHdl,M_LL_CH_TYP, &data) < 0 ) goto CLEANUP;
	printf(" channel type:            %ld\n\n", data);

    /*----------------------------------+  
    | M_setstat - device configuration  |
    +----------------------------------*/
	printf("M_setstat() - set current channel to 0\n");
	printf("--------------------------------------\n");
	/* set current channel to 0 */
	if( M_setstat( devHdl, M_MK_CH_CURRENT, 0 ) < 0 ) goto CLEANUP;
	printf(" OK\n\n");

	printf("M_setstat() - set auto-increment mode\n");
	printf("-------------------------------------\n");
	/* set auto-increment mode */
	if( M_setstat( devHdl, M_MK_IO_MODE, M_IO_EXEC_INC ) < 0 ) goto CLEANUP;
	printf(" OK\n\n");

    /*----------------------------------+  
    | M_read - read operation           |
    +----------------------------------*/
	printf("M_read() - read ch 0..15\n");
	printf("------------------------\n");
	
	for (ch=0; ch<=15; ch++){
	/* read one channel (then autoincrement) */
		if ( M_read(devHdl, &data) < 0 ) goto CLEANUP;
		printf(" channel %2d : %ld\n", ch, data);
	}
	printf("\n");

    /*----------------------------------+  
    | M_getblock - getblock operation   |
    +----------------------------------*/
	printf("M_getblock() - read all channels\n");
	printf("--------------------------------\n");

	/* get all channels (data in 2 bytes buffer) */
	byteCount = M_getblock(devHdl, (u_int8*)&buf, 2);
	if( byteCount < 0 ) goto CLEANUP;

	printf(" channel: ");
	for (ch=0; ch<=15; ch++)
		printf(" %2d ",ch);
	printf("\n state:   ");
	for (ch=0; ch<=15; ch++)
		printf("  %d ", (buf>>ch) & 0x01);

	printf("\n => M_getblock: %d bytes got\n\n", (int) byteCount);

    /*----------------------------------+  
    | M_setstat - produce an error      |
    +----------------------------------*/
	printf("M_setstat() - produce an error\n");
	printf("------------------------------\n");
	if ( M_setstat(devHdl,M_LL_NOTEXIST, 0) < 0 ) ShowError();
    printf("\n");

    /*----------------------------------+  
    | M_close - close the device        |
    +----------------------------------*/
	printf("M_close() - close the device\n");
	printf("----------------------------\n");
    if( M_close(devHdl) < 0) goto CLEANUP;
	printf(" device %s closed\n\n", devName);

    printf("=> OK\n");
    return 0;


CLEANUP:
    ShowError();
    printf("=> ERROR\n");
    return 1;
}

/******************************** ShowError *********************************
 *
 *  Description:  Show MDIS or OS error message.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  -
 *
 *  Globals....:  -
 ****************************************************************************/
static void ShowError( void )
{
   u_int32 error;

   error = UOS_ErrnoGet();

   printf("*** %s ***\n",M_errstring( error ) );
}




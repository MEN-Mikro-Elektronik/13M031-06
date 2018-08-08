/****************************************************************************
 ************                                                    ************
 ************                   M31_SIG                          ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ds
 *        $Date: 2010/03/11 10:42:58 $
 *    $Revision: 1.2 $
 *
 *  Description: Signal example program for the M31 driver
 *
 *               Demonstrates the usage of signals with the M31 driver. 
 *                      
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m31_sig.c,v $
 * Revision 1.2  2010/03/11 10:42:58  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.1  2001/08/22 17:15:37  Schmidt
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2001 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/m31_drv.h>

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static u_int32 G_SigSum;	/* counter for signals */
static u_int32 G_SigCount;	/* signal occured flag */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void PrintMdisError(char *info);
static void PrintUosError(char *info);
static void __MAPILIB SigHandler( u_int32 sigCode );

/********************************* main *************************************
 *
 *  Description: Program main function
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc,argv	argument counter, data ..
 *  Output.....: return	    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	MDIS_PATH	path = -1;
	int32	    ch, byteCount;
	char	    *device;
	u_int16     state, change;
	
	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m31_sig <device>\n");
		printf("Function: M31 example for signal usage\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("\n");
		return(1);
	}
	
	device = argv[1];

	/*------------------------------------+
	|  install signalhandler and signals  |
	+------------------------------------*/
	/* install signal handler */
	if (UOS_SigInit(SigHandler)) {
		PrintUosError("SigInit");
		return(1);
	}

	/* install signal #1 */
	if (UOS_SigInstall(UOS_SIG_USR1)) {
		PrintUosError("SigInstall");
		goto cleanup;
	}

	/* clear signal sum and counter */
	G_SigSum = 0;
	G_SigCount = 0;

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintMdisError("open");
		return(1);
	}

    /*----------------------+
    |  set signal           |
    +----------------------*/
	/* install UOS_SIG_USR1 signal */ 
	if ((M_setstat(path, M31_SIGSET, UOS_SIG_USR1)) < 0) {
		PrintMdisError("setstat M31_SIGSET (UOS_SIG_USR1)");
		goto cleanup;
	}

    /*----------------------+
    |  enable interrupt     |
    +----------------------*/
	if ((M_setstat(path, M_MK_IRQ_ENABLE, 1)) < 0) {
		PrintMdisError("setstat M_MK_IRQ_ENABLE");
		goto cleanup;
	}

	/*--------------------+
    |  wait on signals    |
    +--------------------*/
	printf("Waiting for signals... (Press Key to abort)\n");
	
	while( TRUE ){

		if( G_SigCount ){

			G_SigCount--;

			printf("\n\007>>> Signal received (channel state changed) <<<\n");

			/*--------------------+
			|  get change flags   |
			+--------------------*/
			if( (byteCount = M_getstat(path, M31_CHANGE_FLAGS, (int32*)&change)) < 0 ){
				PrintMdisError("getstat M31_CHANGE_FLAGS");
				goto cleanup;
			}

			/*--------------------+
			|  read all states    |
			+--------------------*/
			if( (byteCount = M_getblock(path, (u_int8*)&state, 2)) < 0 ) {
				PrintMdisError("M_getblock");
				goto cleanup;
			}

			printf(" channel: ");
			for (ch=0; ch<=15; ch++)
				printf(" %2d ", (int)ch);

			printf("\n change:  ");
			for (ch=0; ch<=15; ch++)
				printf("  %d ", (change>>ch) & 0x01);

			printf("\n state:   ");
			for (ch=0; ch<=15; ch++)
				printf("  %d ", (state>>ch) & 0x01);

			printf("\n");
		}
		else{
			UOS_Delay( 500 );	/* delay 500ms */
			printf(".");
		}

		if( UOS_KeyPressed() >= 0 )
			break;
	}	

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	cleanup:
	
	/* disable interrupt */
	if ((M_setstat(path, M_MK_IRQ_ENABLE, 0)) < 0)
		PrintMdisError("setstat M_MK_IRQ_ENABLE");

    /* clear alarm signals */
	if ((M_setstat(path, M31_SIGCLR, 0)) < 0) {
		PrintMdisError("setstat M31_SIGCLR");
	}

	/* terminate signal handling */
	UOS_SigExit();

	/* print signal counters */
	printf("\n");
	printf("Sum of signals : %u \n", (unsigned int)G_SigSum);

	if (M_close(path) < 0)
		PrintMdisError("close");

	return(0);
}

/********************************* SigHandler *******************************
 *
 *  Description: Signal handler
 *			   
 *---------------------------------------------------------------------------
 *  Input......: sigCode	signal code received
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void __MAPILIB SigHandler( u_int32 sigCode )
{
	if( sigCode == UOS_SIG_USR1 ){
		G_SigSum++;
		G_SigCount++;
	}
}

/********************************* PrintMdisError *******************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void PrintMdisError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}

/********************************* PrintUosError ****************************
 *
 *  Description: Print UOS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void PrintUosError(char *info)
{
	printf("*** can't %s: %s\n", info, UOS_ErrString(UOS_ErrnoGet()));
}


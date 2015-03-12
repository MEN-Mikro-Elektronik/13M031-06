/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m31_drv.h
 *
 *       Author: dieter.pfeuffer@men.de
 *        $Date: 2010/03/11 10:43:47 $
 *    $Revision: 2.4 $
 *
 *  Description: Header file for M31 driver
 *               - M31 specific status codes
 *               - M31 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m31_drv.h,v $
 * Revision 2.4  2010/03/11 10:43:47  amorbach
 * R: driver ported to MDIS5, new MDIS_API and men_typs
 * M: for backward compatibility to MDIS4 optionally define new types
 *
 * Revision 2.3  2004/05/03 14:49:52  cs
 * M31_HYS_MODE set/getstat for M82 added
 *
 * Revision 2.2  2001/08/22 17:15:41  Schmidt
 * 1) function prototype declarations changed to static
 * 2) M31_SIGSET/CLR status codes added
 * 3) driver variant support added
 *
 * Revision 2.1  1998/07/21 16:37:35  Schmidt
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _M31_LLDRV_H
#define _M31_LLDRV_H

#ifdef __cplusplus
      extern "C" {
#endif


/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* M31 specific status codes (STD) */        /* S,G: S=setstat, G=getstat */
#define M31_SIGSET		    M_DEV_OF+0x00	 /* S,G: set/get signal		  */
#define M31_SIGCLR		    M_DEV_OF+0x01	 /* S  : clear signal		  */
#define M31_CHANGE_FLAGS    M_DEV_OF+0x02	 /*   G: get change flags	  */
#define M31_HYS_MODE	    M_DEV_OF+0x03	 /* S,G: set/get hysteresis mode  (for M82 only!) */

/* M31 specific status codes (BLK) */        /* S,G: S=setstat, G=getstat */
/* none */

#ifndef  M31_VARIANT
# define M31_VARIANT M31
#endif

# define _M31_GLOBNAME(var,name) var##_##name
#ifndef _ONE_NAMESPACE_PER_DRIVER_
# define M31_GLOBNAME(var,name) _M31_GLOBNAME(var,name)
#else
# define M31_GLOBNAME(var,name) _M31_GLOBNAME(M31,name)
#endif

#define GetEntry	M31_GLOBNAME(M31_VARIANT,GetEntry)

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void GetEntry(LL_ENTRY* drvP);
#endif
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
    /* we have an MDIS4 men_types.h and mdis_api.h included */
    /* only 32bit compatibility needed!                     */
    #define INT32_OR_64  int32
        #define U_INT32_OR_64 u_int32
    typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
      }
#endif

#endif /* _M31_LLDRV_H */




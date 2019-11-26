/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m31_drv.h
 *
 *       Author: dieter.pfeuffer@men.de
 *
 *  Description: Header file for M31 driver
 *               - M31 specific status codes
 *               - M31 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *---------------------------------------------------------------------------
 * Copyright 1998-2019, MEN Mikro Elektronik GmbH
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




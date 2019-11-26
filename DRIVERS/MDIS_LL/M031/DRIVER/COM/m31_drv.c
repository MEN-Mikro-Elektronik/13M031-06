/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m31_drv.c
 *      Project: M31 module driver 
 *
 *       Author: dieter.pfeuffer@men.de
 *
 *  Description: Low-level driver for M31/M32/M82 M-Modules
 *
 *               The M31/M32/M82 M-Module is a 16-bit binary input M-Module.
 *               The signals of mechanical switches are debounced by a digital
 *               circuit. Each input signal edge generates a non-maskable
 *               interrupt.
 *
 *               The driver provides 16 logical channels (0..15) corresponding
 *               to the 16 binary input lines D00..D15 on the M-Module.   
 *               The channel states can be queried separately for each channel
 *               or through one function call for all channels.
 *
 *               The driver supports interrupts from the M-Module. Each input
 *               signal edge triggers the interrupt service routine which
 *               stores level changes of a channel in a separate flag. 
 *               The flags can be read and reset via GetStat code.
 *               Furthermore an interrupt can inform the application about
 *               input signal edges with a definable user signal.
 *               The signal can be installed for all channels together via
 *               SetStat code.
 *
 *               M82 M-Module specific Set/GetStat code:
 *               The driver provides the M31_HYS_MODE Set/GetStat code to
 *               set/get the hysteresis mode of the current channel. This
 *               Set/GetStat code can only be used for M82 M-Modules but
 *               not for M31/M32 M-Modules.
 *
 *     Required: -
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
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

#define _NO_LL_HANDLE		/* ll_defs.h: don't define LL_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>		/* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general */
#define CH_NUMBER			16			/* nr of device channels */
#define ADDRSPACE_COUNT		1			/* nr of required address spaces */
#define MOD_ID_MAGIC		0x5346      /* EEPROM identification (magic) */
#define MOD_ID_SIZE			128			/* EEPROM size */
#define MOD_ID_M31			31			/* M-Module ID for M31 module */
#define MOD_ID_M32			32			/* M-Module ID for M32 module */
#define MOD_ID_M82			82			/* M-Module ID for M82 module */

/* debug settings */
#define DBG_MYLEVEL		llHdl->dbgLevel
#define DBH             llHdl->dbgHdl

/* register offsets */
#define DATA_REG			0x00		/* data register */
#define MODE_REG			0x04		/* mode register */
#define IRQCRL_REG			0x80		/* interrupt clear register */

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* ll handle */
typedef struct {
	/* general */
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table */
    int32           memAlloc;		/* size allocated for the handle */
    OSS_HANDLE      *osHdl;         /* oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /* irq handle */
    DESC_HANDLE     *descHdl;       /* desc handle */
    MACCESS         ma;             /* hw access handle */
    u_int32         irqCount;		/* irq counter */
	/* debug */
    u_int32         dbgLevel;		/* debug level */
	DBG_HANDLE      *dbgHdl;        /* debug handle */
	/* id */
    u_int32         idCheck;		/* id check enabled */
    /* sig */
	OSS_SIG_HANDLE  *sigHdl;		/* signal handle */
	/* misc */
	u_int16			changeFlags;	/* stores level changes */
	u_int16			lastState;		/* last state */
	u_int8			irqEnable;		/* irq enable flag */
	u_int32			modId;			/* module id */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low-level driver branch table  */
#include <MEN/m31_drv.h>    /* M31 driver header file */

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 M31_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
					   MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
					   OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 M31_Exit(LL_HANDLE **llHdlP );
static int32 M31_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 M31_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 M31_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 M31_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64);
static int32 M31_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 M31_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							 int32 *nbrWrBytesP);
static int32 M31_Irq(LL_HANDLE *llHdl );
static int32 M31_Info(int32 infoType, ... );
static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);


/**************************** M31_GetEntry *********************************
 *
 *  Description:  Initialize driver's branch table
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  drvP  pointer to the initialized branch table structure
 *
 *  Globals....:  -
 ****************************************************************************/
#ifdef _ONE_NAMESPACE_PER_DRIVER_
    extern void LL_GetEntry( LL_ENTRY* drvP )
#else
    extern void GetEntry( LL_ENTRY* drvP )
#endif
{
    drvP->init        = M31_Init;
    drvP->exit        = M31_Exit;
    drvP->read        = M31_Read;
    drvP->write       = M31_Write;
    drvP->blockRead   = M31_BlockRead;
    drvP->blockWrite  = M31_BlockWrite;
    drvP->setStat     = M31_SetStat;
    drvP->getStat     = M31_GetStat;
    drvP->irq         = M31_Irq;
    drvP->info        = M31_Info;
}

/******************************** M31_Init ***********************************
 *
 *  Description:  Allocate and return low-level handle, initialize hardware
 * 
 *                The following descriptor keys are used:
 *
 *                Descriptor key        Default            Range
 *                --------------------  -----------------  -------------
 *                DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT    see dbg.h
 *                DEBUG_LEVEL           OSS_DBG_DEFAULT    see dbg.h
 *                ID_CHECK              1                  0 or 1 
 *
 *---------------------------------------------------------------------------
 *  Input......:  descSpec   pointer to descriptor data
 *                osHdl      oss handle
 *                maHdl      hardware access handle
 *                devSemHdl  device semaphore handle
 *                irqHdl     irq handle
 *
 *  Output.....:  llHdlP     pointer to low-level driver handle
 *                return     success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *maHdl,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    LL_HANDLE	*llHdl = NULL;
    u_int32		gotsize;
    int32		error;
    u_int32		value;
    int			modIdMagic;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
	/* alloc */
    if ((*llHdlP = llHdl = (LL_HANDLE*)
		 OSS_MemGet(osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	/* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

	/* init */
    llHdl->memAlloc = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;
    llHdl->ma		  = *maHdl;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
	/* driver's ident function */
	llHdl->idFuncTbl.idCall[0].identCall = Ident;
	/* libraries' ident functions */
	llHdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
	llHdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
	/* terminator */
	llHdl->idFuncTbl.idCall[3].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
	/* prepare access */
    if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
		return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, &value,
								"DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	DESC_DbgLevelSet(llHdl->descHdl, value);	/* set level */

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, &llHdl->dbgLevel,
								"DEBUG_LEVEL")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    DBGWRT_1((DBH, "LL - M31_Init\n"));

    /* ID_CHECK */
    if ((error = DESC_GetUInt32(llHdl->descHdl, 1, &llHdl->idCheck,
								"ID_CHECK")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    /*------------------------------+
    |  check M-Module ID            |
    +------------------------------*/
	if (llHdl->idCheck) {
		modIdMagic = m_read((U_INT32_OR_64)llHdl->ma, 0);
		llHdl->modId = m_read((U_INT32_OR_64)llHdl->ma, 1);

		if (modIdMagic != MOD_ID_MAGIC) {
			DBGWRT_ERR((DBH,"*** LL - M31_Init: illegal magic=0x%04x\n", modIdMagic));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
		if ( (llHdl->modId != MOD_ID_M31) &&
			 (llHdl->modId != MOD_ID_M32) &&
			 (llHdl->modId != MOD_ID_M82) ) {
			DBGWRT_ERR((DBH,"*** LL - M31_Init: illegal id=%d\n", llHdl->modId));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
		DBGWRT_2((DBH," M%d module detected\n", llHdl->modId));
	}


    /*------------------------------+
    |  init hardware                |
    +------------------------------*/
	/* do nothing */

	return(ERR_SUCCESS);
}

/****************************** M31_Exit *************************************
 *
 *  Description:  De-initialize hardware and clean up memory
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdlP  	pointer to low-level driver handle
 *
 *  Output.....:  return    success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_Exit(
   LL_HANDLE    **llHdlP
)
{
    LL_HANDLE *llHdl = *llHdlP;
	int32 error = 0;

    DBGWRT_1((DBH, "LL - M31_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
	/* do nothing */

    /*------------------------------+
    |  clean up memory              |
    +------------------------------*/
	/* remove signal */
	if (llHdl->sigHdl)
		OSS_SigRemove(llHdl->osHdl, &llHdl->sigHdl);
	
	*llHdlP = NULL;		/* set low-level driver handle to NULL */ 
	error = Cleanup(llHdl,error);

	return(error);
}

/****************************** M31_Read *************************************
 *
 *  Description:  Reads the state of the current channel.
 *
 *                Bit 0 of valueP represents the state of the current channel.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *
 *  Output.....:  valueP   read value
 *                return   success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_Read(
    LL_HANDLE	*llHdl,
    int32		ch,
    int32		*valueP
)
{
	u_int16		data;

    DBGWRT_1((DBH, "LL - M31_Read: ch=%d\n",ch));

	/* read all channels */
	data = MREAD_D16(llHdl->ma, DATA_REG);

	/* extract one channel */
	*valueP = (int32)( (data >> ch) & 0x01 );
	
	return(ERR_SUCCESS);
}

/****************************** M31_Write ************************************
 *
 *  Description:  Write value to channel (unused)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *                value    value to write 
 *
 *  Output.....:  return   ERR_LL_ILL_FUNC
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_Write(
    LL_HANDLE	*llHdl,
    int32		ch,
    int32		value
)
{
    DBGWRT_1((DBH, "LL - M31_Write: ch=%d\n",ch));
	
	return(ERR_LL_ILL_FUNC);
}

/****************************** M31_SetStat **********************************
 *
 *  Description:  Set driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see dbg.h
 *                M_LL_CH_DIR          channel direction          M_CH_IN
 *				  M_LL_IRQ_COUNT	   interrupt counter	      0..max
 *                M_MK_IRQ_ENABLE	   irq disable/enable	      0..1
 *                M31_SIGSET		   set signal				  1..max
 *                M31_SIGCLR           clear signal				  -
 *                M31_HYS_MODE (M82)   hysteresis of curr chan    0..1
 *                -------------------  -------------------------  ----------
 *
 *                M31_SIGSET installs a user signal with the specified signal
 *                  number. The signal will be sent to the caller if an
 *                  interrupt is triggered (if any input level changes and the
 *                  interrupt is enabled).
 *
 *                M31_SIGCLR deinstalls the user signal.
 *
 *                M31_HYS_MODE sets the hysteresis mode of the current channel:
 *                  0 = Hysteresis Mode B; 5.5V..15.5V
 *                  1 = Hysteresis Mode A; 5.5V..9.5V
 *                  This SetStat code can only be used for M82 M-Modules but
 *                  not for M31/M32 M-Modules.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64     data or pointer to block data struct (M_SG_BLOCK)
 *                                  for block status codes
 *
 *  Output.....:  return     success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_SetStat(
    LL_HANDLE	*llHdl,
    int32		code,
    int32		ch,
    INT32_OR_64 value32_or_64
)
{
	int32 error = ERR_SUCCESS;
	int16 reg;
    int32       value = (int32)value32_or_64;
    /*INT32_OR_64 valueP = value32_or_64; */

    DBGWRT_1((DBH, "LL - M31_SetStat: ch=%d code=0x%04x value=%08p\n",
			  ch,code,value));

    switch(code) {
        /* -------- common setstat codes ----------- */
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            if( value != M_CH_IN )
                return( ERR_LL_ILL_DIR );
            break;
        /*--------------------------+
        |  set irq counter          |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
			llHdl->irqCount = value;
            break;
        /*--------------------------+
        |  enable interrupts        |
        +--------------------------*/
        case M_MK_IRQ_ENABLE:
			/* enable irq */
			if(value){
				/* save current states */
				llHdl->lastState = MREAD_D16(llHdl->ma, DATA_REG);	
				/* clear change flags */
				llHdl->changeFlags = 0x00;
				/* irq is enabled */
				llHdl->irqEnable = TRUE;
			}
			/* disable irq */
			else{
				/* irq is disabled */
				llHdl->irqEnable = FALSE;
			}
			/* say not supported because irq is always enabled */
			error = ERR_LL_UNK_CODE;	
            break;

		/* ----- module specific setstat codes ----- */
        /*--------------------------+
        |   set signal              |
        +--------------------------*/
        case M31_SIGSET:

			/* already defined ? */
			if (llHdl->sigHdl != NULL) {   
				DBGWRT_ERR((DBH,
					"*** LL - M31_SetStat(M31_SIGSET): signal already installed\n"));
				return(ERR_OSS_SIG_SET);
			}

			/* illegal signal code ? */
			if (value == 0)
				return(ERR_LL_ILL_PARAM);

			/* install signal */
			if ((error = (OSS_SigCreate(llHdl->osHdl, value, &llHdl->sigHdl))))
				return(error);
			break;
        /*--------------------------+
        |   clear signal            |
        +--------------------------*/
        case M31_SIGCLR:

			/* not defined ? */
			if (llHdl->sigHdl == NULL) {   
				DBGWRT_ERR((DBH,
					"*** LL - M31_SetStat(M31_SIGCLR): signal not installed\n"));
				return(ERR_OSS_SIG_CLR);
			}

			/* remove signal */
			if ((error = (OSS_SigRemove(llHdl->osHdl, &llHdl->sigHdl))))
				return(error);
			break;
        /*--------------------------+
        |  hysteresis mode          |
        +--------------------------*/
        case M31_HYS_MODE:
			/* M82 only */
			if( llHdl->modId == MOD_ID_M82 ){
				/* set hysteresis mode for current channel */
				reg = MREAD_D16(llHdl->ma, MODE_REG);
				if( value )
					reg |= 0x01 << ch;
				else
					reg &= ~(0x01 << ch);
				MWRITE_D16(llHdl->ma, MODE_REG, reg);
			}
			else {
	          error = ERR_LL_UNK_CODE;
			}
			break;
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/****************************** M31_GetStat **********************************
 *
 *  Description:  Get driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see dbg.h
 *                M_LL_CH_NUMBER       number of channels         16
 *                M_LL_CH_DIR          direction of curr chan     M_CH_IN
 *                M_LL_CH_LEN          length of curr chan [bits] 1..max
 *                M_LL_CH_TYP          description of curr chan   M_CH_BINARY
 *				  M_LL_IRQ_COUNT       interrupt counter          0..max
 *                M_LL_ID_CHECK        EEPROM is checked          0..1
 *                M_LL_ID_SIZE         EEPROM size [bytes]        128
 *                M_LL_BLK_ID_DATA     EEPROM raw data            -
 *                M_MK_BLK_REV_ID      ident function table ptr   -
 *                M31_SIGSET		   get signal				  1..max
 *                M31_CHANGE_FLAGS	   get change flags			  0x00..0xff
 *                M31_HYS_MODE (M82)   hysteresis of curr chan    0..1
 *                -------------------  -------------------------  ----------
 *
 *                M31_SIGSET gets the signal number of the installed user
 *                  signal. If no signal was installed it yields the value 0.
 *
 *                M31_CHANGE_FLAGS gets 16 flags which inform about level
 *                  changes of each channel if the interrupt is enabled.
 *                  Bits 15..0 of the bit mask (flags) correspond to channels
 *                  15..0. A flag set to 1 indicates that the level of the
 *                  belonging channel was changed from 0 to 1 or vice versa
 *                  (regardless how often). The flags are reset to 0 after this
 *                  GetStat call or when the interrupt is enabled (SetStat
 *                  code M_MK_IRQ_ENABLE).
 *
 *                M31_HYS_MODE gets the hysteresis mode of the current channel:
 *                  0 = Hysteresis Mode B; 5.5V..15.5V
 *                  1 = Hysteresis Mode A; 5.5V..9.5V
 *                  This GetStat code can only be used for M82 M-Modules but
 *                  not for M31/M32 M-Modules.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64P    pointer to block data struct (M_SG_BLOCK) 
 *                                  for block status codes
 *
 *  Output.....:  value32_or_64P    data pointer or ptr to block data struct (M_SG_BLOCK)
 *                                  for block status codes
 *                return            success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_GetStat(
    LL_HANDLE	*llHdl,
    int32		code,
    int32		ch,
    INT32_OR_64  *value32_or_64P
)
{
    int32       *valueP     = (int32*)value32_or_64P; /* pointer to 32 bit value */
    INT32_OR_64 *value64P     = value32_or_64P;         /* stores 32/64bit pointer */
	M_SG_BLOCK  *blk        = (M_SG_BLOCK*)value32_or_64P;

	int32 error = ERR_SUCCESS;
    int32 dummy;
	int16 data;
    
	DBGWRT_1((DBH, "LL - M31_GetStat: ch=%d code=0x%04x\n",
			  ch,code));

    switch(code)
    {
        /* -------- common getstat codes ----------- */
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  nr of channels           |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_IN;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 1;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_BINARY;
            break;
        /*--------------------------+
        |  ID PROM check enabled    |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = llHdl->idCheck;
            break;
        /*--------------------------+
        |   ID PROM size            |
        +--------------------------*/
        case M_LL_ID_SIZE:
            *valueP = MOD_ID_SIZE;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            *valueP = llHdl->irqCount;
            break;
        /*--------------------------+
        |   ID PROM data            |
        +--------------------------*/
        case M_LL_BLK_ID_DATA:
		{
			u_int8 n;
			u_int16 *dataP = (u_int16*)blk->data;

			if (blk->size < MOD_ID_SIZE)		/* check buf size */
				return(ERR_LL_USERBUF);

			for (n=0; n<MOD_ID_SIZE/2; n++)		/* read MOD_ID_SIZE/2 words */
				*dataP++ = (u_int16)m_read((U_INT32_OR_64)llHdl->ma, n);

			break;
		}
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *value64P = (INT32_OR_64)&llHdl->idFuncTbl;
           break;

		/* ----- module specific getstat codes ----- */
        /*--------------------------+
        |  signal code              |
        +--------------------------*/
        case M31_SIGSET:
			/* return signal */
			if (llHdl->sigHdl == NULL)
				*valueP = 0x00;
			else
				OSS_SigInfo(llHdl->osHdl, llHdl->sigHdl, valueP, &dummy);
			break;
        /*--------------------------+
        |  change flags             |
        +--------------------------*/
        case M31_CHANGE_FLAGS:
			if(llHdl->irqEnable){
				*valueP = (int32)llHdl->changeFlags;
				llHdl->changeFlags = 0x00;
			}
			else{
				error = ERR_LL_DEV_NOTRDY;
			}
			break;
        /*--------------------------+
        |  hysteresis mode          |
        +--------------------------*/
        case M31_HYS_MODE:
			/* M82 only */
			if( llHdl->modId == MOD_ID_M82 ){
				/* get hysteresis mode for current channel */
				data = MREAD_D16(llHdl->ma, MODE_REG);
				*valueP = (int32)( (data >> ch) & 0x01 );
			}
			else {
	          error = ERR_LL_UNK_CODE;
			}
			break;
       /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/******************************* M31_BlockRead *******************************
 *
 *  Description:  Read the state of all 16 channels
 *
 *                Bits 15..0 of the first two bytes of the data buffer (buf)
 *                correspond to channels 15..0.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size (minimum: two bytes)
 *
 *  Output.....:  nbrRdBytesP  number of read bytes
 *                return       success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_BlockRead(
     LL_HANDLE	*llHdl,
     int32		ch,
     void		*buf,
     int32		size,
     int32		*nbrRdBytesP
)
{
	DBGWRT_1((DBH, "LL - M31_BlockRead: ch=%d, size=%d\n",ch,size));

	/* return nr of read bytes */
	*nbrRdBytesP = 0;

	if (size < 2)
		return ERR_LL_USERBUF;

	*((u_int16*)buf) = MREAD_D16(llHdl->ma, DATA_REG);

	*nbrRdBytesP = 2;

	return(ERR_SUCCESS);
}

/****************************** M31_BlockWrite *******************************
 *
 *  Description:  Write data block to device (unused)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *
 *  Output.....:  nbrWrBytesP  number of written bytes
 *                return       ERR_LL_ILL_FUNC
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_BlockWrite(
     LL_HANDLE	*llHdl,
     int32		ch,
     void		*buf,
     int32		size,
     int32		*nbrWrBytesP
)
{
    DBGWRT_1((DBH, "LL - M31_BlockWrite: ch=%d, size=%d\n",ch,size));

	/* return nr of written bytes */
	*nbrWrBytesP = 0;

	return(ERR_LL_ILL_FUNC);
}


/****************************** M31_Irq *************************************
 *
 *  Description:  Interrupt service routine
 *
 *                The interrupt is triggered when any input level changes.
 *                For each channel a level change will be stored in a flag.
 *                If a user signal is installed, the signal will be sent.
 *
 *                If the driver can detect the interrupt cause it returns
 *                LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *
 *  Output.....:  return   LL_IRQ_UNKNOWN
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_Irq(
   LL_HANDLE *llHdl
)
{
	u_int16 currState;
    IDBGWRT_1((DBH, "LL - M31_Irq:\n"));

	/* get current states */	
	currState = MREAD_D16(llHdl->ma, DATA_REG);

	/* save level changes */
	llHdl->changeFlags |= llHdl->lastState ^ currState;
	llHdl->lastState = currState;

	/* signal installed? */
	if(llHdl->sigHdl){
		/* send signal */
		OSS_SigSend(llHdl->osHdl, llHdl->sigHdl);
	}

	/* clear interrupt */
	MREAD_D16(llHdl->ma, IRQCRL_REG);

	/* not my interrupt */
	return LL_IRQ_UNKNOWN;
}

/****************************** M31_Info ************************************
 *
 *  Description:  Get driver info
 *
 *                The following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                LL_INFO_HW_CHARACTER      hardware characteristics
 *                LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *                LL_INFO_ADDRSPACE         address space type
 *                LL_INFO_IRQ               interrupt required
 *                LL_INFO_LOCKMODE 			process locking required (LL_LOCK_xxx).
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  infoType	   info code
 *                ...          argument(s)
 *
 *  Output.....:  return       success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
static int32 M31_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
		/*-------------------------------+
        |  hardware characteristics      |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);

			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD16;
			break;
	    }
		/*-------------------------------+
        |  nr of required address spaces |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
	    }
		/*-------------------------------+
        |   address space type           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);

			if (addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = 0x100;
			}

			break;
	    }
		/*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = TRUE;
			break;
	    }
        /*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
        {
            u_int32 *lockModeP = va_arg(argptr, u_int32*);

            *lockModeP = LL_LOCK_CALL;
            break;
        }
		/*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  return  pointer to ident string
 *
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )	/* nodoc */
{
    return( (char*)IdentString );
}

/********************************* Cleanup **********************************
 *
 *  Description: Close all handles, free memory and return error code
 *
 *		         NOTE: The low-level handle is invalid after calling this
 *                     function.
 *			   
 *---------------------------------------------------------------------------
 *  Input......: llHdl		low-level handle
 *               retCode    return value
 *
 *  Output.....: return	    retCode
 *
 *  Globals....: -
 ****************************************************************************/
static int32 Cleanup(	/* nodoc */
   LL_HANDLE    *llHdl,
   int32        retCode
)
{
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
	/* clean up desc */
	if (llHdl->descHdl)
		DESC_Exit(&llHdl->descHdl);

	/* clean up debug */
	DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
	return(retCode);
}




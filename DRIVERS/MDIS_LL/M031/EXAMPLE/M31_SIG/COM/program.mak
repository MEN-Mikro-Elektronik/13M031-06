#**************************  M a k e f i l e ********************************
#  
#         Author: ds
#          $Date: 2004/05/03 14:49:44 $
#      $Revision: 1.2 $
#  
#    Description: Makefile definitions for the m31_sig example program
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2004/05/03 14:49:44  cs
#   removed switch MAK_OPTIM
#
#   Revision 1.1  2001/08/22 17:15:38  Schmidt
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2001 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

MAK_NAME=m31_sig

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX) \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/m31_drv.h \
	 $(MEN_INC_DIR)/men_typs.h \
         $(MEN_INC_DIR)/mdis_api.h \
         $(MEN_INC_DIR)/usr_oss.h

MAK_INP1=m31_sig$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

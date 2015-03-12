#**************************  M a k e f i l e ********************************
#  
#         Author: ds
#          $Date: 2004/05/03 14:49:42 $
#      $Revision: 1.3 $
#  
#    Description: Makefile definitions for the m31_simp example program
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.3  2004/05/03 14:49:42  cs
#   added include mdis_err.h
#   removed switch MAK_OPTIM
#
#   Revision 1.2  2001/08/22 17:15:34  Schmidt
#   m31_drv.h added
#
#   Revision 1.1  1998/07/21 16:37:25  Schmidt
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

MAK_NAME=m31_simp

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX) \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/m31_drv.h \
	 $(MEN_INC_DIR)/men_typs.h \
         $(MEN_INC_DIR)/mdis_api.h \
         $(MEN_INC_DIR)/mdis_err.h \
         $(MEN_INC_DIR)/usr_oss.h

MAK_INP1=m31_simp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)


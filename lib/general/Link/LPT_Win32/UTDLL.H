/***************************************************************************\
*  Copyright (C) 1996 Dallas Semiconductor Corporation.                     *
*  All rights Reserved. Printed in U.S.A.                                   *
*  This software is protected by copyright laws of                          *
*  the United States and of foreign countries.                              *
*  This material may also be protected by patent laws of the United States  *
*  and of foreign countries.                                                *
*  This software is furnished under a license agreement and/or a            *
*  nondisclosure agreement and may only be used or copied in accordance     *
*  with the terms of those agreements.                                      *
*  The mere transfer of this software does not imply any licenses           *
*  of trade secrets, proprietary technology, copyrights, patents,           *
*  trademarks, maskwork rights, or any other form of intellectual           *
*  property whatsoever. Dallas Semiconductor retains all ownership rights.  *
*                                                                           *                 
*  Module: UTDll.h                                                          *
*  Compiler: MSVC 4.0                                                       *
*  Programmer: CW                                                           *
\***************************************************************************/

#ifndef APIENTRY /* to prevent problems w/ 16 bit compilers  */
#define APIENTRY /* and the function prototypes.             */
#endif

//==============================================================================
// Defines for case statement in 16bit UTProc
//==============================================================================
#define MYSETUP        1
#define MYFIRST        2
#define MYNEXT         3
#define MYACCESS       4
#define MYDATABYTE     5
#define MYGNDTEST      6
#define MYROMDATA      7
#define MYKEYOPEN      8
#define MYKEYCLOSE     9
#define MYCOPYROMDATA  10
#define MYCOPY2ROMDATA 11
#define MYDOWCHECK     12
#define MYOVERDRIVEOFF 13
#define MYOVERDRIVEON  14
#define MYDATABIT      15

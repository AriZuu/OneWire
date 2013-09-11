//  (CCW)
//  sacwd32.h                                       { Ver 1.00 5/96 }
//
//  Copyright (C) 1996 Dallas Semiconductor Corporation. 
//  All rights Reserved. Printed in U.S.A.
//  This software is protected by copyright laws of
//  the United States and of foreign countries.
//  This material may also be protected by patent laws of the United States 
//  and of foreign countries.
//  This software is furnished under a license agreement and/or a
//  nondisclosure agreement and may only be used or copied in accordance
//  with the terms of those agreements.
//  The mere transfer of this software does not imply any licenses
//  of trade secrets, proprietary technology, copyrights, patents,
//  trademarks, maskwork rights, or any other form of intellectual
//  property whatsoever. Dallas Semiconductor retains all ownership rights.
//
typedef unsigned char  uchar;
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef struct
{
   uchar   ld;            /* last discrepancy */
   uchar   lastone;       /* true when last part is seen */
   uchar   unit;          /* parallel port unit, 1, 2, or 3 */
   uchar   accflg;        /* true when access has be executed */
   ushort  count;         /* timing for probes of parallel port */
   ushort  port;          /* parallel port base address */
   ushort  setup_ok;      /* has to be equal to MAGIC */
   uchar   romdta[8];
} 
sa_struct;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


#ifdef __cplusplus
extern "C" {
#endif

uchar  APIENTRY OverdriveOn     (void);
void   APIENTRY OverdriveOff    (void);
uchar  APIENTRY setup           (uchar, sa_struct *gb);
uchar  APIENTRY dowcheck        (void);
uchar  APIENTRY keyopen         (void);
uchar  APIENTRY keyclose        (void);
uchar  APIENTRY first           (sa_struct *gb);
uchar  APIENTRY next            (sa_struct *gb);
uchar  APIENTRY gndtest         (sa_struct *gb);
uchar  * APIENTRY romdata       (sa_struct *gb);
uchar  APIENTRY databyte        (uchar, sa_struct *gb);                                           
uchar  APIENTRY databit         (uchar, sa_struct *gb);
void   APIENTRY copyromdata     (ushort *, sa_struct *);
void   APIENTRY copy2romdata    (ushort *, sa_struct *);
uchar  APIENTRY access          (sa_struct *gb);
unsigned long APIENTRY GetAccessAPIVersion(void);

#ifdef __cplusplus
}
#endif


#define DOWRESET      1
#define DOWBIT        2
#define DOWBYTE       3
#define DOWTOGGLEOD   4
#define DOWCHECKBSY   5
#define DOWTOGGLEPASS 6
#define CHECK_OVERDRIVE 0x22

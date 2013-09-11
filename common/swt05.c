//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
//
//  swt05.c - Turns DS2405 on/off and reads if it is on/off
//  version 2.00
//

// Include Files
#include "ownet.h"

//--------------------------------------------------------------------------
// SUBROUTINE - SetSwitch05
//
// This routine turns the device on or off.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'SwNum'    - The serial number of the swith that is to be turned on or off
// 'seton'    - 'TRUE' then it is turned on if off and left alone if already on.
//              'FALSE' turns it off if already on
//
// Returns: TRUE(1):    If set is successful
//          FALSE(0):   If set is not successful
//
int SetSwitch05(int portnum, uchar *SwNum, int seton)
{
   int compare;

   owSerialNum(portnum,&SwNum[0], FALSE);

   compare = owVerify(portnum,TRUE);

   if((compare && seton) ||
      (!compare && !seton))
      return TRUE;
   else
      if(owAccess(portnum))
      {
         compare = owVerify(portnum,TRUE);

         if((compare && seton) ||
            (!compare && !seton))
            return TRUE;
         else
            return FALSE;
      }
      else
         return FALSE;
}

//--------------------------------------------------------------------------
// SUBROUTINE - ReadSwitch05
//
// This routine reads if the DS2405 device is on or off.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                OpenCOM to indicate the port number.
// 'SerialNum'  - The Serial number of the DS2405 that info is requested on
// 'level'      - This is 1 for high and 0 for low level
//
// Returns: TRUE(1):    If device is found and active
//          FALSE(0):   If device is not found and not active or not there
//
int ReadSwitch05(int portnum, uchar *SerialNum, int *level)
{

   owSerialNum(portnum,&SerialNum[0], FALSE);

   if(owVerify(portnum,FALSE))
   {

      if(owTouchByte(portnum,0xFF) == 0xFF)
         *level = 1;
      else
         *level = 0;

      if(owVerify(portnum,TRUE))
         return TRUE;
      else
         return FALSE;
   }
   else
      return FALSE;
}

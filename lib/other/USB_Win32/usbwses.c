//---------------------------------------------------------------------------
// Copyright (C) 2003 Dallas Semiconductor Corporation, All Rights Reserved.
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
//---------------------------------------------------------------------------
//
//  usbwses.c - Acquire and release a Session on the 1-Wire Net using the
//              USB interface DS2490.
//
//  Version: 3.00
//
//  History:
//

#include "ownet.h"
#include "ds2490.h"

// handles to USB ports
HANDLE usbhnd[MAX_PORTNUM];
static SMALLINT usbhnd_init = 0;

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a USB port and a DS2490 based
// adapter.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format '\\.\DS2490-X' where X is the port number.
//
// Returns: TRUE - success, COM port opened
//
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   if(!usbhnd_init)
   {
      int i;
      for(i=0; i<MAX_PORTNUM; i++)
         usbhnd[0] = 0;
      usbhnd_init = 1;
   }
   OWASSERT( portnum<MAX_PORTNUM && portnum>=0 && !usbhnd[portnum],
             OWERROR_PORTNUM_ERROR, FALSE );

   // get a handle to the device
   usbhnd[portnum] = CreateFile(port_zstr,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

   if (usbhnd[portnum] != INVALID_HANDLE_VALUE)
   {
      // verify adapter is working
      if (!AdapterRecover(portnum))
      {
         CloseHandle(usbhnd[portnum]);
         return FALSE;
      }
      // reset the 1-Wire
      owTouchReset(portnum);
   }
   else
   {
      // could not get resouse
      OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
      return FALSE;
   }

   return TRUE;
}

//---------------------------------------------------------------------------
// Attempt to acquire the specified 1-Wire net.
//
// 'port_zstr'     - zero terminated port name.
//
// Returns: port number or -1 if not successful in setting up the port.
//
int owAcquireEx(char *port_zstr)
{
   int portnum = 0;

   if(!usbhnd_init)
   {
      int i;
      for(i=0; i<MAX_PORTNUM; i++)
         usbhnd[0] = 0;
      usbhnd_init = 1;
   }

   // check to find first available handle slot
   for(portnum = 0; portnum<MAX_PORTNUM; portnum++)
   {
      if(!usbhnd[portnum])
         break;
   }
   OWASSERT( portnum<MAX_PORTNUM, OWERROR_PORTNUM_ERROR, -1 );

   // get a handle to the device
   usbhnd[portnum] = CreateFile(port_zstr,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

   if (usbhnd[portnum] != INVALID_HANDLE_VALUE)
   {
      // verify adapter is working
      if (!AdapterRecover(portnum))
      {
         CloseHandle(usbhnd[portnum]);
         return -1;
      }
      // reset the 1-Wire
      owTouchReset(portnum);
   }
   else
   {
      // could not get resouse
      OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------------------
// Release the port previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
void owRelease(int portnum)
{
   CloseHandle(usbhnd[portnum]);
}


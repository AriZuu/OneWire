//---------------------------------------------------------------------------
// Copyright (C) 1999 Dallas Semiconductor Corporation, All Rights Reserved.
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
//  DS520SES.C - Acquire and release a Session for general 1-Wire Net
//              library
//
//  Version: 2.00
//           1.03 -> 2.00  Changed 'MLan' to 'ow'. Added support for
//                         multiple ports.
//

#include "ownet.h"
#include <ds8xc520.h>

// Port for 1Wire communication.  Defaults here, but defined in Makefile also
// WARNING: See appnote ??? to select an appropriate port pin and
//          allow for appropriate protection of the pin.
// If this is changed, also change in link file
#ifndef OW_PORT
#define OW_PORT INT2
#endif

// defined in link file
extern void usDelay(int);

// local function prototypes
SMALLINT owAcquire(int,char *);
void     owRelease(int);

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'port_zstr'  - zero terminated port name.
//
// Returns: TRUE - success, port opened
//
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   port_zstr = 0;
   portnum = 0;

   OW_PORT = 1; // drive bus high.
   usDelay(500); // give time for line to settle

   // checks to make sure the line is idling high.
   return (OW_PORT==1?TRUE:FALSE);
}

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a com port and a DS2480 based
// adapter.
//
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format COMX where X is the port number.
//
// Returns: The portnum or -1 if the port wasn't acquired.
//
int owAcquireEx(char *port_zstr)
{
   int portnum = 0;
   port_zstr = 0;

   OW_PORT = 1; // drive bus high.
   usDelay(500); // give time for line to settle

   if(OW_PORT != 1)
   {
      OWERROR(OWERROR_OPENCOM_FAILED);
      return -1;
   }

   // checks to make sure the line is idling high.
   return portnum;
}

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
void owRelease(int portnum)
{
   portnum = 0;
}



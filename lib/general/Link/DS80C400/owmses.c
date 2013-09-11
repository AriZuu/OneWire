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
//  OWMSES.C - Acquire and release a Session for the 1-Wire Master (OWM) on the
//              DS80C400 microcontroller.
//
//
//  Version: 1.00
//
//  History:
//

#include <reg400.h>
#include "ownet.h"
#include "owm.h"          // 1-Wire Master defines

// local function prototypes
int      owAcquireEx(char *);

extern SMALLINT USpeed; // current 1-Wire Net communication speed
extern SMALLINT ULevel; // current 1-Wire Net level

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net
//
// 'port_zstr'  - zero terminated port name.
//
// Returns: valid handle, or -1 if an error occurred
//
int owAcquireEx(char *port_zstr)
{
   port_zstr = 0;

   // Set up clock divisor
   OWMAD = OWM_CLOCK_DIVISOR;
   OWMDR = (OWM_DIVISOR | OWM_CLK_EN_MASK);

   // Set 1-Wire control register to known state
   OWMAD = OWM_CONTROL;
   OWMDR = 0;

   // Set 1-Wire command register to known state
   OWMAD = OWM_COMMAND;
   OWMDR = 0;

   ULevel = MODE_NORMAL;
   USpeed = MODE_NORMAL;

   return 0;
}

//--------------------------------------------------------------------------
-
// Attempt to acquire a 1-Wire net using a com port and a DS2480 based
// adapter.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                OpenCOM to indicate the port number.
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format COMX where X is the port number.
//
// Returns: TRUE - success, COM port opened
//
// exportable functions defined in ownetu.c
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   port_zstr = 0;

   // Set up clock divisor
   OWMAD = OWM_CLOCK_DIVISOR;
   OWMDR = (OWM_DIVISOR | OWM_CLK_EN_MASK);

   // Set 1-Wire control register to known state
   OWMAD = OWM_CONTROL;
   OWMDR = 0;

   // Set 1-Wire command register to known state
   OWMAD = OWM_COMMAND;
   OWMDR = 0;

   ULevel = MODE_NORMAL;
   USpeed = MODE_NORMAL;

   return TRUE;
}

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - valid handle, returned by earlier call to owAcquire()
//
void owRelease(int portnum)
{
   portnum = 0;

   // Turn off clock
   OWMAD = OWM_CLOCK_DIVISOR;
   OWMDR &= ~(OWM_CLK_EN_MASK);
}


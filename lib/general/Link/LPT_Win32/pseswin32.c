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
//---------------------------------------------------------------------------
//
//  w32pses.C - Acquire and release a Session for general 1-Wire Net
//              library for parallel port adapters DS1410E/DS1410D
//
//  Version: 2.00
//

#include "ownet.h"
#include <windows.h>
#include "sacwd32.h"

// local function prototypes
SMALLINT  owAcquire(int,char *);
void owRelease(int);

// external prototypes
extern SMALLINT owTouchReset(int);
extern SMALLINT owWriteByte(int,int sendbyte);
extern SMALLINT owReadByte(int);
extern SMALLINT owSpeed(int,int);

// globals
char portname[MAX_PORTNUM][32];
int  PortNum[MAX_PORTNUM];
SMALLINT PortNum_init = 0;
extern sa_struct SauthGB[MAX_PORTNUM];

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'port_zstr'  - zero terminated port name.
// 'return_msg' - zero terminated return message.
//
// Returns: TRUE - success, port opened
//
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   uchar phys_port;

   if(!PortNum_init)
   {
      int i;
      for(i=0; i<MAX_PORTNUM; i++)
         PortNum[i] = 0;
      PortNum_init = 1;
   }

   OWASSERT( portnum<MAX_PORTNUM && portnum>=0 && !PortNum[portnum],
             OWERROR_PORTNUM_ERROR, FALSE );

   // convert the string in port_zstr to be the port number
   phys_port = atoi(port_zstr);
   if (phys_port > 3)
      return FALSE;

   PortNum[portnum] = phys_port;

   // get permission from sauth
   if (dowcheck())
   {
      setup(phys_port,&SauthGB[phys_port]);
      if (keyopen())
      {
         // looks like we have an adapter
         if (owTouchReset(portnum))
         {
            owWriteByte(portnum,0xF0);// Search
            if (owReadByte(portnum) != 0xFF)
            {
               sprintf(portname[portnum],"DS1410E/DS1410D on port %d",phys_port);
               return TRUE;
            }
         }

         keyclose();
      }
   }

   PortNum[portnum] = 0;
   return FALSE;
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
   int portnum;

   if(!PortNum_init)
   {
      int i;
      for(i=0; i<MAX_PORTNUM; i++)
         PortNum[i] = 0;
      PortNum_init = 1;
   }

   // check to find first available handle slot
   for(portnum = 0; portnum<MAX_PORTNUM; portnum++)
   {
      if(!PortNum[portnum])
         break;
   }
   OWASSERT( portnum<MAX_PORTNUM, OWERROR_PORTNUM_ERROR, -1 );

   if(owAcquire(portnum, port_zstr))
      return portnum;
   else
      return -1;
}

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'return_msg' - zero terminated return message.
//
void owRelease(int portnum)
{
   // release for sauth
   keyclose();

   PortNum[portnum] = 0;
}



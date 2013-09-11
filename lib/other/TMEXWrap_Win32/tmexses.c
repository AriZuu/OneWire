//---------------------------------------------------------------------------
// Copyright (C) 2001 Dallas Semiconductor Corporation, All Rights Reserved.
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
//  owsestmx.c - Acquire and release a Session on the 1-Wire Net using TMEX.
//               (Requires TMEX 3.11 or newer)
//
//  Version: 2.01
//
//  History: 1.03 -> 2.00  Changed 'MLan' to 'ow'. Added support for
//                         multiple ports.
//           2.00 -> 2.01  Added support for owError library.
//

#include <stdio.h>
#include <windows.h>
#include "ownet.h"

// external function prototypes
extern long  far pascal TMExtendedStartSession(short, short, void far *);
extern short far pascal TMEndSession(long);
extern short far pascal TMClose(long);
extern short far pascal TMSetup(long);
extern short far pascal TMReadDefaultPort(short far *, short far *);

short PortNum=1,PortType=2;
long  SessionHandle[MAX_PORTNUM];
SMALLINT handle_init = FALSE;

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a com port and a DS2480 based
// adapter.
//
// 'port_zstr'     - zero terminated port name.  For this platform
//                   use format {port number, port type}.
//
// Returns: port number and -1 if not successful in setting up the port.
//
int owAcquireEx(char *port_zstr)
{
   int portnum;
   int string_counter, counter, i;
   char portnum_str[5];
   char porttype_str[5];
   void *tmex_options = NULL;

   if(!handle_init)
   {
      for(i=0; i<MAX_PORTNUM; i++)
         SessionHandle[i] = 0;
      handle_init = TRUE;
   }

   // check to find first available handle slot
   for(portnum = 0; portnum<MAX_PORTNUM; portnum++)
   {
      if(!SessionHandle[portnum])
         break;
   }
   OWASSERT( portnum<MAX_PORTNUM, OWERROR_PORTNUM_ERROR, -1 );

   // convert the string in port_zstr to be the port number and port type
   if(port_zstr)
   {
      string_counter = 1;
      counter = 0;
      do
      {
         portnum_str[counter] = port_zstr[string_counter];

         counter++;
         string_counter++;
      }
      while(port_zstr[string_counter] != ',');

      portnum_str[counter] = '\0';

      string_counter++;
      counter = 0;

      do
      {
         porttype_str[counter] = port_zstr[string_counter];

         counter++;
         string_counter++;
      }
      while(port_zstr[string_counter] != '}');

      porttype_str[counter] = '\0';

      PortNum = atoi(portnum_str);
      PortType = atoi(porttype_str);
   }

   // open a session
   SessionHandle[portnum] = TMExtendedStartSession(PortNum,PortType,tmex_options);

   // check the session handle
   if (SessionHandle[portnum] <= 0)
   {
      OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
      return -1;
   }

   // setup the port
   if (TMSetup(SessionHandle[portnum]) != 1)
   {
      TMClose(SessionHandle[portnum]);
      TMEndSession(SessionHandle[portnum]);
      OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a com port and a DS2480 based
// adapter.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format COMX where X is the port number.
//
// Returns: TRUE - success, COM port opened
//
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   int i;

   if(!handle_init)
   {
      for(i=0; i<MAX_PORTNUM; i++)
         SessionHandle[i] = 0;
      handle_init = TRUE;
   }

   OWASSERT( portnum<MAX_PORTNUM && portnum>=0 && !SessionHandle[portnum],
             OWERROR_PORTNUM_ERROR, FALSE );

   // read the default PortNum and PortType
   TMReadDefaultPort(&PortNum,&PortType);

   // convert the string in port_zstr to be the port number
   PortNum = atoi(port_zstr);

   // open a session
   SessionHandle[portnum] = TMExtendedStartSession(PortNum,PortType,NULL);

   // check the session handle
   if (SessionHandle[portnum] <= 0)
   {
      OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
      return FALSE;
   }

   // setup the port
   if (TMSetup(SessionHandle[portnum]) != 1)
   {
      TMClose(SessionHandle[portnum]);
      TMEndSession(SessionHandle[portnum]);
      OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
      return FALSE;
   }

   return TRUE;
}

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
void owRelease(int portnum)
{
   TMClose(SessionHandle[portnum]);
   TMEndSession(SessionHandle[portnum]);
   SessionHandle[portnum] = 0;
}


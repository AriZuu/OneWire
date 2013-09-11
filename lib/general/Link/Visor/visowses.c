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
//  VisowSes.C - Acquire and release a Session on the 1-Wire Net.

#include <SystemMgr.h>
#include "SauthPalm.h"

// external function prototypes
extern SMALLINT owSpeed(int,SMALLINT);
extern void output_status(int, char *);
extern void msDelay(int);

// local function prototypes
int RetPort(int);

// keep port name for later message when closing
char portname[MAX_PORTNUM][128];
int CntAcqRel=0;
int port[MAX_PORTNUM];

// Code to save batteries
static  void (*g_lpTrapLCDSleep)();
static void HandleLCDSleepTrap()
{
  AdapterPowerManagement(0);
  
  if(g_lpTrapLCDSleep)
    g_lpTrapLCDSleep();
}
// Code to save batteries


//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a com port and a DS2480 based
// adapter.  
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                indicate the port number.
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format COMX where X is the port number.
// 'return_msg' - zero terminated return message. 
//
// Returns: TRUE - success, COM port opened
//
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   int  input;

   if(!CntAcqRel)
   {
   		g_lpTrapLCDSleep = SysGetTrapAddress(sysTrapHwrDisplaySleep);
   		SysSetTrapAddress(sysTrapHwrDisplaySleep, HandleLCDSleepTrap);
   		
   		AdapterPowerManagement(1);   
   		msDelay(50);		
   }
         
   if(port_zstr[0] == '0')
   {	
   		port[portnum] = VISOR_INT;
   		input = VISOR_INT;
   }
   else if(port_zstr[0] == '1')
   {
   		port[portnum] = VISOR_EX;
   		input = VISOR_EX;
   }
   		
   iBSetup(input);
   iBKeyOpen();

   if(owSpeed(portnum,MODE_NORMAL) != MODE_NORMAL)
   		return FALSE;
   
   CntAcqRel++;
   	
   return TRUE;
}

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                indicate the port number.
// 'return_msg' - zero terminated return message. 
//
void owRelease(int portnum)
{
    
   if(CntAcqRel)
   {
   
   		iBSetup(port[portnum]);
   		owSpeed(port[portnum],MODE_NORMAL);

   		CntAcqRel--;
   		
   		// close the communications port
   		iBKeyClose();  
   		
   		if(!CntAcqRel)
   		{
   			if(g_lpTrapLCDSleep)
	  			SysSetTrapAddress(sysTrapHwrDisplaySleep, g_lpTrapLCDSleep);
	  			
	  		AdapterPowerManagement(0);
	 	  		
	  	} 		   		
   }

}


//------------------------------------------------------------------------
// Used to get the correct port for the Visor
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                indicate the port number.
int RetPort(int portnum)
{
	return port[portnum];
}
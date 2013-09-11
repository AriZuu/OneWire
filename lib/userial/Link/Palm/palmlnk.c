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
//  PalmLnk.C - COM functions required by OWLLU.C, OWTRNU, OWNETU.C and
//              OWSESU.C for the One-Wire to communicate with the DS2480 based 
//              Universal Serial Adapter 'U'.  Fill in the platform specific code.
//

#include "ownet.h"
#include <PalmTypes.h>
#include <SystemMgr.h>
#include <SerialMgrOld.h>
#include <TimeMgr.h>

// Global constatnts
#define timeout 		5
#define flush_timeout 	0
#define PARMSET_9600	0x00
#define PARMSET_19200 	0x02
#define PARMSET_57600	0x04
#define PARMSET_115200	0x06

// Global reference number
unsigned short ref[MAX_PORTNUM];
unsigned short tempref;


// exportable functions required for 
// MLANLL.C, MLANTRNU, or MLANNETU.C 
void FlushCOM(int);
SMALLINT WriteCOM(int, int, uchar *);
int ReadCOM(int, int, uchar *);
void BreakCOM(int);
void msDelay(int);
void SetBaudCOM(int, uchar);
SMALLINT  OpenCOM(int, char *);
void CloseCOM(int);


//---------------------------------------------------------------------------
//  Description:
//     flush the rx and tx buffers
//
void FlushCOM(int portnum)
{
   SerSendFlush(ref[portnum]);	
   SerReceiveFlush(ref[portnum], flush_timeout);
}


//--------------------------------------------------------------------------
// Write an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set.
//
// Returns 1 for success and 0 for failure
//
SMALLINT WriteCOM(int portnum, int outlen, uchar *outbuf)
{
	int returnlen;
	Err errp;
	
   	returnlen = SerSend(ref[portnum], outbuf, outlen, &errp);
   		
   	if( (returnlen == outlen) && (errp == 0) )
   		return 1;
   	else
   		return 0;
}


//--------------------------------------------------------------------------
// Read an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set.
//
// Returns number of characters read
//
int ReadCOM(int portnum, int inlen, uchar *inbuf)
{
	ulong bytes;
	Err errp;
	
	if(!SerReceiveWait(ref[portnum], inlen, timeout))
	{
		bytes = SerReceive(ref[portnum], inbuf, inlen, timeout, &errp);
		if(!errp)
			return bytes;
	}
			
   	return 0;
}


//--------------------------------------------------------------------------
//  Description:
//     Send a break on the com port for at least 2 ms
// 
void BreakCOM(int portnum)
{
	SerSettingsType comset;
	ulong tempbaud;	
	uchar s[1];
	
	s[0] = 0x00;
	
	if(!SerGetSettings(ref[portnum], &comset))
	{
		tempbaud = comset.baudRate;
		comset.baudRate = 2400;
		SerSetSettings(ref[portnum], &comset);
	}
	
	if(WriteCOM(portnum, 1, s))
		ReadCOM(portnum, 1, s);
		
	comset.baudRate = tempbaud;
	SerSetSettings(ref[portnum], &comset);	
}


//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
// 
void msDelay(int len)
{
	long sysdelay;
	
	sysdelay = (len + 5)/10;
	
	if(sysdelay == 0)
		sysdelay++;
		
	SysTaskDelay(sysdelay);
}


//--------------------------------------------------------------------------
// Set the baud rate on the com port.  The possible baud rates for
// 'new_baud' are:
//
// PARMSET_9600     0x00
// PARMSET_19200    0x02
// PARMSET_57600    0x04
// PARMSET_115200   0x06
// 
void SetBaudCOM(int portnum, uchar new_baud)
{
	SerSettingsType comset;
	
	if(!SerGetSettings(ref[portnum], &comset))
	{
		switch(new_baud)
		{
			case PARMSET_9600:
				comset.baudRate = 9600;
				break;
				
			case PARMSET_19200:
				comset.baudRate = 19200;
				break;
				
			case PARMSET_57600:
				comset.baudRate = 57600;
				break;
				
			case PARMSET_115200:
				comset.baudRate = 115200;
				break;
				
			default:
				break;
		}
		
		SerSetSettings(ref[portnum], &comset);
	}
	
}


//---------------------------------------------------------------------------
// Attempt to open a com port.  
// Set the starting baud rate to 9600.
//
// 'port_zstr' - zero terminate port name.  Format is platform
//               dependent.
//
// Returns: TRUE - success, COM port opened
//
SMALLINT OpenCOM(int portnum, char *port_zstr)
{
    int             port = 0;
    int             ret = 0;
    SerSettingsType setcom;
    
    if(!SysLibFind("Serial Library", &ref[portnum]))
    {
    	if(port_zstr[0] == '0')
    		port = 0;
    	if(port_zstr[0] == '1')
	   		port = 1;
    	if(port_zstr[0] == '2')
    		port = 2;
    	if(port_zstr[0] == '3')
    		port = 3;
    		    
    	if(!SerOpen(ref[portnum], port, 9600))
    	{
       		ret = TRUE;
     
       		setcom.baudRate = 9600;
       		setcom.flags = serSettingsFlagStopBits1 | serSettingsFlagBitsPerChar8 |
       		    		   serSettingsFlagRTSAutoM;
       		setcom.ctsTimeout = timeout;
       
       		if(SerSetSettings(ref[portnum], &setcom))
          		ret = FALSE;
    	}
    	
    }
    else
    	ret = FALSE;
    	
    return ret;
}


//---------------------------------------------------------------------------
// Closes the connection to the port.
//
void CloseCOM(int portnum)
{
    SerClose(ref[portnum]);
}


//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   return (TimGetTicks()*10);
}

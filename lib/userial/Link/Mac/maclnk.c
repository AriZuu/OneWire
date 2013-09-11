//---------------------------------------------------------------------------
// Copyright (C) 1998 Dallas Semiconductor Corporation, All Rights Reserved.
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
//  TODO.C - COM functions required by MLANLLU.C, MLANTRNU, MLANNETU.C and
//           MLanFile.C for MLANU to communicate with the DS2480 based
//           Universal Serial Adapter 'U'.  Fill in the platform specific code.
//
//  Version: 1.00
//
//

//--------------------------------------------------------------------------
// Platform specific code for Macintosh.
//
// Copyright (C) 1998 Harry Whitfield and University of Newcastle upon Tyne,
// All Rights Reserved.
//--------------------------------------------------------------------------
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
// IN NO EVENT SHALL THE UNIVERSITY OF NEWCASTLE UPON TYNE OR HARRY WHITFIELD
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//---------------------------------------------------------------------------
//
//  MacLNK.C - COM functions required by MLANLLU.C, MLANTRNU.C, MLANNETU.C and
//             MLanFile.C for MLANU to communicate with the DS2480 based
//             Universal Serial Adapter 'U'.  Platform specific code.
//
//  Version: 2.01
//
//  History: 1.01 -> 1.02  Changed to generic OpenCOM/CloseCOM for easier
//                           use with other platforms.
//           1.02 -> 1.03  Added stub of function msGettick. Still needs
//                           platform specific code.
//           1.03 -> 2.00  Function prototpyes to support for multiple ports.
//                         Include "ownet.h". Reorder functions.  Made
//                         'ainRefNum' and 'aoutRefNum' global (not sure
//                         how it could have worked before)
//

#include "ownet.h"
#include "ds2480.h"
#include <Serial.h>
#include <Events.h>

// exportable functions
void FlushCOM(int);
SMALLINT WriteCOM(int,int,uchar *);
int ReadCOM(int,int,uchar *);
void BreakCOM(int);
void SetBaudCOM(int,int);
SMALLINT OpenCOM(int,char *);
void CloseCOM(int);
void msDelay(int);
long msGettick(void);

// MacLNK globals needed
static Handle inputBufferHandle;

// MacLNK constants
const short INPUTBUFFERSIZE = 2048;
short ainRefNum;     // serial port A input
short aoutRefNum;    // serial port A output
int   globalPortnumCounter = 0;

//---------------------------------------------------------------------------
// Attempt to open a com port.  Keep the handle in ComID.
// Set the starting baud rate to 9600.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number provided will
//               be used to indicate the port number desired when calling
//               all other functions in this library.
//
//
// Returns: the port number if it was succesful otherwise -1
//
int OpenCOMEx(char *port_zstr)
{
   int portnum = globalPortnumCounter++;

   if(!OpenCOM(portnum, port_zstr))
   {
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------------------
//  Description:
//     Attempt to open the modem port.
//
SMALLINT OpenCOM(int portnum, char *port_zstr)
{
	const Str255 AIN  = "\p.AIn"; 	// {4,'.','A','I','n'};
	const Str255 AOUT = "\p.AOut";	// {5,'.','A','O','u','t'};

	OSErr res;

	res = OpenDriver(AOUT, &aoutRefNum);
	if ( res == noErr )
	{
		res = OpenDriver(AIN, &ainRefNum);
		if ( res == noErr )
		{
			res = SerReset(aoutRefNum, baud9600 + data8 + noParity + stop10);
			if ( res == noErr )
			{
				inputBufferHandle = SetBufferCOM(INPUTBUFFERSIZE);
				if ( inputBufferHandle != nil )
				{
					res = noErr;
				}
				else
				{
               OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
					res = statusErr;
				}
			}
			else
            OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
		}
		else
         OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
	}
	else
      OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);

	return (res == noErr);
}


//---------------------------------------------------------------------------
//  Description:
//     Closes the connection to the modem port.
//
void CloseCOM(int portnum)
{
	Handle h;
	OSErr res;

	res = KillIO(aoutRefNum);
	if ( res == noErr )
	{
		res = CloseDriver(ainRefNum);     // serial port A input
	}
	if ( res == noErr )
	{
		res = CloseDriver(aoutRefNum);    // serial port A output
	}

	h = ReleaseBufferCOM(inputBufferHandle);	// release input buffer
}


//--------------------------------------------------------------------------
// Write an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set.
//
// Returns 1 for success and 0 for failure
//
SMALLINT WriteCOM(int portnum, int outlen, uchar *outbuf)
{
	OSErr wres;
	long count = outlen;

	SerStaRec serSta;
	OSErr sres;

	wres = FSWrite(aoutRefNum, &count, outbuf);

	do
	{
		sres = SerStatus(aoutRefNum, &serSta);
	} while (serSta.wrPend != 0);			// wait for write pending to clear

	// It is not entirely clear from "Inside Macintosh" what serSta.wrPend == 0
	// really indicates. It is probably an indication that the output (driver)
	// buffer is empty. There could still be data in the transmit chain.
	// Inspection of bit 0 of RR1 of the Z8530 would tell us whether the tx
	// chain was clear, but RR1 cannot be safely accessed in user mode.
	// Pending further thoughts on this, the following delay is included to
	// give the tx chain time to clear. This is a worst case estimate.

	msDelay(1000*count/960); 	// Delay 1000/960 ms for each character sent.

	msDelay(1);					// Delay an extra 16.67+ ms for reliability

	return (wres == noErr);
}


//--------------------------------------------------------------------------
// Read an array of bytes to the COM port.
// Assume that baud rate has been set.
//
// Returns number of characters read
//
int ReadCOM(int portnum, int inlen, uchar *inbuf)
{
	OSErr res;
	long available;

	res = SerGetBuf(ainRefNum, &available);
	if ( available > 0 )
	{
		if ( available > inlen )
		{
			available = inlen;
		}
		res = FSRead(ainRefNum, &available, inbuf);
	}
	return available;
}


//---------------------------------------------------------------------------
//  Description:
//     flush the rx and tx buffers
//
void FlushCOM(int portnum)
{
	Str255 buffer;
	int len;
	OSErr res;

	res = KillIO(aoutRefNum);
	res = KillIO(ainRefNum);

	do
	{
		len = ReadCOM(256, buffer);
	} while ( len > 0 );						// Probably overkill !
}


//--------------------------------------------------------------------------
//  Description:
//     Send a break on the com port for at least 2 ms
//
void BreakCOM(int portnum)
{
	OSErr res;

	res = SerSetBrk(aoutRefNum);
	msDelay(2);
	res = SerClrBrk(aoutRefNum);
}


//--------------------------------------------------------------------------
// Set the baud rate on the com port.  The possible baud rates for
// 'new_baud' are:
//
// PARMSET_9600     0x00
// PARMSET_19200    0x02
// PARMSET_57600    0x04
// PARMSET_115200   0x06  -- we can't do this on the Macintosh
//
void SetBaudCOM(int portnum, int new_baud)
{
   OSErr res;

	if (new_baud == PARMSET_9600)
	{
		res = SerReset(aoutRefNum, baud9600 + data8 + noParity + stop10);
	}
	else if (new_baud == PARMSET_19200)
	{
		res = SerReset(aoutRefNum, baud19200 + data8 + noParity + stop10);
	}
	else if (new_baud == PARMSET_57600)
	{
		res = SerReset(aoutRefNum, baud57600 + data8 + noParity + stop10);
	}
}


//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void) // (1.03)
{
   return (50L * TickCount()) / 3L;
}

//---------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void msDelay(int len)
{
	long interval = ( (len + 15) / 16 );		// Macintosh ticks every 16.67 millisecs
   	long endTick;

	Delay(interval, &endTick);
}


//---------------------------------------------------------------------------
//  Description:
//     Setup (new) input buffer.
//     Handle returned for use in ReleaseBufferCOM.
//
Handle SetBufferCOM(short size)
{
	Handle h;
	Ptr	p;
	OSErr res;

	h = NewHandle(size);
	if ( h != nil )
	{
		HLock(h);
		p = *h;
		res = SerSetBuf(ainRefNum, p, size);
		if ( res != noErr )
		{
			HUnlock(h);
			DisposHandle(h);
			h = nil;
		}
	}
	return h;
}


//---------------------------------------------------------------------------
//  Description:
//     Release (new) input buffer. Go back to standard buffer.
//
Handle ReleaseBufferCOM(Handle h)
{
	Ptr	p;
	OSErr res;

	if ( h != nil )
	{
		p = *h;
		res = SerSetBuf(ainRefNum, p, 0);
		if ( res == noErr )
		{
			HUnlock(h);
			DisposHandle(h);
			h = nil;
		}
	}
	return h;
}

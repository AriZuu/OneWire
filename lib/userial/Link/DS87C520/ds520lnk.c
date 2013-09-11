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
//  ds520lnk.C - Serial functions using 8051 to be used as a test
//               for DS2480 based Universal Serial Adapter 'U'
//               functions.
//
//  Version: 1.00
//
//  TODO:	1) Clean up ugly code.
//			2) Allow for compile-time choice of using Timer2 for baud rate of Serial1
//			3) Package Serial0 and Serial1 SFRs into nice structs, hopefully to allow
//			   for removal of all the if-else statements in each function.
//			4) Do the math and write a better msDelay function.
//

// -- Keil c51 control directives
// Using inline assembly means we have to dump the output of the entire file to
// an intermediate assembly assembly source file, then assemble that file so
// we can have our object for linking.
#pragma SRC(ds520lnk.a51)
// -- End directives


#include "ownet.h"
#include "microser.h"
#include "ds2480.h"

/* external functions */
/* serial0 routines */
extern void serial0_init(uchar);
extern void serial0_flush(void);
/* extern void serial0_handler(void) */
extern void serial0_putchar(char);
extern char serial0_getchar(void);
extern char serial0_peek(void);

/* serial1 routines */
extern void serial1_init(uchar);
extern void serial1_flush(void);
/* extern void serial1_handler(void) */
extern void serial1_putchar(char);
extern char serial1_getchar(void);
extern char serial1_peek(void);

/* exportable functions */
SMALLINT  OpenCOM(int, char*);
SMALLINT  WriteCOM(int, int, uchar*);
void      CloseCOM(int);
void      FlushCOM(int);
int       ReadCOM(int, int, uchar*);
void      BreakCOM(int);
void      SetBaudCOM(int, uchar);
void      msDelay(unsigned int);
void      usDelay(unsigned int);
long      msGettick(void);

//---------------------------------------------------------------------------
//-------- COM required functions for MLANU
//---------------------------------------------------------------------------

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
   int portnum;

   if(port_zstr[0] == '0')
   {
      portnum = 0;
   }
   else
   {
      portnum = 1;
   }

   if(!OpenCOM(portnum, port_zstr))
   {
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------------------
// Attempt to open a com port.  Keep the handle in ComID.
// Set the starting baud rate to 9600.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number provided will
//               be used to indicate the port number desired when calling
//               all other functions in this library.
//
// 'port_zstr' - zero terminate port name.  NOT USED.
//
//
// Returns: TRUE(1)  - success, COM port opened
//          FALSE(0) - failure, could not open specified port
//
SMALLINT OpenCOM(int portnum, char *port_zstr)
{
	port_zstr = 0; //to silence compiler

   if(portnum==0)
      serial0_init(BAUD9600_TIMER_RELOAD_VALUE);
   else
      serial1_init(BAUD9600_TIMER_RELOAD_VALUE);

   FlushCOM(portnum);

	return TRUE;
}

//---------------------------------------------------------------------------
// Closes the connection to the port.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
void CloseCOM(int portnum)
{
	if(portnum==0)
	{
	   ES0 = 0; //disable serial interrupt
	   serial0_flush();
	}
	else if(portnum==1)
	{
	   ES1 = 0; //disable serial interrup
	   serial1_flush();
	}
}

//---------------------------------------------------------------------------
// Flush the rx and tx buffers
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
void FlushCOM(int portnum)
{
	if(portnum==0)
	   serial0_flush();
	else
	   serial1_flush();
}

//--------------------------------------------------------------------------
// Write an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'outlen'   - number of bytes to write to COM port
// 'outbuf'   - pointer ot an array of bytes to write
//
// Returns:  TRUE(1)  - success
//           FALSE(0) - failure
//
SMALLINT WriteCOM(int portnum, int outlen, uchar *outbuf)
{
	int i = 0;
	if(portnum==0)
	{
   	for(; i<outlen; i++)
   	{
   		serial0_putchar(outbuf[i]);
   	}
   }
   else
   {
   	for(; i<outlen; i++)
   	{
   		serial1_putchar(outbuf[i]);
   	}
   }
	return (i==outlen?TRUE:FALSE); //unnecessary, but thinking ahead
}

//--------------------------------------------------------------------------
// Read an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//               OpenCOM to indicate the port number.
// 'inlen'     - number of bytes to read from COM port
// 'inbuf'     - pointer to a buffer to hold the incomming bytes
//
// Returns: number of characters read
//
int ReadCOM(int portnum, int inlen, uchar *inbuf)
{
	volatile int i = 0;
	volatile uchar cnt = 5;
	volatile uchar quit = FALSE;
	if(portnum==0)
	{
   	for(; !quit && i<inlen; cnt=5)
   	{
   	   while( !serial0_peek() && (cnt--)>0 )
   	      msDelay(10);

   	   if(!serial0_peek())
   	      quit = TRUE;
   	   else
      		inbuf[i++] = serial0_getchar();
   	}
   }
   else
   {
   	for(; !quit && i<inlen; cnt=5)
   	{
   	   while( !serial1_peek() && (cnt--)>0 )
   	      msDelay(10);

   	   if(!serial1_peek())
            quit = TRUE;
         else
      		inbuf[i++] = serial1_getchar();
   	}
   }

	return i;
}

//--------------------------------------------------------------------------
// Send a break on the com port for at least 2 ms
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
void BreakCOM(int portnum)
{
	if(portnum==0)
	{
	   //hold P3.1 low
	   //ES0 = 0;
      P3 &= 0xFE;
		msDelay(4);
      P3 |= 0x01;
      //ES0 = 1;
   }
   else
	{
	   //hold P1.3 low
	   //ES1 = 0;
   	P1 &= 0xF7;
		msDelay(4);
   	P1 |= 0x08;
   	//ES1 = 1;
	}
}

//--------------------------------------------------------------------------
// Set the baud rate on the com port.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number was provided to
//               OpenCOM to indicate the port number.
// 'new_baud'  - new baud rate defined as
//                PARMSET_9600     0x00
//                PARMSET_19200    0x02
//                PARMSET_57600    0x04
//                PARMSET_115200   0x06
//
void SetBaudCOM(int portnum, uchar new_baud)
{
   int reload_value;
	switch (new_baud)
	{
		case PARMSET_9600:
			reload_value = BAUD9600_TIMER_RELOAD_VALUE;
			break;
		case PARMSET_19200:
			reload_value = BAUD19200_TIMER_RELOAD_VALUE;
			break;
		case PARMSET_57600:
			reload_value = BAUD57600_TIMER_RELOAD_VALUE;
			break;
		case PARMSET_115200:
			reload_value = BAUD115200_TIMER_RELOAD_VALUE;
			break;
		default:
		   return;
	}

#if USE_TIMER2_FOR_SERIAL0
   if(portnum==0)
   {
      TR2 = 0; // stop timer 2
      TL2 = RCAP2L = reload_value;
      TH2 = RCAP2H = reload_value>>8;
      TF2 = 0; // clear timer 2 overflow flag
      TR2 = 1; // start timer 2
   }
   else
   {
#else
      portnum = 0;
#endif
   	TR1 = 0; // stop timer 1
		TL1 = TH1 = reload_value;
		TF1 = 0; // clear timer 1 overflow flag
		TR1 = 1; // start timer 1
#if USE_TIMER2_FOR_SERIAL0
	}
#endif
}


//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void msDelay(unsigned int delay)
{
	volatile int j;
   P1 = 2;
	do
   {
      j=425;
      do
		{
		   _nop_();
		   _nop_();
		   _nop_();
		   _nop_();
		   _nop_();
		}
		while(--j);
	}
   while(--delay);
	P1 = 6;
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' microseconds
//
// Adapted from Kevin Vigor's TINI 1Wire code.
//
// Works only @33Mhz.  We need 8.25 machine cycles per iteration to get the microsecond
// delay.  Can't have .25 of a cycle, so this function uses 8 cycles per iteration. This
// produces an error of 3% ( .03 = (8.25-8)/8 ).  Most of the use of this function by
// 1-Wire routines calls for delays under 50 microseconds, in which case it is under by
// less than 1.5 microseconds.  It is assumed the overhead of calling the function will
// add enough to account for the difference.  For values much greater than 50 (only in
// reset function), a "fudge factor" is added to account for any error.
//
//
void usDelay(unsigned int delay)
{
   P1 = 3;
#pragma asm
   ; delay is in r6/r7
   ; double-check in output assembly
   mov  a, r7
   orl  a, r6			; quick out for zero case.
   jz   _usDelayDone

   inc  r6
   cjne r7, #0, _usDelayLoop
   dec  r6

   _usDelayLoop:
   nop
   nop
   nop
   nop
   nop
   djnz r7, _usDelayLoop
   djnz r6, _usDelayLoop
   _usDelayDone:

#pragma endasm
   P1 ^= 4;
   delay = 0;
   P1 ^= 5;
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
	return (((long)TH1)<<8)+TL1;
}




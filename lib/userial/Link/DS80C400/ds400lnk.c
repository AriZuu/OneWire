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
//  ds400lnk.C - Serial functions using a DS80C400 microcontroller supporting
//               all three serial ports.
//
//  Version: 1.00
//

// -- Keil c51 control directives
// Using inline assembly means we have to dump the compiler output of our code to
// an intermediate assembly source file, then assemble that file so we can have
// our object for linking.
//#pragma SRC(ds400lnk.a51)
// -- End directives


#include "ownet.h"
#include "ser400.h"
#include "ds2480.h"
#include <string.h>

/* serial0 routines */
extern void serial0_init(uchar);
extern void serial0_flush(void);
extern void serial0_putchar(char);
extern char serial0_getchar(void);
extern char serial0_peek(void);
extern void serial0_break(uchar);
extern void serial0_shutdown(void);

/* serial1 routines */
extern void serial1_init(uchar);
extern void serial1_flush(void);
extern void serial1_putchar(char);
extern char serial1_getchar(void);
extern char serial1_peek(void);
extern void serial1_break(uchar);
extern void serial1_shutdown(void);

/* serial2 routines */
extern void serial2_init(uchar);
extern void serial2_flush(void);
extern void serial2_putchar(char);
extern char serial2_getchar(void);
extern char serial2_peek(void);
extern void serial2_break(uchar);
extern void serial2_shutdown(void);

/* exportable functions */
int       OpenCOM(char*);
SMALLINT  WriteCOM(int, int, uchar*);
void      CloseCOM(int);
void      FlushCOM(int);
int       ReadCOM(int, int, uchar*);
void      BreakCOM(int);
void      SetBaudCOM(int, uchar);
void      msDelay(unsigned int);
void      usDelay(unsigned int);
long      msGettick(void);

/* A new exportable function for dynamic crystal usage */
void      SetCPUClockSpeed(long);

// local routines
void usDelay(unsigned int);
uchar getreloadvalue(long);

// local variable
int port_handle[4];

#ifdef USE_DYNAMIC_CPU_CLOCK
/* Variable containing the current CPU clock rate */
long cpuClockRate;
#endif

//---------------------------------------------------------------------------
//-------- COM required functions for MLANU
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Attempt to open a com port.  Keep the handle in ComID.
// Set the starting baud rate to 9600.
//
// 'port_zstr' - zero terminate port name.
//
// Returns: valid handle, or -1 if an error occurred
//
int OpenCOMEx(char *port_zstr)
{
   int portnum;

   if (strcmp(port_zstr,DS400_SERIAL0_STRING) == 0)
     portnum = DS400_SERIAL0;
   else if (strcmp(port_zstr,DS400_SERIAL1_STRING) == 0)
     portnum = DS400_SERIAL1;
   else if (strcmp(port_zstr,DS400_SERIAL2_STRING) == 0)
     portnum = DS400_SERIAL2;
   else
     return -1;

   if(!OpenCOM(portnum, NULL))
   {
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------------------
// Attempt to open a com port.  Keep the handle in ComID.
// Set the starting baud rate to 9600.
//
// 'port_zstr' - zero terminate port name.
//
// Returns: valid handle, or -1 if an error occurred
//
SMALLINT OpenCOM(int portnum, char *port_zstr)
{
   int actualPortnum;

   if(port_zstr)
   {
      if (strcmp(port_zstr,DS400_SERIAL0_STRING) == 0)
        actualPortnum = DS400_SERIAL0;
      else if (strcmp(port_zstr,DS400_SERIAL1_STRING) == 0)
        actualPortnum = DS400_SERIAL1;
      else if (strcmp(port_zstr,DS400_SERIAL2_STRING) == 0)
        actualPortnum = DS400_SERIAL2;
      else
        return FALSE;
   }
   else
      actualPortnum = portnum;

   // attempt to open the communications port
   port_handle[portnum] = actualPortnum;

   switch (actualPortnum)
   {
     case DS400_SERIAL0:
       serial0_init(BAUD9600_TIMER_RELOAD_VALUE);
       break;
     case DS400_SERIAL1:
       serial1_init(BAUD9600_TIMER_RELOAD_VALUE);
       break;
     case DS400_SERIAL2:
       serial2_init(BAUD9600_TIMER_RELOAD_VALUE);
       break;
   }

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
   switch (port_handle[portnum])
   {
     case DS400_SERIAL0:
	    serial0_shutdown();
       break;
     case DS400_SERIAL1:
	    serial1_shutdown();
       break;
     case DS400_SERIAL2:
	    serial2_shutdown();
       break;
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
   switch (port_handle[portnum])
   {
	  case DS400_SERIAL0:
       serial0_flush();
       break;
	  case DS400_SERIAL1:
	    serial1_flush();
       break;
	  case DS400_SERIAL2:
	    serial2_flush();
       break;
   }
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

   switch (port_handle[portnum])
   {
	  case DS400_SERIAL0:
       for(; i<outlen; i++)
       {
         serial0_putchar(outbuf[i]);
       }
       break;
	  case DS400_SERIAL1:
       for(; i<outlen; i++)
       {
         serial1_putchar(outbuf[i]);
       }
       break;
	  case DS400_SERIAL2:
       for(; i<outlen; i++)
       {
         serial2_putchar(outbuf[i]);
       }
       break;
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
	int i = 0;
	uchar cnt = 5; //50ms timeout
	uchar quit = FALSE;

   switch (port_handle[portnum])
   {
	  case DS400_SERIAL0:
       for(; !quit && i<inlen; cnt=5)
       {
         while( !serial0_peek() && (cnt--)>0 )
           msDelay(10);

         if(!serial0_peek())
           quit = TRUE;
         else
          inbuf[i++] = serial0_getchar();
       }
       break;
	  case DS400_SERIAL1:
       for(; !quit && i<inlen; cnt=5)
       {
         while( !serial1_peek() && (cnt--)>0 )
           msDelay(10);

         if(!serial1_peek())
           quit = TRUE;
         else
           inbuf[i++] = serial1_getchar();
       }
       break;
	  case DS400_SERIAL2:
       for(; !quit && i<inlen; cnt=5)
       {
         while( !serial2_peek() && (cnt--)>0 )
           msDelay(10);

         if(!serial2_peek())
           quit = TRUE;
         else
           inbuf[i++] = serial2_getchar();
       }
       break;
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
   switch (port_handle[portnum])
   {
	  case DS400_SERIAL0:
       serial0_break(1);
       msDelay(4);
       serial0_break(0);
       break;
	  case DS400_SERIAL1:
       serial1_break(1);
       msDelay(4);
       serial1_break(0);
       break;
	  case DS400_SERIAL2:
       serial2_break(1);
       msDelay(4);
       serial2_break(0);
       break;
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
#ifdef USE_DYNAMIC_CPU_CLOCK
         reload_value = getreloadvalue(9600);
#else
         reload_value = BAUD9600_TIMER_RELOAD_VALUE;
#endif
         break;
      case PARMSET_19200:
#ifdef USE_DYNAMIC_CPU_CLOCK
         reload_value = getreloadvalue(19200);
#else
         reload_value = BAUD19200_TIMER_RELOAD_VALUE;
#endif
         break;
      case PARMSET_57600:
#ifdef USE_DYNAMIC_CPU_CLOCK
         reload_value = getreloadvalue(57600);
#else
         reload_value = BAUD57600_TIMER_RELOAD_VALUE;
#endif
         break;
      case PARMSET_115200:
#ifdef USE_DYNAMIC_CPU_CLOCK
         reload_value = getreloadvalue(115200);
#else
         reload_value = BAUD115200_TIMER_RELOAD_VALUE;
#endif
         break;
      default:
         return;
   }

   switch (port_handle[portnum])
   {
	  case DS400_SERIAL0:
#if USE_TIMER2_FOR_SERIAL0
       TR2 = 0; // stop timer 2
       TL2 = RCAP2L = reload_value;
       TH2 = RCAP2H = reload_value>>8;
       TF2 = 0; // clear timer 2 overflow flag
       TR2 = 1; // start timer 2
#else
       TR1 = 0; // stop timer 1
       TL1 = TH1 = reload_value;
       TF1 = 0; // clear timer 1 overflow flag
       TR1 = 1; // start timer 1
#endif
       break;
	  case DS400_SERIAL1:
       TR1 = 0; // stop timer 1
       TL1 = TH1 = reload_value;
       TF1 = 0; // clear timer 1 overflow flag
       TR1 = 1; // start timer 1
       break;
	  case DS400_SERIAL2:
	   T3CM &= ~(0xC0);  /* stop timer 3, and clear overflow flag */
       TL3 = TH3 = reload_value;
	   T3CM |= 0x40;  /* start timer 3 */
       break;
   }
}


//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void msDelay(unsigned int delay)
{
   while(delay--)
   {
      usDelay(1030);
   }
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
   djnz r7, _usDelayLoop
   djnz r6, _usDelayLoop
   _usDelayDone:
#pragma endasm

//   delay = 0;
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
	return (long)TL1;
}

/*--------------------------------------------------------------------------
 * Set the CPU frequency for use in dynamic baud rate calculations.
 *
 * 'newcpuspeed' - Frequency of the CPU (in Hertz).
 */
void SetCPUClockSpeed(long newcpuspeed)
{
#ifdef USE_DYNAMIC_CPU_CLOCK
  cpuClockRate = newcpuspeed;
#endif
}

#ifdef USE_DYNAMIC_CPU_CLOCK
/*--------------------------------------------------------------------------
 * Set the CPU frequency for use in dynamic baud rate calculations.
 *
 * 'newcpuspeed' - Frequency of the CPU (in Hertz).
 */
uchar getreloadvalue(long baudrate)
{
   long result;

/*
   Timer reload values for baud rates:
   TH1 = 256 - ( (2^SMOD/32) * ( xtal / T1DIV * baud) )
*/

   result = cpuClockRate*2;
   result /= (32L*4*baudrate);
   result = 256 - result;
   return result;
}
#endif


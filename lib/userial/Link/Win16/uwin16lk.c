//---------Copyright (C) 1997 Dallas Semiconductor Corporation--------------
//
//  COMUT16.C - This source file contains the COM specific utility funcs
//              for 16 bit COM communication.
//
//  Compiler: VC 1.52c
//  Version: 1.00
//

#include <windows.h>
#include <stdio.h>
#include "ownet.h"
#include "ds2480.h"

// local functions
SMALLINT SetupCOM(int, ulong);
void     MarkTime(void);
SMALLINT ElapsedTime(long);
long     msGettick(void);
void     Sleep(long);
void     msDelay(int);

// globals needed
static int ComID[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,};
static long TimeStamp;
static char port[5];
int         globalPortnumCounter = 0;

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

   // check to see if exceeded number of ports
   if (globalPortnumCounter >= MAX_PORTNUM)
      globalPortnumCounter = 0;

   if(!OpenCOM(portnum, port_zstr))
   {
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------
//  Description:
//     Attept to open a com port.
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  port_zstr  - the string of the port to be used
//     Returns FALSE if unsuccessful and TRUE if successful.
//
SMALLINT OpenCOM(int portnum, char *port_zstr)
{
   int i;

   for(i=0;i<4;i++)
      port[i] = port_zstr[i];

   // Check if port has already been opened
   if (ComID[portnum] < 0) // Not opened
   {
      // Obtain port id
      ComID[portnum] = OpenComm(port_zstr,1024,1024);

      if (ComID[portnum] < 0)
      {
         OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
         return FALSE;
      }
   }
   else
   {
      return TRUE; // Already opened
   }

   if(!SetupCOM(portnum,CBR_9600)) // Reset unsuccessful
   {
      CloseCOM(portnum);
      OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
      return FALSE;
   }

   return TRUE;
}


//------------------------------------------------------------------
//  Description:
//     This routines sets up the DCB and performs a SetCommState().
//     SetupCOM assumes OpenComm has previously been called.
//     Returns 0 if unsuccessful and 1 if successful.
//
//  Prt      - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  baudrate - the baudrate that should be used in setting up the port.
//
//  return TRUE if the port was setup correctly
//
SMALLINT SetupCOM(int Prt, ulong baudrate)
{
   DCB dcb;

   dcb.Id = (uchar)ComID[Prt];            // current device: from OpenComm
   dcb.BaudRate = (short)baudrate;        // current baud rate
   dcb.ByteSize = 8;                      // number of bits/byte, 4-8
   dcb.Parity = NOPARITY;                 // 0-4=no,odd,even,mark,space
   dcb.StopBits = ONESTOPBIT;             // 0,1,2 = 1, 1.5, 2
   dcb.RlsTimeout = 0;                // Receive-line-signal-detect
   dcb.CtsTimeout = 0;                // Clear-to-send
   dcb.DsrTimeout = 0;                // Data-set-ready

   dcb.fBinary = TRUE;                    // binary mode, no EOF check
   dcb.fRtsDisable = FALSE;              // disables Rts
   dcb.fParity = FALSE;                   // enable parity checking
   dcb.fOutxCtsFlow = FALSE;              // CTS output flow control
   dcb.fOutxDsrFlow = FALSE;              // DSR output flow control
   dcb.fDtrDisable = FALSE;              // disables Dtr

   dcb.fOutX = FALSE;                     // XON/XOFF out flow control
   dcb.fInX = FALSE;                      // XON/XOFF in flow control
   dcb.fPeChar = FALSE;                  // enable error replacement
   dcb.fNull = FALSE;                     // enable null stripping
   dcb.fChEvt = FALSE;                  // received event character flag

   //??dcb.fDtrflow = FALSE;               // DTR flow control type
   //??dcb.fRtsflow = FALSE;               // RTS flow control

   dcb.XonChar = 0;                       // Tx and Rx XON character
   dcb.XoffChar = 1;                      // Tx and Rx XOFF character
   dcb.XonLim = 0;                        // transmit XON threshold
   dcb.XoffLim = 0;                       // transmit XOFF threshold
   dcb.PeChar = 0;                        // parity error replacement character
   dcb.EofChar = 0;                       // end of input character
   dcb.EvtChar = 0;                       // received event character

   if (SetCommState(&dcb) < 0)
      return FALSE;
   else
      return TRUE;
}


//-------------------------------------------------------------------
//  Description:
//     Closes the connection to the port.
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
//     Returns FALSE if unsuccessful and TRUE if successful.
//
void CloseCOM(int portnum)
{
   if (ComID[portnum] >= 0)
   {
      // Flush and close current port
      FlushComm(ComID[portnum],0);
      FlushComm(ComID[portnum],1);
      // close the port
      CloseComm(ComID[portnum]);
      // reset port and return
      ComID[portnum] = -1;
   }
}

//-------------------------------------------------------------------
//  Description:
//     flush the rx and tx buffers
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
void FlushCOM(int portnum)
{
   char ch[] = {"X"};
   COMSTAT stat;

   for (;;)
   {
      GetCommError(ComID[portnum],&stat);
      if (stat.cbInQue)
         ReadComm(ComID[portnum],ch,1);
      else
         break;
   }
   FlushComm(ComID[portnum],0);
   FlushComm(ComID[portnum],1);
}


//-------------------------------------------------------------------
// Write an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set and the buffers have
// been flushed.
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  outlen     - the length of the data to be written
//  outbuf     - the output data
//
// Returns TRUE for success and FALSE for failure
//
int WriteCOM(int portnum, int outlen, uchar *outbuf)
{
   short result;
   COMSTAT ComStat;
   ulong m;
   // declare and set the default timeout
   int timeout = 20 * outlen + 60;

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DODEBUG
   short i;
      printf("W[");
      for (i = 0; i < outlen; i++)
         printf("%02X ",outbuf[i]);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // Send data to write buffer
   result = WriteComm(ComID[portnum],outbuf,outlen);

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DODEBUG
         printf("(%d)]",result);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
   // loop to wait for the write to complete
   if (result == outlen)
   {
      m = (ulong)msGettick() + (ulong)timeout;
      do
      {
         // yield this process
         //Yield();
         GetCommError(ComID[portnum],&ComStat);
         if ((short)ComStat.cbOutQue == 0)
            return result;
      }
      while ((ulong)msGettick() <= m);
   }
   else
      return FALSE;
}


//-------------------------------------------------------------------
// Read an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set and the buffers have
// been flushed.
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  inlen      - the length of the data that was read
//  outbuf     - the input data
//
// Returns number of characters read
//
int ReadCOM(int portnum, int inlen, uchar *inbuf)
{
   COMSTAT ComStat;
   short result;
   ulong m;
   ulong more;
   // declare and set the default timeout
   int timeout = 20 * inlen + 60;

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DODEBUG
   short i;
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

   m = (ulong)msGettick() + (ulong)timeout;
   do
   {
      GetCommError(ComID[portnum],&ComStat);
      if ((short)ComStat.cbInQue >= inlen)
      {
         result = ReadComm(ComID[portnum],inbuf,inlen);

         if (result == (int)inlen)
         {
            GetCommError(ComID[portnum],&ComStat);
            more = ComStat.cbInQue;

            //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
            #ifdef DODEBUG
               printf("R[");
               for (i = 0; i < inlen; i++)
                  printf("%02X ",inbuf[i]);
               printf("]");
            #endif
            //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
            return result;
         }
         else
            return FALSE;
      }
      //else
         // yield this process
         //Yield();

   }
   while ((ulong)msGettick() <= m);

   return FALSE;
}


//-------------------------------------------------------------------
//  Description:
//     Send a break on the com port for "len" msec
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
void BreakCOM(int portnum)
{
   // start the reset pulse
   SetCommBreak(ComID[portnum]);

   // delay
   msDelay(2);

   // clear the reset pulse and return
   // PROGRAMMER'S NOTE: ClearCommBreak is returning 24:an undefined code
   ClearCommBreak(ComID[portnum]);

   // Win3.1 bug, close and then open port
   CloseCOM(portnum);
   OpenCOM(portnum,&port[0]);
}

//--------------------------------------------------------------------------
// Replacement for sleep
//
// dwMilliseconds  - the number of milliseconds to sleep
//
void Sleep(long dwMilliseconds)
{
   long limit;

   // setup the limit
   limit = msGettick() + dwMilliseconds;

   // do the delay
   while (limit >= msGettick())
   {
      // yield this process
      //??? Yield here does bad things, Yield();
   }
}


//--------------------------------------------------------------------------
// Delays for the given amout of time
//
// len  - the number of milliseconds to sleep
//
void msDelay(int len)
{
   Sleep(len);
}

//--------------------------------------------------------------------------
// Mark time stamp for later comparison
//
void MarkTime(void)
{
   // get the timestamp
   TimeStamp = msGettick();
}


//--------------------------------------------------------------------------
// Check if timelimit number of ms have elapse from the call to MarkTime
//
// timelimit  - the time limit for the elapsed time
//
// Return TRUE if the time has elapse else FALSE.
//
SMALLINT ElapsedTime(long timelimit)
{
   // check if time elapsed
   return ((TimeStamp + timelimit) < msGettick());
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
   DCB dcb;

   // get the current com port state
   GetCommState(ComID[portnum], &dcb);

   // change just the baud rate
   switch (new_baud)
   {
      case PARMSET_19200:
         dcb.BaudRate = CBR_19200;
         break;
      case PARMSET_9600:
      default:
         dcb.BaudRate = CBR_9600;
         break;
   }

   // restore to set the new baud rate
   SetCommState(&dcb);
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
// return the number of ticks.
//
long msGettick(void)
{
   return GetTickCount();
}
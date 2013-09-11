// stdafx.cpp : source file that includes just the standard includes
//	first.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "ownet.h"
#include "WinCElnk.h"
#include "ibhs.h"

// local functions
int OpenCOM(int, char *);      //IBAPI
int OpenCOMEx(char *);
int SetupCOM(short, ulong);    //IBAPI
void FlushCOM(int);
void CloseCOM(int);            //IBAPI
//IBAPI WriteCOM(int, int, uchar IBFAR *);
int WriteCOM(int, int, uchar *);
//IBAPI ReadCOM(int, int, uchar IBFAR *);
int ReadCOM(int,int,uchar *);
void BreakCOM(int);            //IBAPI
int PowerCOM(short, short);   //IBAPI
int RTSCOM(short, short);     //IBAPI
int FreeCOM(short);           //IBAPI
void StdFunc MarkTime(void);
int ElapsedTime(long);        //IBAPI
int DSRHigh(short);           //IBAPI
void msDelay(int len);
void SetBaudCOM(int, uchar);


static DWORD TimeStamp;
int          globalPortnumCounter = 0;

//---------------------------------------------------------------------------
// Utility Functions
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

//---------------------------------------------------------------------------
//  Description:
//  Attept to open a com port.  Keep the handle in ComID.
//
int OpenCOM(int Prt, char *port_zstr)
{
   TCHAR szPort[15];
   short fRetVal;
   COMMTIMEOUTS CommTimeOuts;

   // Only open if not already open
   if (ComID[Prt] == 0)
   {
      // load the COM prefix string and append port number
      wsprintf(szPort, TEXT("COM%d:"), Prt);

      // open COMM device

      if ((ComID[Prt] =
               CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                                      0,        // exclusive access
                                      NULL,     // no security attrs
                                      OPEN_EXISTING,
                                      0,
                                      NULL )) == (HANDLE) -1 )
      {
         ComID[Prt] = 0;
         return (FALSE) ;
      }
      else
      {
          // get any early notifications
          SetCommMask(ComID[Prt], EV_RXCHAR | EV_TXEMPTY | EV_ERR | EV_BREAK);

          // setup device buffers
          SetupComm(ComID[Prt], 1030, 1030);

          // purge any information in the buffer
          PurgeComm(ComID[Prt], PURGE_TXABORT | PURGE_RXABORT |
                                           PURGE_TXCLEAR | PURGE_RXCLEAR );

          // set up for overlapped non-blocking I/O (3.00Beta4)
          CommTimeOuts.ReadIntervalTimeout = DEF_READ_INT;
          CommTimeOuts.ReadTotalTimeoutMultiplier = DEF_READ_MULT;
          CommTimeOuts.ReadTotalTimeoutConstant = DEF_READ_CONST;
          CommTimeOuts.WriteTotalTimeoutMultiplier = DEF_WRITE_MULT;
          CommTimeOuts.WriteTotalTimeoutConstant = DEF_WRITE_CONST;
          SetCommTimeouts(ComID[Prt], &CommTimeOuts);
      }
   }

   // get the connection
   fRetVal = SetupCOM(Prt,brate);

   if (!fRetVal)
   {
      CloseHandle(ComID[Prt]);
      ComID[Prt] = 0;
   }

   return (fRetVal);
}


//---------------------------------------------------------------------------
//  Description:
//     This routines sets up the DCB and performs a SetCommState().
//
int SetupCOM(short Prt, ulong baudrate)
{
   short fRetVal;
   DCB dcb;

   dcb.DCBlength = sizeof(DCB);

   GetCommState(ComID[Prt], &dcb);

   dcb.BaudRate = baudrate;               // current baud rate
   dcb.fBinary = TRUE;                    // binary mode, no EOF check
   dcb.fParity = FALSE;                   // enable parity checking
   dcb.fOutxCtsFlow = FALSE;              // CTS output flow control
   dcb.fOutxDsrFlow = FALSE;              // DSR output flow control
   //???dcb.fDtrControl = DTR_CONTROL_ENABLE;  // DTR flow control type
   dcb.fDsrSensitivity = FALSE;           // DSR sensitivity
   dcb.fTXContinueOnXoff = TRUE;          // XOFF continues Tx
   dcb.fOutX = FALSE;                     // XON/XOFF out flow control
   dcb.fInX = FALSE;                      // XON/XOFF in flow control
   dcb.fErrorChar = FALSE;                // enable error replacement
   dcb.fNull = FALSE;                     // enable null stripping
   //???dcb.fRtsControl = RTS_CONTROL_ENABLE;  // RTS flow control
   dcb.fAbortOnError = FALSE;             // abort reads/writes on error
   dcb.XonLim = 0;                        // transmit XON threshold
   dcb.XoffLim = 0;                       // transmit XOFF threshold
   dcb.ByteSize = 8;                      // number of bits/byte, 4-8
   dcb.Parity = NOPARITY;                 // 0-4=no,odd,even,mark,space
   dcb.StopBits = ONESTOPBIT;             // 0,1,2 = 1, 1.5, 2
   dcb.XonChar = 0;                       // Tx and Rx XON character
   dcb.XoffChar = 1;                      // Tx and Rx XOFF character
   dcb.ErrorChar = 0;                     // error replacement character
   dcb.EofChar = 0;                       // end of input character
   dcb.EvtChar = 0;                       // received event character

   fRetVal = SetCommState(ComID[Prt], &dcb);

   return (fRetVal);
}


//---------------------------------------------------------------------------
//  Description:
//     Closes the connection to the port.
//     - Use new PurgeComm() API to clear communications driver before
//       closing device.
//
void CloseCOM(int Prt)
{
   // disable event notification and wait for thread
   // to halt
   SetCommMask(ComID[Prt], 0);

   // purge any outstanding reads/writes and close device handle
   PurgeComm(ComID[Prt], PURGE_TXABORT | PURGE_RXABORT |
                         PURGE_TXCLEAR | PURGE_RXCLEAR );
   CloseHandle(ComID[Prt]);

   ComID[Prt] = 0;

}

//---------------------------------------------------------------------------
//  Description:
//     Closes the connection to the port
//     - Use new PurgeComm() API to clear communications driver before
//       closing device.
//
int FreeCOM(short Prt)
{
   CloseCOM(Prt);

   return TRUE;
}

//---------------------------------------------------------------------------
//  Description:
//     flush the rx and tx buffers
//
void FlushCOM(int Prt)
{
   // purge any information in the buffer
   PurgeComm(ComID[Prt], PURGE_TXABORT | PURGE_RXABORT |
                         PURGE_TXCLEAR | PURGE_RXCLEAR );
}


//---------------------------------------------------------------------------
// Write an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set and the buffers have
// been flushed.
//
// Returns 1 for success and 0 for failure
//
//IBAPI WriteCOM(int Prt, int outlen, uchar IBAPI *outbuf)
int WriteCOM(int Prt, int outlen, uchar *outbuf)
{
   BOOL fWriteStat;
   DWORD dwBytesWritten=0;
   DWORD ler=0,to;
   COMMTIMEOUTS CommTimeOuts;

   // check if not using default timeout
   if (timeout > 0)
   {
      CommTimeOuts.ReadIntervalTimeout = DEF_READ_INT;
      CommTimeOuts.ReadTotalTimeoutMultiplier = DEF_READ_MULT;
      CommTimeOuts.ReadTotalTimeoutConstant = DEF_READ_CONST;
      CommTimeOuts.WriteTotalTimeoutMultiplier = 1;
      CommTimeOuts.WriteTotalTimeoutConstant = timeout;
      SetCommTimeouts(ComID[Prt], &CommTimeOuts);
      to = timeout + outlen + 5;
   }
   // calculate a timeout
   else
      to = DEF_WRITE_MULT * outlen + DEF_WRITE_CONST + 5;

   // write the byte
   fWriteStat = WriteFile(ComID[Prt], (LPSTR) outbuf, outlen,
                          &dwBytesWritten, NULL );

   // check for an error
   if (!fWriteStat)
      ler = GetLastError();

   // check if need to restore timeouts
   if (timeout > 0)
   {
      CommTimeOuts.ReadIntervalTimeout = DEF_READ_INT;
      CommTimeOuts.ReadTotalTimeoutMultiplier = DEF_READ_MULT;
      CommTimeOuts.ReadTotalTimeoutConstant = DEF_READ_CONST;
      CommTimeOuts.WriteTotalTimeoutMultiplier = DEF_WRITE_MULT;
      CommTimeOuts.WriteTotalTimeoutConstant = DEF_WRITE_CONST;
      SetCommTimeouts(ComID[Prt], &CommTimeOuts);
   }

   // check results of write
   if (!fWriteStat || (dwBytesWritten != (unsigned short)outlen))
      return 0;
   else
      return 1;
}


//---------------------------------------------------------------------------
// Read an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set and the buffers have
// been flushed.
//
// Returns number of characters read
//
int ReadCOM(int Prt, int inlen, uchar *inbuf)   //,ulong IBAPI *more
{
   DWORD dwLength=0, dwErrorFlags, temp_len=0;
   BOOL fReadStat;
   DWORD ler=0,to;
   COMMTIMEOUTS CommTimeOuts;
   COMSTAT ComStat;

   // check if not using default timeout
   if (timeout > 0)
   {
      CommTimeOuts.ReadIntervalTimeout = DEF_READ_INT;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 1;
      CommTimeOuts.ReadTotalTimeoutConstant = timeout;
      CommTimeOuts.WriteTotalTimeoutMultiplier = DEF_WRITE_MULT;
      CommTimeOuts.WriteTotalTimeoutConstant = DEF_WRITE_CONST;
      SetCommTimeouts(ComID[Prt], &CommTimeOuts);
      to = timeout + inlen + 5;
   }
   // calculate a timeout
   else
      to = DEF_READ_MULT * inlen + DEF_READ_CONST + 5;

   // small delay
   Sleep(0);//(3.11)

   // write the byte (string of bits)
   fReadStat = ReadFile(ComID[Prt], (LPSTR) inbuf, inlen, &dwLength,
                        NULL);

   // check for less read (3.11)
   if ((fReadStat == 1) && (dwLength != (unsigned short)inlen))
   {
      temp_len = dwLength;
      Sleep(10);
      fReadStat = ReadFile(ComID[Prt], (LPSTR)&inbuf[temp_len], inlen - temp_len, &dwLength,
                           NULL);
      dwLength += temp_len;
   }


   // check for an error
   if (!fReadStat)
      ler = GetLastError();

   // check if need to restore timeouts
   if (timeout > 0)
   {
      CommTimeOuts.ReadIntervalTimeout = DEF_READ_INT;
      CommTimeOuts.ReadTotalTimeoutMultiplier = DEF_READ_MULT;
      CommTimeOuts.ReadTotalTimeoutConstant = DEF_READ_CONST;
      CommTimeOuts.WriteTotalTimeoutMultiplier = DEF_WRITE_MULT;
      CommTimeOuts.WriteTotalTimeoutConstant = DEF_WRITE_CONST;
      SetCommTimeouts(ComID[Prt], &CommTimeOuts);
   }

   // check results
   if (fReadStat)
   {
      ClearCommError(ComID[Prt], &dwErrorFlags, &ComStat);
      return (short)dwLength;
   }
   else
      return 0;
}


//--------------------------------------------------------------------------
//  Description:
//     Send a break on the com port for len ms
//
//  len > 0 - do a break for len ms
//      = 0 - remove an infinite break
//      < 0 - set an infinite break
//
void BreakCOM(int Prt)  //, short len)
{

   short len = 2;
   int rt;

   // set a break?
   if (len != 0)
      // start the reset pulse
      if (SetCommBreak(ComID[Prt]) != 1) rt = FALSE;

    // special case for very small break times
   if ((len < 5) && (len > 0))
   {
      // get a time stamp
      MarkTime();
      // wait for len ms
      while (!ElapsedTime(len));
   }
   else if (len >= 5)
      // sleep
      Sleep(len);

   // is this not an infinite break?
   if (len >= 0)
      // clear the reset pulse
      if (ClearCommBreak(ComID[Prt]) != 1)
         rt = FALSE;

   rt = TRUE;
}


//--------------------------------------------------------------------------
//  Description:
//     Drop and then raise power (DTR,RTS) on the com port
//
int PowerCOM(short Prt, short len)
{
   DCB dcb;

   // drop power?
   if (len != 0)
   {
      // DTR/RTS off (inverse loader mode for the 5000)
      dcb.DCBlength = sizeof(DCB);
      if (GetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
      dcb.fDtrControl = DTR_CONTROL_DISABLE;  // disable DTR
      dcb.fRtsControl = RTS_CONTROL_DISABLE;
      if (SetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
   }

   // special case for very small break times
   if ((len < 5) && (len > 0))
   {
      // get a time stamp
      MarkTime();
      // wait for len ms
      while (!ElapsedTime(len));
   }
   else if (len >= 5)
      // sleep
      Sleep(len);

   // is this not an infinite power drop?
   if (len >= 0)
   {
      // DTR/RTS on
      dcb.DCBlength = sizeof(DCB);
      if (GetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
      dcb.fDtrControl = DTR_CONTROL_ENABLE;  // enable DTR
      dcb.fRtsControl = RTS_CONTROL_ENABLE;
      if (SetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
   }

   return TRUE;
}


//--------------------------------------------------------------------------
//  Description:
//     Drop and then raise power (DTR,RTS) on the com port
//
int RTSCOM(short Prt, short len)
{
   DCB dcb;

   // clear RTS?
   if (len != 0)
   {
      // DTR/RTS off (inverse loader mode for the 5000)
      dcb.DCBlength = sizeof(DCB);
      if (GetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
      dcb.fDtrControl = DTR_CONTROL_ENABLE;  // enable DTR
      dcb.fRtsControl = RTS_CONTROL_DISABLE;
      if (SetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
   }

   // special case for very small break times
   if ((len < 5) && (len > 0))
   {
      // get a time stamp
      MarkTime();
      // wait for len ms
      while (!ElapsedTime(len));
   }
   else if (len >= 5)
      // sleep
      Sleep(len);

   // is this not an infinite clear RTS?
   if (len >= 0)
   {
      // DTR/RTS on
      dcb.DCBlength = sizeof(DCB);
      if (GetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
      dcb.fRtsControl = RTS_CONTROL_ENABLE;
      if (SetCommState(ComID[Prt], &dcb) != 1)
         return FALSE;
   }

   return TRUE;
}


//--------------------------------------------------------------------------
//  Description:
//     Check to see if DSR is set.
//
int DSRHigh(short Prt)
{
   DCB dcb;
   uchar rbuf[] = { 0xFF, 0x00 };
   short rt = FALSE;

   // DSR flow control on
   dcb.DCBlength = sizeof(DCB);
   if (GetCommState(ComID[Prt], &dcb) != 1)
      return FALSE;
   dcb.fRtsControl = RTS_CONTROL_ENABLE;
   dcb.fOutxDsrFlow = TRUE;              // DSR output flow control
   if (SetCommState(ComID[Prt], &dcb) != 1)
      return FALSE;

   // if can write then DSR must be high
   if (WriteCOM(Prt,1,rbuf))
      rt = TRUE;

   // DTR/RTS on
   dcb.DCBlength = sizeof(DCB);
   if (GetCommState(ComID[Prt], &dcb) != 1)
      return FALSE;
   dcb.fOutxDsrFlow = FALSE;              // DSR output flow control
   if (SetCommState(ComID[Prt], &dcb) != 1)
      return FALSE;

   FlushCOM(Prt);

   return rt;
}

//--------------------------------------------------------------------------
// Mark time stamp for later comparison
//
void StdFunc MarkTime(void)
{
   TimeStamp = GetTickCount();
}


//--------------------------------------------------------------------------
// Check if timelimit number of ms have elapse from the call to MarkTime
// Return TRUE if the time has elapse else FALSE.
//
int ElapsedTime(long timelimit)
{
   return ((TimeStamp + timelimit) < GetTickCount());
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void msDelay(int len)
{
   Sleep(len);
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
   short fRetVal;
   DCB dcb;
   ulong baud;

   dcb.DCBlength = sizeof(DCB);

   GetCommState(ComID[portnum], &dcb);

   switch(new_baud)
	{
		case PARMSET_9600:
			baud = 9600;
			break;

		case PARMSET_19200:
			baud = 19200;
			break;

		case PARMSET_57600:
			baud = 57600;
			break;

		case PARMSET_115200:
			baud = 115200;
			break;

		default:
			break;
	}

   dcb.BaudRate = baud;                      // current baud rate

   fRetVal = SetCommState(ComID[portnum], &dcb);
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   return GetTickCount();
}

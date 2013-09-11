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
//  TMEXLnk.C - Link module to call on TMEX low-level functions to
//              excersize the general 1-Wire Net functions.
//              (Requires TMEX 3.11 or newer)
//
//  Version: 3.00
//
//  History: 1.00 -> 1.01  Return values in owLevel corrected.
//                         Added function msDelay.
//           1.02 -> 1.03  Add msGettick, always return owLevel success
//                         to hide adapters (DS9097E) that do not have
//                         power delivery capabilities.
//           1.03 -> 2.00  Changed 'MLan' to 'ow'. Added support for
//                         multiple ports.
//           2.10 -> 3.00  Added owReadBitPower and owWriteBytePower

#include "ownet.h"
#include <windows.h>

// external TMEX variables
extern long SessionHandle[MAX_PORTNUM];
extern uchar StateBuffer[MAX_PORTNUM][5120];
extern short far pascal TMTouchByte(long, short);
extern short far pascal TMTouchReset(long);
extern short far pascal TMTouchBit(long, short);
extern short far pascal TMProgramPulse(long);
extern short far pascal TMOneWireCom(long, short, short);
extern short far pascal TMOneWireLevel(long, short, short, short);

// globals
SMALLINT FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = FALSE; // for compatibility purposes

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns: TRUE(1):  presense pulse(s) detected, device(s) reset
//          FALSE(0): no presense pulses detected
//
SMALLINT owTouchReset(int portnum)
{
   int result;

   // Assume valid Session
   result = TMTouchReset(SessionHandle[portnum]);

   // success if the normal or alarm presence
   if ((result == 1) || (result == 2))
      return TRUE;
   else
      return FALSE;
}


//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and return the
// result 1 bit read from the 1-Wire Net.  The parameter 'sendbit'
// least significant bit is used and the least significant bit
// of the result is the return bit.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'sendbit'     - the least significant bit is the bit to send
//
// Returns: 0:   0 bit read from sendbit
//          1:   1 bit read from sendbit
//
SMALLINT owTouchBit(int portnum, SMALLINT sendbit)
{
   // Assume valid Session
   return TMTouchBit(SessionHandle[portnum],(short)sendbit);
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same
//
SMALLINT owTouchByte(int portnum, SMALLINT sendbyte)
{
   // Assume valid Session
   return TMTouchByte(SessionHandle[portnum],(short)sendbyte);
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the MicroLAN and verify that the
// 8 bits read from the MicroLAN is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same
//
SMALLINT owWriteByte(int portnum, SMALLINT sendbyte)
{
   return (owTouchByte(portnum,sendbyte) == sendbyte) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------
// Send 8 bits of read communication to the 1-Wire Net and and return the
// result 8 bits read from the 1-Wire Net.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:  TRUE:  8 bytes read from 1-Wire Net
//           FALSE: the 8 bytes were not read
//
SMALLINT owReadByte(int portnum)
{
   return owTouchByte(portnum,0xFF);
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net communucation speed.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'new_speed'  - new speed defined as
//                MODE_NORMAL     0x00
//                MODE_OVERDRIVE  0x01
//
// Returns:  current 1-Wire Net speed
//
SMALLINT owSpeed(int portnum, SMALLINT new_speed)
{
   return TMOneWireCom(SessionHandle[portnum],0,(short)new_speed);
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net line level.  The values for new_level are
// as follows:
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number is provided to
//               indicate the symbolic port number.
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
// Returns:  current 1-Wire Net level
//
SMALLINT owLevel(int portnum, SMALLINT new_level)
{
   int rslt;
   int docheck = FALSE;

   // check for DS2480 bug
   if (((SessionHandle[portnum] & 0x0F0) == 0x050) &&
       (TMOneWireLevel(SessionHandle[portnum],1,0,0) == 1))
      docheck = TRUE;

   switch (new_level)
   {
      case MODE_NORMAL:
         rslt = TMOneWireLevel(SessionHandle[portnum],0,0,0);
         // test code for DS2480 bug
         if (docheck)
            TMTouchBit(SessionHandle[portnum],1);
         break;
      case MODE_STRONG5:
         rslt = TMOneWireLevel(SessionHandle[portnum],0,1,0);
         break;
      case MODE_PROGRAM:
         rslt = TMOneWireLevel(SessionHandle[portnum],0,3,0);
         break;
      case MODE_BREAK:
         rslt = TMOneWireLevel(SessionHandle[portnum],0,2,0);
         break;
      default:
         rslt = 0;
   }

   // Assume TMEX can do it so always return NewLevel
   return new_level;
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:  TRUE  successful
//           FALSE program voltage not available
//
SMALLINT owProgramPulse(int portnum)
{
   return TMProgramPulse(SessionHandle[portnum]);
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
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   return GetTickCount();
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same 
//
SMALLINT owWriteBytePower(int portnum, SMALLINT sendbyte)
{
   // prime for power delivery after byte
   TMOneWireLevel(SessionHandle[portnum],0,1,2);

   // send the byte and start strong pullup
   if(TMTouchByte(SessionHandle[portnum],(short)sendbyte) != sendbyte)
   {
      TMOneWireLevel(SessionHandle[portnum],0,0,0);
      return FALSE;
   }

   return TRUE;
}

//--------------------------------------------------------------------------
// Read 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are read change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same 
//
SMALLINT owReadBytePower(int portnum)
{
   SMALLINT sendbyte = 0xFF;

   // prime for power delivery after byte
   TMOneWireLevel(SessionHandle[portnum],0,1,2);

   // send the byte and start strong pullup
   return TMTouchByte(SessionHandle[portnum],(short)sendbyte);
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// response matches the 'applyPowerResponse' bit and apply power delivery
// to the 1-Wire net.  Note that some implementations may apply the power
// first and then turn it off if the response is incorrect.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'applyPowerResponse' - 1 bit response to check, if correct then start
//                        power delivery 
//
// Returns:  TRUE: bit written and response correct, strong pullup now on
//           FALSE: response incorrect
//
SMALLINT owReadBitPower(int portnum, SMALLINT applyPowerResponse)
{
   // prime for power delivery after bit
   TMOneWireLevel(SessionHandle[portnum],0,1,1);

   // send the byte and start strong pullup
   if(TMTouchBit(SessionHandle[portnum],0x01) != applyPowerResponse)
   {
      TMOneWireLevel(SessionHandle[portnum],0,0,0);
      return FALSE;
   }

   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive. 
//
SMALLINT owHasPowerDelivery(int portnum)
{
   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive. 
//
SMALLINT owHasOverDrive(int portnum)
{
   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse 
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  program volatage available
//           FALSE program voltage not available  
SMALLINT owHasProgramPulse(int portnum)
{
   return TMProgramPulse(SessionHandle[portnum]);
}
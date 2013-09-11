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
//  multilnk.C - Wrapper to hook all adapter types in the 1-Wire Public 
//               Domain API for link functions.
//
//  Version: 3.00
//

#include "mownet.h"

// If TRUE, puts a delay in owTouchReset to compensate for alarming clocks.
SMALLINT FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = FALSE; // default owTouchReset to quickest response. 

extern SMALLINT default_type; 
extern SMALLINT LFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
extern SMALLINT SFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
extern SMALLINT UFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;

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
   // check global flag does not match specific adapters
   if (FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE != 
       UFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE)
   {
      UFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = 
         FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
      SFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = 
         FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
      LFAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = 
         FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE;
   }

   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owTouchReset_DS9490(portnum & 0xFF);
      case DS1410E: return owTouchReset_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owTouchReset_DS9097U(portnum & 0xFF);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owTouchBit_DS9490(portnum & 0xFF, sendbit);
      case DS1410E: return owTouchBit_DS1410E(portnum & 0xFF, sendbit);
      default: 
      case DS9097U: return owTouchBit_DS9097U(portnum & 0xFF, sendbit);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owTouchByte_DS9490(portnum & 0xFF, sendbyte);
      case DS1410E: return owTouchByte_DS1410E(portnum & 0xFF, sendbyte);
      default: 
      case DS9097U: return owTouchByte_DS9097U(portnum & 0xFF, sendbyte);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owSpeed_DS9490(portnum & 0xFF, new_speed);
      case DS1410E: return owSpeed_DS1410E(portnum & 0xFF, new_speed);
      default: 
      case DS9097U: return owSpeed_DS9097U(portnum & 0xFF, new_speed);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owLevel_DS9490(portnum & 0xFF, new_level);
      case DS1410E: return owLevel_DS1410E(portnum & 0xFF, new_level);
      default: 
      case DS9097U: return owLevel_DS9097U(portnum & 0xFF, new_level);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owProgramPulse_DS9490(portnum & 0xFF);
      case DS1410E: return owProgramPulse_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owProgramPulse_DS9097U(portnum & 0xFF);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owWriteBytePower_DS9490(portnum & 0xFF, sendbyte);
      case DS1410E: return owWriteBytePower_DS1410E(portnum & 0xFF, sendbyte);
      default: 
      case DS9097U: return owWriteBytePower_DS9097U(portnum & 0xFF, sendbyte);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owReadBitPower_DS9490(portnum & 0xFF, applyPowerResponse);
      case DS1410E: return owReadBitPower_DS1410E(portnum & 0xFF, applyPowerResponse);
      default: 
      case DS9097U: return owReadBitPower_DS9097U(portnum & 0xFF, applyPowerResponse);
   };
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant bit)
//
// Returns:  TRUE: bytes written and echo was the same, strong pullup now on
//           FALSE: echo was not the same 
//
SMALLINT owReadBytePower(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owReadBytePower_DS9490(portnum & 0xFF);
      case DS1410E: return owReadBytePower_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owReadBytePower_DS9097U(portnum & 0xFF);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owHasPowerDelivery_DS9490(portnum & 0xFF);
      case DS1410E: return owHasPowerDelivery_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owHasPowerDelivery_DS9097U(portnum & 0xFF);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owHasOverDrive_DS9490(portnum & 0xFF);
      case DS1410E: return owHasOverDrive_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owHasOverDrive_DS9097U(portnum & 0xFF);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owHasProgramPulse_DS9490(portnum & 0xFF);
      case DS1410E: return owHasProgramPulse_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owHasProgramPulse_DS9097U(portnum & 0xFF);
   };
}
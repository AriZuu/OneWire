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
//  usbwlnk.c - USB (DS2490) 1-Wire Public Domain low-level functions 
//             (Requires DS2490.SYS)
//
//  Version: 3.00
//

#include "ownet.h"
#include "ds2490.h"

// globals
// flag for DS1994/DS2404 support
SMALLINT FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = TRUE; 

// DS2490 state info
extern SMALLINT USBSpeed[MAX_PORTNUM]; 
extern SMALLINT USBLevel[MAX_PORTNUM]; 
extern HANDLE usbhnd[MAX_PORTNUM];
extern SMALLINT USBVpp[MAX_PORTNUM]; 

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
   SETUP_PACKET setup;
   ULONG nOutput = 0;
   SMALLINT present,vpp;

   // make sure strong pullup is not on
   if (USBLevel[portnum] == MODE_STRONG5)
      owLevel(portnum, MODE_NORMAL);

   // construct command
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_1_WIRE_RESET | COMM_F | COMM_IM | COMM_SE;
   setup.Index = (USBSpeed[portnum] == MODE_OVERDRIVE) ?
                  ONEWIREBUSSPEED_OVERDRIVE : ONEWIREBUSSPEED_FLEXIBLE;
   setup.Length = 0;
   setup.DataOut = FALSE;
   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_RESET_FAILED);
      AdapterRecover(portnum);
      return FALSE;
   }
   else
   {
      // extra delay for alarming DS1994/DS2404 complience
      if ((FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE) && (USBSpeed[portnum] != MODE_OVERDRIVE))
         Sleep(5);

      // success, check for shorts
      if (DS2490ShortCheck(usbhnd[portnum], &present,&vpp))
      {
         USBVpp[portnum] = vpp;     
         return present;
      }
      else
      {
         OWERROR(OWERROR_OW_SHORTED);
         // short occuring
         msDelay(300);
         AdapterRecover(portnum);
         return FALSE;
      }
   }
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
   SETUP_PACKET setup;
   ULONG nOutput = 0;
   WORD nBytes;   
   BYTE buf[2];

   // make sure strong pullup is not on
   if (USBLevel[portnum] == MODE_STRONG5)
      owLevel(portnum, MODE_NORMAL);

   // set to do touchbit
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_BIT_IO | COMM_IM | ((sendbit) ? COMM_D : 0);
   setup.Index = 0;  
   setup.Length = 0;
   setup.DataOut = FALSE;

   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return 0;
   }
   else
   {
      // success, read the result
      nBytes = 1;
      if (DS2490Read(usbhnd[portnum], buf, &nBytes))
         return buf[0];
      else
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return 0;
      }
   }
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
   SETUP_PACKET setup;
   ULONG nOutput = 0;
   WORD nBytes;   
   BYTE buf[2];

   // make sure strong pullup is not on
   if (USBLevel[portnum] == MODE_STRONG5)
      owLevel(portnum, MODE_NORMAL);

   // set to do touchbyte
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_BYTE_IO | COMM_IM;
   setup.Index = sendbyte & 0xFF;  
   setup.Length = 0;
   setup.DataOut = FALSE;

   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return 0;
   }
   else
   {
      // success, read the result
      nBytes = 1;
      if (DS2490Read(usbhnd[portnum], buf, &nBytes))
         return buf[0];
      else
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return 0;
      }
   }
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
   SETUP_PACKET setup;
   ULONG nOutput = 0;

   // set to change the speed
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = MODE_CMD;
   setup.Value = MOD_1WIRE_SPEED;
   setup.Index = ((new_speed == MODE_OVERDRIVE) ? ONEWIREBUSSPEED_OVERDRIVE : ONEWIREBUSSPEED_FLEXIBLE) & 0x00FF;
   setup.Length = 0x00;
   setup.DataOut = FALSE;
   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return USBSpeed[portnum];
   }
   else
   {
      // success, read the result
      USBSpeed[portnum] = new_speed;
      return new_speed;
   }
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
//                MODE_PROGRAM    0x04 (not supported in this version)
//                MODE_BREAK      0x08 (not supported in chip)
//
// Returns:  current 1-Wire Net level
//
SMALLINT owLevel(int portnum, SMALLINT new_level)
{
   SETUP_PACKET setup;
   ULONG nOutput = 0;

   // Turn off infinite strong pullup?
   if ((new_level == MODE_NORMAL) && (USBLevel[portnum] == MODE_STRONG5))
   {
      if (DS2490HaltPulse(usbhnd[portnum]))
         USBLevel[portnum] = MODE_NORMAL;  
   }
   // Turn on infinite strong5 pullup?
   else if ((new_level == MODE_STRONG5) && (USBLevel[portnum] == MODE_NORMAL))
   {
      // assume duration set to infinite during setup of device
      // enable the pulse
      setup.RequestTypeReservedBits = 0x40;
      setup.Request = MODE_CMD;
      setup.Value = MOD_PULSE_EN;
      setup.Index = ENABLEPULSE_SPUE;
      setup.Length = 0x00;
      setup.DataOut = FALSE;
      // call the driver
      if (!DeviceIoControl(usbhnd[portnum],
					       DS2490_IOCTL_VENDOR,
					       &setup,
					       sizeof(SETUP_PACKET),
					       NULL,
					       0,
					       &nOutput,
					       NULL))
      {
         // failure
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return USBLevel[portnum];
      }

      // start the pulse
      setup.RequestTypeReservedBits = 0x40;
      setup.Request = COMM_CMD;
      setup.Value = COMM_PULSE | COMM_IM;
      setup.Index = 0;
      setup.Length = 0;
      setup.DataOut = FALSE;
      // call the driver
      if (!DeviceIoControl(usbhnd[portnum],
					       DS2490_IOCTL_VENDOR,
					       &setup,
					       sizeof(SETUP_PACKET),
					       NULL,
					       0,
					       &nOutput,
					       NULL))
      {
         // failure
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return USBLevel[portnum];
      }
      else
      {
         // success, read the result
         USBLevel[portnum] = new_level;
         return new_level;
      }
   }
   // unsupported
   else if (new_level != USBLevel[portnum])
   {
      OWERROR(OWERROR_FUNC_NOT_SUP);
      return USBLevel[portnum];
   }

   // success, return the current level 
   return USBLevel[portnum];
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
   SETUP_PACKET setup;
   ULONG nOutput = 0;

   // check if Vpp available
   if (!USBVpp[portnum])
      return FALSE;

   // make sure strong pullup is not on
   if (USBLevel[portnum] == MODE_STRONG5)
      owLevel(portnum, MODE_NORMAL);

   // send the pulse (already enabled)
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_PULSE | COMM_IM | COMM_TYPE;
   setup.Index = 0;
   setup.Length = 0;
   setup.DataOut = FALSE;
   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return FALSE;
   }
   else
      return TRUE;
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
// Send and recieve 8 bits of communication to the 1-Wire Net.   
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net to strong pullup 
// power delivery.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  Byte read: byte read (or echo if write)
//           0: failure 
//
SMALLINT owTouchBytePower(int portnum, SMALLINT sendbyte)
{
   SETUP_PACKET setup;
   ULONG nOutput = 0;
   WORD nBytes;   
   BYTE buf[2];

   // make sure strong pullup is not on
   if (USBLevel[portnum] == MODE_STRONG5)
      owLevel(portnum, MODE_NORMAL);

   // enable the strong pullup pulse
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = MODE_CMD;
   setup.Value = MOD_PULSE_EN;
   setup.Index = ENABLEPULSE_SPUE;
   setup.Length = 0x00;
   setup.DataOut = FALSE;
   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return FALSE;
   }
   
   // set to do touchbyte with the SPU immediatly after
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_BYTE_IO | COMM_IM | COMM_SPU;
   setup.Index = sendbyte & 0xFF;  
   setup.Length = 0;
   setup.DataOut = FALSE;

   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return FALSE;
   }
   else
   {
      // now strong pullup is enabled
      USBLevel[portnum] = MODE_STRONG5;
      // success, read the result
      nBytes = 1;
      if (DS2490Read(usbhnd[portnum], buf, &nBytes))
         return buf[0];
      else
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return FALSE;
      }
   }
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire net and verify that the
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
   return (owTouchBytePower(portnum,sendbyte) == sendbyte);
}

//--------------------------------------------------------------------------
// Read 8 bits of communication from the 1-Wire net and provide strong
// pullup power.  
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  byte read
//
SMALLINT owReadBytePower(int portnum)
{
   return owTouchBytePower(portnum,0xFF);
}

//--------------------------------------------------------------------------
// Read 1 bit of communication from the 1-Wire net and verify that the
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
   SETUP_PACKET setup;
   ULONG nOutput = 0;
   WORD nBytes;   
   BYTE buf[2];

   // make sure strong pullup is not on
   if (USBLevel[portnum] == MODE_STRONG5)
      owLevel(portnum, MODE_NORMAL);

   // enable the strong pullup pulse
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = MODE_CMD;
   setup.Value = MOD_PULSE_EN;
   setup.Index = ENABLEPULSE_SPUE;
   setup.Length = 0x00;
   setup.DataOut = FALSE;
   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return FALSE;
   }

   // set to do touchbit
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_BIT_IO | COMM_IM | COMM_SPU | COMM_D;
   setup.Index = 0;  
   setup.Length = 0;
   setup.DataOut = FALSE;

   // call the driver
   if (!DeviceIoControl(usbhnd[portnum],
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL))
   {
      // failure
      OWERROR(OWERROR_ADAPTER_ERROR);
      AdapterRecover(portnum);
      return FALSE;
   }
   else
   {
      // now strong pullup is enabled
      USBLevel[portnum] = MODE_STRONG5;
      // success, read the result
      nBytes = 1;
      if (DS2490Read(usbhnd[portnum], buf, &nBytes))
      {
         // check response 
         if (buf[0] != applyPowerResponse)
         {
            owLevel(portnum, MODE_NORMAL);
            return FALSE;
         }
         else
            return TRUE;
      }
      else
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return FALSE;
      }
   }
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
//
SMALLINT owHasProgramPulse(int portnum)
{
   owTouchReset(portnum);
   return USBVpp[portnum];
}

//--------------------------------------------------------------------------
// Attempt to recover communication with the DS2490
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              indicate the port number.
//
// Returns:  TRUE  DS2490 recover successfull
//           FALSE failed to recover
//
SMALLINT AdapterRecover(int portnum)
{
   // dectect DS2490 and set it up
   if (DS2490Detect(usbhnd[portnum]))
   {
      USBSpeed[portnum] = MODE_NORMAL; // current DS2490 1-Wire Net communication speed
      USBLevel[portnum] = MODE_NORMAL; // current DS2490 1-Wire Net level
      return TRUE;
   }
   else
   {
      OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
      return FALSE;
   }
}


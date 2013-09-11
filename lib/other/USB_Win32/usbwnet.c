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
//  usbwnet.C - USB (DS2490) 1-Wire Public Domain API for network functions.
//
//  Version: 3.00
//
//

#include "ownet.h"
#include "ds2490.h"
#include <windows.h>


// local functions defined in ownetu.c
static SMALLINT bitacc(SMALLINT,SMALLINT,SMALLINT,uchar *);

// external 
extern HANDLE usbhnd[MAX_PORTNUM];

// global variables for this module to hold search state information
static int LastDiscrepancy[MAX_PORTNUM];
static int LastFamilyDiscrepancy[MAX_PORTNUM];
static uchar LastDevice[MAX_PORTNUM];
uchar SerialNum[MAX_PORTNUM][8];

//--------------------------------------------------------------------------
// The 'owFirst' finds the first device on the 1-Wire Net  This function
// contains one parameter 'alarm_only'.  When
// 'alarm_only' is TRUE (1) the find alarm command 0xEC is
// sent instead of the normal search command 0xF0.
// Using the find alarm command 0xEC will limit the search to only
// 1-Wire devices that are in an 'alarm' state.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'do_reset'   - TRUE (1) perform reset before search, FALSE (0) do not
//                perform reset before search.
// 'alarm_only' - TRUE (1) the find alarm command 0xEC is
//                sent instead of the normal search command 0xF0
//
// Returns:   TRUE (1) : when a 1-Wire device was found and it's
//                        Serial Number placed in the global SerialNum[portnum]
//            FALSE (0): There are no devices on the 1-Wire Net.
//
SMALLINT owFirst(int portnum, SMALLINT do_reset, SMALLINT alarm_only)
{
   // reset the search state
   LastDiscrepancy[portnum] = 0;
   LastDevice[portnum] = FALSE;
   LastFamilyDiscrepancy[portnum] = 0;

   return owNext(portnum, do_reset, alarm_only);
}

//--------------------------------------------------------------------------
// The 'owNext' function does a general search.  This function
// continues from the previos search state. The search state
// can be reset by using the 'owFirst' function.
// This function contains one parameter 'alarm_only'.
// When 'alarm_only' is TRUE (1) the find alarm command
// 0xEC is sent instead of the normal search command 0xF0.
// Using the find alarm command 0xEC will limit the search to only
// 1-Wire devices that are in an 'alarm' state.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'do_reset'   - TRUE (1) perform reset before search, FALSE (0) do not
//                perform reset before search.
// 'alarm_only' - TRUE (1) the find alarm command 0xEC is
//                sent instead of the normal search command 0xF0
//
// Returns:   TRUE (1) : when a 1-Wire device was found and it's
//                       Serial Number placed in the global SerialNum[portnum]
//            FALSE (0): when no new device was found.  Either the
//                       last search was the last device or there
//                       are no devices on the 1-Wire Net.
//
SMALLINT owNext(int portnum, SMALLINT do_reset, SMALLINT alarm_only)
{
   SETUP_PACKET setup;
   short rt=FALSE,i,ResetSearch=FALSE;
   BYTE rom_buf[16];
   BYTE ret_buf[16];
   WORD buf_len;
   uchar lastcrc8;
   WORD  nBytes = 8;
   ULONG nOutput = 0;
   long limit;
   STATUS_PACKET status;
   BYTE  nResult;

   // if the last call was the last one
   if (LastDevice[portnum])
   {
      // reset the search
      LastDiscrepancy[portnum] = 0;
      LastDevice[portnum] = FALSE;
      LastFamilyDiscrepancy[portnum] = 0;
      return FALSE;
   }

   // check if reset first is requested
   if (do_reset)
   {
      // reset the 1-wire
      // extra reset if last part was a DS1994/DS2404 (due to alarm)
      if ((SerialNum[portnum][0] & 0x7F) == 0x04)
         owTouchReset(portnum);
      // if there are no parts on 1-wire, return FALSE
      if (!owTouchReset(portnum))
      {
         // reset the search
         LastDiscrepancy[portnum] = 0;
         LastFamilyDiscrepancy[portnum] = 0;
         OWERROR(OWERROR_NO_DEVICES_ON_NET);
         return FALSE;
      }
   }

   // build the rom number to put to the USB chip
   for (i = 0; i < 8; i++)
      rom_buf[i] = SerialNum[portnum][i];

   // take into account LastDiscrepancy
   if (LastDiscrepancy[portnum] != 0xFF)
   {
      if (LastDiscrepancy[portnum] > 0)
         bitacc(WRITE_FUNCTION,1,(short)(LastDiscrepancy[portnum] - 1),rom_buf);

      for (i = LastDiscrepancy[portnum]; i < 64; i++)
         bitacc(WRITE_FUNCTION,0,i,rom_buf);
   }

   // put the ROM ID in EP2
   nBytes = 8;
   if (!DS2490Write(usbhnd[portnum], rom_buf, &nBytes))
   {
      AdapterRecover(portnum);
      return FALSE;
   }

   // setup for search command call
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_SEARCH_ACCESS | COMM_IM | COMM_SM | COMM_F | COMM_RTS; 
   // the number of devices to read (1) with the search command
   setup.Index = 0x0100 | (((alarm_only) ? 0xEC : 0xF0) & 0x00FF);  
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
      AdapterRecover(portnum);
      return FALSE;
   }

   // set a time limit
   limit = msGettick() + 200;
   // loop to wait on EP3 ready (device will go idle)   
   do
   {
      // read the status to see if the pulse has been stopped
      if (!DS2490GetStatus(usbhnd[portnum], &status, &nResult))
      {
         // failure
         break;
      }
      else
      {
	      // look for any fail conditions
	      for (i = 0; i < nResult; i++)
	      {
	        // only check for error conditions when the condition is not a ONEWIREDEVICEDETECT
	        if (status.CommResultCodes[i] != ONEWIREDEVICEDETECT)
	        {
               // failure
               break;
           }
         }
      }
   }
   while (((status.StatusFlags & STATUSFLAGS_IDLE) == 0) && 
          (limit > msGettick()));

   // check results of the waite for idel
   if ((status.StatusFlags & STATUSFLAGS_IDLE) == 0) 
   {
      AdapterRecover(portnum);
      return FALSE;
   }

   // check for data 
   if (status.ReadBufferStatus > 0)
   {
      // read the load 
      buf_len = 16;
	   if(!DS2490Read(usbhnd[portnum], ret_buf, &buf_len))
      {
         AdapterRecover(portnum);
		   return FALSE;
      }

      // success, get rom and desrepancy
      LastDevice[portnum] = (buf_len == 8); 
      // extract the ROM (and check crc)
      setcrc8(portnum,0);
      for (i = 0; i < 8; i++)
      {
         SerialNum[portnum][i] = ret_buf[i];
         lastcrc8 = docrc8(portnum,ret_buf[i]);
      }
      // crc OK and family code is not 0
      if (!lastcrc8 && SerialNum[portnum][0])
      {
         // loop through the descrepancy to get the pointers
         for (i = 0; i < 64; i++)
         {
            // if descrepancy
            if (bitacc(READ_FUNCTION,0,i,&ret_buf[8]) &&
                (bitacc(READ_FUNCTION,0,i,&ret_buf[0]) == 0))
            {
               LastDiscrepancy[portnum] = i + 1;  
            }
         }
         rt = TRUE;
      }
      else
      {
         ResetSearch = TRUE;
         rt = FALSE;
      }
   }

   // check if need to reset search
   if (ResetSearch)
   {
      LastDiscrepancy[portnum] = 0xFF;
      LastDevice[portnum] = FALSE;
      for (i = 0; i < 8; i++)
         SerialNum[portnum][i] = 0;
   }

   return rt;
}

//--------------------------------------------------------------------------
// The 'owSerialNum' function either reads or sets the SerialNum buffer
// that is used in the search functions 'owFirst' and 'owNext'.
// This function contains two parameters, 'serialnum_buf' is a pointer
// to a buffer provided by the caller.  'serialnum_buf' should point to
// an array of 8 unsigned chars.  The second parameter is a flag called
// 'do_read' that is TRUE (1) if the operation is to read and FALSE
// (0) if the operation is to set the internal SerialNum buffer from
// the data in the provided buffer.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'serialnum_buf' - buffer to that contains the serial number to set
//                   when do_read = FALSE (0) and buffer to get the serial
//                   number when do_read = TRUE (1).
// 'do_read'       - flag to indicate reading (1) or setting (0) the current
//                   serial number.
//
void owSerialNum(int portnum, uchar *serialnum_buf, SMALLINT do_read)
{
   uchar i;

   // read the internal buffer and place in 'serialnum_buf'
   if (do_read)
   {
      for (i = 0; i < 8; i++)
         serialnum_buf[i] = SerialNum[portnum][i];
   }
   // set the internal buffer from the data in 'serialnum_buf'
   else
   {
      for (i = 0; i < 8; i++)
         SerialNum[portnum][i] = serialnum_buf[i];
   }
}

//--------------------------------------------------------------------------
// Setup the search algorithm to find a certain family of devices
// the next time a search function is called 'owNext'.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number was provided to
//                   OpenCOM to indicate the port number.
// 'search_family' - family code type to set the search algorithm to find
//                   next.
//
void owFamilySearchSetup(int portnum, SMALLINT search_family)
{
   uchar i;

   // set the search state to find search_family type devices
   SerialNum[portnum][0] = search_family;
   for (i = 1; i < 8; i++)
      SerialNum[portnum][i] = 0;
   LastDiscrepancy[portnum] = 64;
   LastFamilyDiscrepancy[portnum] = 0;
   LastDevice[portnum] = FALSE;
}

//--------------------------------------------------------------------------
// Set the current search state to skip the current family code.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
void owSkipFamily(int portnum)
{
   // set the Last discrepancy to last family discrepancy
   LastDiscrepancy[portnum] = LastFamilyDiscrepancy[portnum];

   // clear the last family discrpepancy
   LastFamilyDiscrepancy[portnum] = 0;

   // check for end of list
   if (LastDiscrepancy[portnum] == 0)
      LastDevice[portnum] = TRUE;
}

//--------------------------------------------------------------------------
// The 'owAccess' function resets the 1-Wire and sends a MATCH Serial
// Number command followed by the current SerialNum code. After this
// function is complete the 1-Wire device is ready to accept device-specific
// commands.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:   TRUE (1) : reset indicates present and device is ready
//                       for commands.
//            FALSE (0): reset does not indicate presence or echos 'writes'
//                       are not correct.
//
SMALLINT owAccess(int portnum)
{
   uchar sendpacket[9];
   uchar i;

   // reset the 1-wire
   if (owTouchReset(portnum))
   {
      // create a buffer to use with block function
      // match Serial Number command 0x55
      sendpacket[0] = 0x55;
      // Serial Number
      for (i = 1; i < 9; i++)
         sendpacket[i] = SerialNum[portnum][i-1];

      // send/recieve the transfer buffer
      if (owBlock(portnum,FALSE,sendpacket,9))
      {
         // verify that the echo of the writes was correct
         for (i = 1; i < 9; i++)
            if (sendpacket[i] != SerialNum[portnum][i-1])
               return FALSE;
         if (sendpacket[0] != 0x55)
         {
            OWERROR(OWERROR_WRITE_VERIFY_FAILED);
            return FALSE;
         }
         else
            return TRUE;
      }
      else
         OWERROR(OWERROR_BLOCK_FAILED);
   }
   else
      OWERROR(OWERROR_NO_DEVICES_ON_NET);

   // reset or match echo failed
   return FALSE;
}

//----------------------------------------------------------------------
// The function 'owVerify' verifies that the current device
// is in contact with the 1-Wire Net.
// Using the find alarm command 0xEC will verify that the device
// is in contact with the 1-Wire Net and is in an 'alarm' state.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'alarm_only'  - TRUE (1) the find alarm command 0xEC
//                 is sent instead of the normal search
//                 command 0xF0.
//
// Returns:   TRUE (1) : when the 1-Wire device was verified
//                       to be on the 1-Wire Net
//                       with alarm_only == FALSE
//                       or verified to be on the 1-Wire Net
//                       AND in an alarm state when
//                       alarm_only == TRUE.
//            FALSE (0): the 1-Wire device was not on the
//                       1-Wire Net or if alarm_only
//                       == TRUE, the device may be on the
//                       1-Wire Net but in a non-alarm state.
//
SMALLINT owVerify(int portnum, SMALLINT alarm_only)
{
   uchar i,sendlen=0,goodbits=0,cnt=0,s,tst;
   uchar sendpacket[50];

   // construct the search rom
   if (alarm_only)
      sendpacket[sendlen++] = 0xEC; // issue the alarming search command
   else
      sendpacket[sendlen++] = 0xF0; // issue the search command
   // set all bits at first
   for (i = 1; i <= 24; i++)
      sendpacket[sendlen++] = 0xFF;
   // now set or clear apropriate bits for search
   for (i = 0; i < 64; i++)
      bitacc(WRITE_FUNCTION,bitacc(READ_FUNCTION,0,i,&SerialNum[portnum][0]),(int)((i+1)*3-1),&sendpacket[1]);

   // send/recieve the transfer buffer
   if (owBlock(portnum,TRUE,sendpacket,sendlen))
   {
      // check results to see if it was a success
      for (i = 0; i < 192; i += 3)
      {
         tst = (bitacc(READ_FUNCTION,0,i,&sendpacket[1]) << 1) |
                bitacc(READ_FUNCTION,0,(int)(i+1),&sendpacket[1]);

         s = bitacc(READ_FUNCTION,0,cnt++,&SerialNum[portnum][0]);

         if (tst == 0x03)  // no device on line
         {
              goodbits = 0;    // number of good bits set to zero
              break;     // quit
         }

         if (((s == 0x01) && (tst == 0x02)) ||
             ((s == 0x00) && (tst == 0x01))    )  // correct bit
            goodbits++;  // count as a good bit
      }

      // check too see if there were enough good bits to be successful
      if (goodbits >= 8)
         return TRUE;
   }
   else
      OWERROR(OWERROR_BLOCK_FAILED);

   // block fail or device not present
   return FALSE;
}

//----------------------------------------------------------------------
// Perform a overdrive MATCH command to select the 1-Wire device with
// the address in the ID data register.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:  TRUE: If the device is present on the 1-Wire Net and
//                 can do overdrive then the device is selected.
//           FALSE: Device is not present or not capable of overdrive.
//
//  *Note: This function could be converted to send DS2480
//         commands in one packet.
//
SMALLINT owOverdriveAccess(int portnum)
{
   uchar sendpacket[8];
   uchar i, bad_echo = FALSE;

   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   // force to normal communication speed
   owSpeed(portnum,MODE_NORMAL);

   // call the 1-Wire Net reset function
   if (owTouchReset(portnum))
   {
      // send the match command 0x69
      if (owWriteByte(portnum,0x69))
      {
         // switch to overdrive communication speed
         owSpeed(portnum,MODE_OVERDRIVE);

         // create a buffer to use with block function
         // Serial Number
         for (i = 0; i < 8; i++)
            sendpacket[i] = SerialNum[portnum][i];

         // send/recieve the transfer buffer
         if (owBlock(portnum,FALSE,sendpacket,8))
         {
            // verify that the echo of the writes was correct
            for (i = 0; i < 8; i++)
               if (sendpacket[i] != SerialNum[portnum][i])
                  bad_echo = TRUE;
            // if echo ok then success
            if (!bad_echo)
               return TRUE;
            else
               OWERROR(OWERROR_WRITE_VERIFY_FAILED);
         }
         else
            OWERROR(OWERROR_BLOCK_FAILED);
      }
      else
         OWERROR(OWERROR_WRITE_BYTE_FAILED);
   }
   else
      OWERROR(OWERROR_NO_DEVICES_ON_NET);

   // failure, force back to normal communication speed
   owSpeed(portnum,MODE_NORMAL);

   return FALSE;
}

//--------------------------------------------------------------------------
// Bit utility to read and write a bit in the buffer 'buf'.
//
// 'op'    - operation (1) to set and (0) to read
// 'state' - set (1) or clear (0) if operation is write (1)
// 'loc'   - bit number location to read or write
// 'buf'   - pointer to array of bytes that contains the bit
//           to read or write
//
// Returns: 1   if operation is set (1)
//          0/1 state of bit number 'loc' if operation is reading
//
static SMALLINT bitacc(SMALLINT op, SMALLINT state, SMALLINT loc, uchar *buf)
{
   SMALLINT nbyt,nbit;

   nbyt = (loc / 8);
   nbit = loc - (nbyt * 8);

   if (op == WRITE_FUNCTION)
   {
      if (state)
         buf[nbyt] |= (0x01 << nbit);
      else
         buf[nbyt] &= ~(0x01 << nbit);

      return 1;
   }
   else
      return ((buf[nbyt] >> nbit) & 0x01);
}



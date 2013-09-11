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
//  usbwtran.C - USB (DS2490) transport functions for 1-Wire Public Domain
//
//  Version: 3.00
//

#include "ownet.h"
#include "ds2490.h"
#include <windows.h>

// handles to USB ports
extern HANDLE usbhnd[MAX_PORTNUM];

//--------------------------------------------------------------------------
// The 'owBlock' transfers a block of data to and from the
// 1-Wire Net with an optional reset at the begining of communication.
// The result is returned in the same buffer.
//
// 'do_reset' - cause a owTouchReset to occure at the begining of
//              communication TRUE(1) or not FALSE(0)
// 'tran_buf' - pointer to a block of unsigned
//              chars of length 'TranferLength' that will be sent
//              to the 1-Wire Net
// 'tran_len' - length in bytes to transfer
// Supported devices: all
//
// Returns:   TRUE (1) : The optional reset returned a valid
//                       presence (do_reset == TRUE) or there
//                       was no reset required.
//            FALSE (0): The reset did not return a valid prsence
//                       (do_reset == TRUE).
//
//  The maximum tran_length is (160)
//
SMALLINT owBlock(int portnum, SMALLINT do_reset, uchar *tran_buf, SMALLINT tran_len)
{
   SETUP_PACKET setup;
   ULONG nOutput = 0;
	STATUS_PACKET status;
	BOOL	command_issued = FALSE;
	WORD	bytes_index=0;
	WORD	nBytesLeft = tran_len;
	WORD	block_len;
	WORD	block_write;
	WORD	loopcount;

   // check if need to do a owTouchReset first
   if (do_reset)
   {
      if (!owTouchReset(portnum))
         return FALSE;
   }

   // check for a block too big
   if (tran_len > 192)
   {
      OWERROR(OWERROR_BLOCK_TOO_BIG);
      return FALSE;
   }

   // loop to write in blocks of 64
	while (nBytesLeft > 0)
	{
      // Put the bytes of data in EP2 (Warning: do not overfill EP2 FIFO)
      // Let's begin by sending 64 bytes to EP2, then send the vendor command (EP0),
      // followed by additional data (while monitoring EP2 FIFO).
      if (nBytesLeft > 64)
         block_len = 64;
      else
         block_len = nBytesLeft;

      // fill EP2
      block_write = block_len;
      if (!DS2490Write(usbhnd[portnum], &tran_buf[bytes_index], &block_write))
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return FALSE;
      }

      // check for write incomplete
      if (block_write != block_len)
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return FALSE;
      }

      // check for one time send of block IO command 
      if (!command_issued)
      {
         // send vendor command
         setup.RequestTypeReservedBits = 0x40;	
         setup.Request = COMM_CMD;				
         setup.Value = COMM_BLOCK_IO | COMM_IM | COMM_F;	
         setup.Index = tran_len;				
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

         command_issued = TRUE;
      }

      // check whether device idle, block_len bytes ready for read
      // and wait for byte's arrival if necessary
      loopcount = 200;	  
      do
      {
         if (!DS2490GetStatus(usbhnd[portnum], &status, NULL))
         {
            AdapterRecover(portnum);
   	      return FALSE;
         }
         msDelay(1);
      }
      while (!(status.StatusFlags & STATUSFLAGS_IDLE)
	            && (status.ReadBufferStatus < block_len)
	            && (loopcount-- > 0));

      // check for timeout
      if ((loopcount == 0) && (status.ReadBufferStatus < block_len))
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return FALSE;
      }

      // read response on previous write
      block_write = block_len;
      if (!DS2490Read(usbhnd[portnum], &tran_buf[bytes_index], &block_write))
      {
         AdapterRecover(portnum);
         return FALSE;  
      }

      // check for read incomplete
      if (block_write != block_len)
      {
         OWERROR(OWERROR_ADAPTER_ERROR);
         AdapterRecover(portnum);
         return FALSE;
      }

      // adjust buffer's pointers
      nBytesLeft =  nBytesLeft - block_len;
      bytes_index = bytes_index + block_len;
	} 

   return TRUE;
}

//--------------------------------------------------------------------------
// Write a byte to an EPROM 1-Wire device.
//
// Supported devices: crc_type=0(CRC8)
//                        DS1982
//                    crc_type=1(CRC16)
//                        DS1985, DS1986, DS2407
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'write_byte' - byte to program
// 'addr'       - address of byte to program
// 'write_cmd'  - command used to write (0x0F reg mem, 0x55 status)
// 'crc_type'   - CRC used (0 CRC8, 1 CRC16)
// 'do_access'  - Flag to access device for each byte
//                (0 skip access, 1 do the access)
//                WARNING, only use do_access=0 if programing the NEXT
//                byte immediatly after the previous byte.
//
// Returns: >=0   success, this is the resulting byte from the program
//                effort
//          -1    error, device not connected or program pulse voltage
//                not available
//
SMALLINT owProgramByte(int portnum, SMALLINT write_byte, int addr, SMALLINT write_cmd,
                       SMALLINT crc_type, SMALLINT do_access)
{
   ushort lastcrc16;
   uchar lastcrc8;

   // optionally access the device
   if (do_access)
   {
      if (!owAccess(portnum))
      {
         OWERROR(OWERROR_ACCESS_FAILED);
         return -1;
      }

      // send the write command
      if (!owWriteByte(portnum,write_cmd))
      {
         OWERROR(OWERROR_WRITE_BYTE_FAILED);
         return -1;
      }

      // send the address
      if (!owWriteByte(portnum,addr & 0xFF) || !owWriteByte(portnum,addr >> 8))
      {
         OWERROR(OWERROR_WRITE_BYTE_FAILED);
         return -1;
      }
   }

   // send the data to write
   if (!owWriteByte(portnum,write_byte))
   {
      OWERROR(OWERROR_WRITE_BYTE_FAILED);
      return -1;
   }

   // read the CRC
   if (crc_type == 0)
   {
      // calculate CRC8
      if (do_access)
      {
         setcrc8(portnum,0);
         docrc8(portnum,(uchar)write_cmd);
         docrc8(portnum,(uchar)(addr & 0xFF));
         docrc8(portnum,(uchar)(addr >> 8));
      }
      else
         setcrc8(portnum,(uchar)(addr & 0xFF));

      docrc8(portnum,(uchar)write_byte);
      // read and calculate the read crc
      lastcrc8 = docrc8(portnum,(uchar)owReadByte(portnum));
      // crc should now be 0x00
      if (lastcrc8 != 0)
      {
         OWERROR(OWERROR_CRC_FAILED);
         return -1;
      }
   }
   else
   {
      // CRC16
      if (do_access)
      {
         setcrc16(portnum,0);
         docrc16(portnum,(ushort)write_cmd);
         docrc16(portnum,(ushort)(addr & 0xFF));
         docrc16(portnum,(ushort)(addr >> 8));
      }
      else
         setcrc16(portnum,(ushort)addr);
      docrc16(portnum,(ushort)write_byte);
      // read and calculate the read crc
      docrc16(portnum,(ushort)owReadByte(portnum));
      lastcrc16 = docrc16(portnum,(ushort)owReadByte(portnum));
      // crc should now be 0xB001
      if (lastcrc16 != 0xB001)
         return -1;
   }

   // send the program pulse
   if (!owProgramPulse(portnum))
   {
      OWERROR(OWERROR_PROGRAM_PULSE_FAILED);
      return -1;
   }

   // read back and return the resulting byte
   return owReadByte(portnum);
}



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
//  multinet.c - Wrapper to hook all adapter types in the 1-Wire Public 
//               Domain API for transport functions.
//
//  Version: 3.00
//

#include "mownet.h"

extern SMALLINT default_type; 

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
//  The maximum tran_len is 64
//
SMALLINT owBlock(int portnum, SMALLINT do_reset, uchar *tran_buf, SMALLINT tran_len)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owBlock_DS9490(portnum & 0xFF, do_reset, tran_buf, tran_len);
      case DS1410E: return owBlock_DS1410E(portnum & 0xFF, do_reset, tran_buf, tran_len);
      default: 
      case DS9097U: return owBlock_DS9097U(portnum & 0xFF, do_reset, tran_buf, tran_len);
   };
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
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owProgramByte_DS9490(portnum & 0xFF, write_byte, addr, write_cmd, crc_type, do_access);
      case DS1410E: return owProgramByte_DS1410E(portnum & 0xFF, write_byte, addr, write_cmd, crc_type, do_access);
      default: 
      case DS9097U: return owProgramByte_DS9097U(portnum & 0xFF, write_byte, addr, write_cmd, crc_type, do_access);
   };
}



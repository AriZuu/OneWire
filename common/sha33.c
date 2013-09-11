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
// sha33.c - Low-level memory and SHA functions for the DS1961S.
//
// Version: 2.10
//

#include "shaib.h"

//this global is in mbshaee.c - necessary for writing to the part
//using the file I/O utilities - yecch...
extern uchar local_secret[8];

//----------------------------------------------------------------------
// Read the scratchpad with CRC16 verification for DS1961S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - pointer to address that is read from scratchpad
// 'es'          - pointer to offset byte read from scratchpad
// 'data'        - pointer to data buffer read from scratchpad
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - scratch read, address, es, and data returned
//         FALSE - error reading scratch, device not present
//
//
SMALLINT ReadScratchpadSHA33(int portnum, int* address, uchar* es,
                             uchar* data, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[16];
   SMALLINT i;
   ushort lastcrc16;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
      resume = 1; // for addition later
   }

   // read scratchpad command
   send_block[send_cnt++] = CMD_READ_SCRATCHPAD;

   // now add the read bytes for data bytes and crc16
   memset(&send_block[send_cnt], 0x0FF, 13);
   send_cnt += 13;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // calculate CRC16 of result
   setcrc16(portnum,0);
   for (i = resume; i < send_cnt ; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );

   // copy data to return buffers
   if(address)
      *address = (send_block[2+resume] << 8) | send_block[1+resume];
   if(es)
      *es = send_block[3+resume];

   memcpy(data, &send_block[4+resume], 8);

   // success
   return TRUE;
}

//----------------------------------------------------------------------
// Write the scratchpad with CRC16 verification for DS1961S.  There must
// be eight bytes worth of data in the buffer.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number is provided to
//               indicate the symbolic port number.
// 'address'   - address to write data to
// 'data'      - data to write
// 'resume'    - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - write to scratch verified
//         FALSE - error writing scratch, device not present, or HIDE
//                 flag is in incorrect state for address being written.
//
SMALLINT WriteScratchpadSHA33(int portnum, int address, uchar *data,
                              SMALLINT resume)
{
   uchar send_block[15];
   short send_cnt=0,i;
   ushort lastcrc16;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   setcrc16(portnum,0);
   // write scratchpad command
   send_block[send_cnt] = CMD_WRITE_SCRATCHPAD;
   docrc16(portnum,send_block[send_cnt++]);
   // address 1
   send_block[send_cnt] = (uchar)(address & 0xFF);
   docrc16(portnum,send_block[send_cnt++]);
   // address 2
   send_block[send_cnt] = (uchar)((address >> 8) & 0xFF);
   docrc16(portnum,send_block[send_cnt++]);
   // data
   for (i = 0; i < 8; i++)
   {
      send_block[send_cnt] = data[i];
      docrc16(portnum,send_block[send_cnt++]);
   }
   // CRC16
   send_block[send_cnt++] = 0xFF;
   send_block[send_cnt++] = 0xFF;

   //PrintHexLabeled("write_scratchpad send_block before", send_block, send_cnt);
   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );
   //PrintHexLabeled("write_scratchpad send_block after", send_block, send_cnt);

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // perform CRC16 of last 2 byte in packet
   for (i = send_cnt - 2; i < send_cnt; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );

   // success
   return TRUE;
}


//----------------------------------------------------------------------
// Copy the scratchpad with verification for DS1961S
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - address of destination
// 'MAC'         - authentication MAC, required to execute copy scratchpad
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - copy scratch verified
//         FALSE - error during copy scratch, device not present, or HIDE
//                 flag is in incorrect state for address being written.
//
SMALLINT CopyScratchpadSHA33(int portnum, int address, uchar* MAC,
                             SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[10];
   uchar test;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   // copy scratchpad command
   send_block[send_cnt++] = CMD_COPY_SCRATCHPAD;
   // address 1
   send_block[send_cnt++] = (uchar)(address & 0xFF);
   // address 2
   send_block[send_cnt++] = (uchar)((address >> 8) & 0xFF);
   // es
   send_block[send_cnt++] = (uchar)((address + 7)&0x1F);

   // verification bytes
   //for (i = 0; i < num_verf; i++)
   //   send_block[send_cnt++] = 0xFF;
   //memset(&send_block[send_cnt], 0x0FF, num_verf);
   //send_cnt += num_verf;

   // now send the block
   //PrintHexLabeled("copy_scratchpad send_block before", send_block, send_cnt);
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );
   //PrintHexLabeled("copy_scratchpad send_block after", send_block, send_cnt);

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // wait 2ms while DS1961S computes SHA
   msDelay(2);

   // now send the MAC
   //PrintHexLabeled("copy_scratchpad MAC send_block before", MAC, 20);
   OWASSERT( owBlock(portnum,FALSE,MAC,20),
             OWERROR_BLOCK_FAILED, FALSE );
   //PrintHexLabeled("copy_scratchpad MAC send_block after", MAC, 20);

   // now wait 10ms for EEPROM write.
   // should do strong pullup here.
   msDelay(10);


   test = owReadByte(portnum);

   if((test != 0xAA) && (test != 0x55))
   {
      if(test == 0xFF)
         OWERROR( OWERROR_WRITE_PROTECTED );
      else if(test == 0x00)
         OWERROR( OWERROR_NONMATCHING_MAC );
      else
         OWERROR( OWERROR_NO_COMPLETION_BYTE );

      return FALSE;
   }


   return TRUE;
}


//----------------------------------------------------------------------
// Read Authenticated Page for DS1961S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'pagenum'     - page number to do a read authenticate
// 'data'        - buffer to read into from page
// 'sign'        - buffer for storing resulting sha computation
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: Value of write cycle counter for the page
//         -1 for error
//
int ReadAuthPageSHA33(int portnum, SMALLINT pagenum, uchar* data,
                      uchar* sign, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[55];
   SMALLINT i;
   ushort lastcrc16;
   int address = pagenum << 5;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, -1 );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   //seed the crc
   setcrc16(portnum,0);

   // create the send block
   // Read Authenticated Page command
   send_block[send_cnt] = CMD_READ_AUTH_PAGE;
   docrc16(portnum, send_block[send_cnt++]);

   // TA1
   send_block[send_cnt] = (uchar)(address & 0xFF);
   docrc16(portnum,send_block[send_cnt++]);

   // TA2
   send_block[send_cnt] = (uchar)((address >> 8) & 0xFF);
   docrc16(portnum,send_block[send_cnt++]);

   // now add the read bytes for data bytes, 0xFF byte, and crc16
   memset(&send_block[send_cnt], 0x0FF, 35);
   send_cnt += 35;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, -1 );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // check the CRC
   for (i = resume?4:3; i < send_cnt; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, -1);

   // transfer results
   memcpy(data, &send_block[send_cnt-35], 32);

   // now wait for the MAC computation.
   // should provide strong pull-up here.
   msDelay(2);

   // read the MAC
   memset(send_block,0xFF,23);
   OWASSERT( owBlock(portnum,FALSE,send_block,23),
             OWERROR_BLOCK_FAILED, -1 );

   // check CRC of the MAC
   setcrc16(portnum,0);
   for (i = 0; i < 22; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC of the MAC is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, -1);

   // check verification
   OWASSERT( ((send_block[22] & 0xF0) == 0x50) ||
             ((send_block[22] & 0xF0) == 0xA0),
             OWERROR_NO_COMPLETION_BYTE, -1);

   // transfer MAC into buffer
   memcpy(sign, send_block, 20);

   // no write cycle counter
   return 0;
}

//----------------------------------------------------------------------
// Read Memory Page for DS1961S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'pagenum'     - page number to do a read authenticate
// 'data'        - buffer to read into from page
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - Read successfull
//         FALSE - error occurred during read.
//
SMALLINT ReadMemoryPageSHA33(int portnum, SMALLINT pagenum,
                             uchar* data, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[36];
   int address = pagenum << 5;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, -1 );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   // create the send block
   // Read Memory command
   send_block[send_cnt++] = CMD_READ_MEMORY;
   // TA1
   send_block[send_cnt++] = (uchar)(address & 0xFF);
   // TA2
   send_block[send_cnt++] = (uchar)((address >> 8) & 0xFF);

   // now add the read bytes for data bytes
   memset(&send_block[send_cnt], 0x0FF, 32);
   send_cnt += 32;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   // transfer the results
   memcpy(data, &send_block[send_cnt-32], 32);

   return TRUE;
}


//----------------------------------------------------------------------
// Loads new system secret directly into secret memory for DS1961S.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'secret'        - the secret buffer used, must be 8 bytes.
// 'resume'        - if true, device access is resumed using the RESUME
//                   ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                   used along with the device's entire address number.
//
// Return: TRUE - Load first secret successfull
//         FALSE - error occurred during secret installation or the
//                 the secret is write-protected.
//
SMALLINT LoadFirstSecret33(int portnum, uchar* secret,
                           SMALLINT resume)
{
   uchar send_block[15], test;
   int send_cnt = 0;

   // write secret data into scratchpad
   OWASSERT( WriteScratchpadSHA33(portnum, 0x80, secret, resume),
             OWERROR_WRITE_SCRATCHPAD_FAILED, FALSE );

   send_block[send_cnt++] = ROM_CMD_RESUME;
   send_block[send_cnt++] = CMD_READ_SCRATCHPAD;
   memset(&send_block[send_cnt], 0xFF, 13);
   send_cnt += 13;

   // now send the block
   OWASSERT( owBlock(portnum,TRUE,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   // compare the contents of what we read with the secret
   OWASSERT( (memcmp(&send_block[5], secret, 8)==0),
             OWERROR_READ_SCRATCHPAD_FAILED, FALSE );

   // now copy the scratchpad to the secret location
   send_block[1] = SHA33_LOAD_FIRST_SECRET;

   // now send the block
   OWASSERT( owBlock(portnum,TRUE,send_block,5),
             OWERROR_BLOCK_FAILED, FALSE );

   // wait 10ms for EEPROM write
   msDelay(20);

   test = owReadByte(portnum);

   if((test != 0xAA) && (test != 0x55))
   {
      if(test == 0xFF)
         OWERROR( OWERROR_WRITE_PROTECTED );
      else if(test == 0x00)
         OWERROR( OWERROR_NONMATCHING_MAC );
      else
         OWERROR( OWERROR_NO_COMPLETION_BYTE );

      return FALSE;
   }


   return TRUE;
}

//----------------------------------------------------------------------
// Uses Load First Secret to copy the current contents of the scratcpad
// to the specified memory location.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'address'       - The address to copy the data to.
// 'resume'        - if true, device access is resumed using the RESUME
//                   ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                   used along with the device's entire address number.
//
// Return: TRUE - Load first secret successfull
//         FALSE - error occurred during secret installation or the
//                 the secret is write-protected.
//
SMALLINT LoadFirstSecretAddress33(int portnum, int address, SMALLINT resume)
{
   uchar send_block[15], test;
   int send_cnt = 0;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, -1 );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   // now copy the scratchpad to the secret location
   send_block[send_cnt++] = SHA33_LOAD_FIRST_SECRET;
   send_block[send_cnt++] = (uchar)(address);
   send_block[send_cnt++] = (uchar)(address>>8);
   send_block[send_cnt++] = ((address + 7) & 0x01F);

   // now send the block
   OWASSERT( owBlock(portnum,TRUE,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   // wait 10ms for EEPROM write
   msDelay(20);

   test = owReadByte(portnum);

   if((test != 0xAA) && (test != 0x55))
   {
      if(test == 0xFF)
         OWERROR( OWERROR_WRITE_PROTECTED );
      else if(test == 0x00)
         OWERROR( OWERROR_NONMATCHING_MAC );
      else
         OWERROR( OWERROR_NO_COMPLETION_BYTE );

      return FALSE;
   }

   return TRUE;
}

//----------------------------------------------------------------------
// Refresh Scratchpad - Loads contents of specified address into
// scratchpad.
//
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'address'       - The address to copy the data to.
// 'resume'        - if true, device access is resumed using the RESUME
//                   ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                   used along with the device's entire address number.
//
// Return: TRUE - Load first secret successfull
//         FALSE - error occurred during secret installation or the
//                 the secret is write-protected.
//
SMALLINT RefreshScratchpad33(int portnum, int address, SMALLINT resume)
{
   uchar send_block[15];
   int send_cnt = 0, i;
   ushort lastcrc16;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, -1 );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   send_block[send_cnt++] = SHA33_REFRESH_SCRATCHPAD;
   send_block[send_cnt++] = (uchar)(address);
   send_block[send_cnt++] = (uchar)(address>>8);
   memset(&send_block[send_cnt], 0x00, 8); // Don't care data
   send_cnt += 8;
   send_block[send_cnt++] = 0xFF; // CRC16
   send_block[send_cnt++] = 0xFF;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   // check CRC
   setcrc16(portnum,0);
   i = (resume?1:0);
   for (i = 0; i < send_cnt; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC of the command is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, -1 );

   return TRUE;
}

//----------------------------------------------------------------------
// Refresh page - Uses Load First Secret and Refresh Scratchpad to
// 'refresh' the contents of a page.
//
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'pagenum'       - the pagenumber to refresh.
// 'resume'        - if true, device access is resumed using the RESUME
//                   ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                   used along with the device's entire address number.
//
// Return: TRUE - Load first secret successfull
//         FALSE - error occurred during secret installation or the
//                 the secret is write-protected.
//
SMALLINT RefreshPage33(int portnum, SMALLINT pagenum, SMALLINT resume)
{
   uchar offset;
   int address = pagenum*32;
   SMALLINT success = TRUE;

   for(offset = 0; offset<32; offset+=8)
   {
      SMALLINT lsucc;
      lsucc = RefreshScratchpad33(portnum, (address + offset), resume);
      success = lsucc && success;
      if(lsucc)
      {
         lsucc = LoadFirstSecretAddress33(portnum, (address + offset), TRUE);
         success = lsucc && success;
      }
      resume = TRUE;
   }

   return success;
}


//global for InstallSystemSecret33 and BindSecretToiButton
static uchar currentSecret[8];

//----------------------------------------------------------------------
// Installs new system secret for DS1961S.  input_secret must be
// divisible by 47.  Then, each block of 47 is split up with 32 bytes
// written to a data page and 8 bytes (of the remaining 15) are
// written to the scratchpad.  The unused 7 bytes should be 0xFF for
// compatibility with the 1963S coprocessor (see ReformatSecretFor1961S())
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'pagenum'       - page number to do a read authenticate
// 'secretnum'     - param ignored, used to keep same calling syntax for
//                   DS1963S InstallSystemSecret.
// 'input_secret'  - the input secret buffer used.
// 'secret_length' - the length of the input secret buffer, divisibly
//                   by 47.
// 'resume'        - if true, device access is resumed using the RESUME
//                   ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                   used along with the device's entire address number.
//
// Return: TRUE - Install successfull
//         FALSE - error occurred during secret installation.
//
SMALLINT InstallSystemSecret33(int portnum, SMALLINT pagenum,
                               SMALLINT secretnum, uchar* input_secret,
                               int secret_length, SMALLINT resume)
{
   int offset = 0, bytes_left = 0;
   uchar data[32],scratchpad[32],MT[64],MAC[20];
   long hash[5];

   memset(currentSecret, 0x00, 8);

   //digest buffer padding
   memset(&MT[52], 0xFF, 3);
   MT[55] = (uchar)0x80;
   memset(&MT[56], 0x00, 6);
   MT[62] = (uchar)0x01;
   MT[63] = (uchar)0xB8;

   // now install the master secret
   for(offset=0; offset<secret_length; offset+=47)
   {
      // clear the buffer
      memset(data, 0x0FF, 32);
      memset(scratchpad, 0x0FF, 32);

      // Determine the amount of bytes remaining to be installed.
      bytes_left = secret_length - offset;

      // copy input_secret data into page buffer and scratchpad buffer
      memcpy(data, &input_secret[offset], (bytes_left<32?bytes_left:32));
      if(bytes_left>32)
      {
         memcpy(&scratchpad[8], &input_secret[offset+32],
                (bytes_left<47?bytes_left-32:15));
      }

      // -- -- Perform ComputeNextSecret in software -- --

      // first four bytes of secret
      memcpy(MT,currentSecret,4);
      // page data to replace
      memcpy(&MT[4],data,32);
      // contents of scratchpad
      memcpy(&MT[36],&scratchpad[8],15);
      // Fix M-P-X control bits
      MT[40] = (uchar)(MT[40]&0x3F);
      // last four bytes of secret
      memcpy(&MT[48],&currentSecret[4],4);

      ComputeSHAVM(MT, hash);
      HashToMAC(hash, MAC);

      memcpy(currentSecret, MAC, 8);
   }

   // After computing the secret, load it onto the button
   OWASSERT( LoadFirstSecret33(portnum, currentSecret, resume),
             OWERROR_LOAD_FIRST_SECRET_FAILED, FALSE );

   //set the secret in mbshaee.c
   //-- man I hate globals!
   memcpy(local_secret, currentSecret, 8);

   return TRUE;
}

//----------------------------------------------------------------------
// BindSecretToiButton(portnum,pageNum,secretNum,bindData,bindCode);
// Only used on initialization, must be called after InstallSystemSecret
SMALLINT BindSecretToiButton33(int portnum, SMALLINT pagenum,
                               SMALLINT secretnum,
                               uchar* bindData, uchar* bindCode,
                               SMALLINT resume)
{
   uchar MT[64],MAC[20];
   long hash[5];

   // digest buffer padding
   memset(MT, 0xFF, 64);
   MT[55] = (uchar)0x80;
   memset(&MT[56], 0x00, 6);
   MT[62] = (uchar)0x01;
   MT[63] = (uchar)0xB8;

   // -- -- Perform ComputeNextSecret in software -- --

   // first four bytes of secret
   memcpy(MT,currentSecret,4);
   // page data to replace
   memcpy(&MT[4],bindData,32);
   // contents of scratchpad
   memcpy(&MT[40],&bindCode[4],8);
   // Fix M-P-X control bits
   MT[40] = (uchar)(MT[40]&0x3F);
   // last four bytes of secret
   memcpy(&MT[48],&currentSecret[4],4);

   ComputeSHAVM(MT, hash);
   HashToMAC(hash, MAC);

   memcpy(currentSecret, MAC, 8);

   //set the secret in mbshaee.c
   //-- man I hate globals!
   memcpy(local_secret, currentSecret, 8);

   OWASSERT( LoadFirstSecret33(portnum, currentSecret, resume),
          OWERROR_LOAD_FIRST_SECRET_FAILED, FALSE );

   return TRUE;
}

//constants used in SHA computation
static const long KTN[4] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

// calculation used for the SHA MAC
static long NLF (long B, long C, long D, int n)
{
   if(n<20)
      return ((B&C)|((~B)&D));
   else if(n<40)
      return (B^C^D);
   else if(n<60)
      return ((B&C)|(B&D)|(C&D));
   else
      return (B^C^D);
}

//----------------------------------------------------------------------
// computes a SHA given the 64 byte MT digest buffer.  The resulting 5
// long values are stored in the given long array, hash.
//
// Note: This algorithm is the SHA-1 algorithm as specified in the
// datasheet for the DS1961S, where the last step of the official
// FIPS-180 SHA routine is omitted (which only involves the addition of
// constant values).
//
// 'MT'        - buffer containing the message digest
// 'hash'      - result buffer
//
void ComputeSHAVM(uchar* MT, long* hash)
{
   unsigned long MTword[80];
   int i;
   long ShftTmp;
   long Temp;

   for(i=0;i<16;i++)
   {
      MTword[i] = ((MT[i*4]&0x00FF) << 24) | ((MT[i*4+1]&0x00FF) << 16) |
                  ((MT[i*4+2]&0x00FF) << 8) | (MT[i*4+3]&0x00FF);
   }

   for(;i<80;i++)
   {
      ShftTmp = MTword[i-3] ^ MTword[i-8] ^ MTword[i-14] ^ MTword[i-16];
      MTword[i] = ((ShftTmp << 1) & 0xFFFFFFFE) |
                  ((ShftTmp >> 31) & 0x00000001);
   }

   hash[0] = 0x67452301;
   hash[1] = 0xEFCDAB89;
   hash[2] = 0x98BADCFE;
   hash[3] = 0x10325476;
   hash[4] = 0xC3D2E1F0;

   for(i=0;i<80;i++)
   {
      ShftTmp = ((hash[0] << 5) & 0xFFFFFFE0) | ((hash[0] >> 27) & 0x0000001F);
      Temp = NLF(hash[1],hash[2],hash[3],i) + hash[4]
               + KTN[i/20] + MTword[i] + ShftTmp;
      hash[4] = hash[3];
      hash[3] = hash[2];
      hash[2] = ((hash[1] << 30) & 0xC0000000) | ((hash[1] >> 2) & 0x3FFFFFFF);
      hash[1] = hash[0];
      hash[0] = Temp;
   }
}

//----------------------------------------------------------------------
// Converts the 5 long numbers that represent the result of a SHA
// computation into the 20 bytes (with proper byte ordering) that the
// SHA iButton's expect.
//
// 'hash'      - result of SHA calculation
// 'MAC'       - 20-byte, LSB-first message authentication code for SHA
//                iButtons.
//
void HashToMAC(long* hash, uchar* MAC)
{
   long temp;
   SMALLINT i, j, offset;

   //iButtons use LSB first, so we have to turn
   //the result around a little bit.  Instead of
   //result A-B-C-D-E, our result is E-D-C-B-A,
   //where each letter represents four bytes of
   //the result.
   for(j=4; j>=0; j--)
   {
      temp = hash[j];
      offset = (4-j)*4;
      for(i=0; i<4; i++)
      {
         MAC[i+offset] = (uchar)temp;
         temp >>= 8;
      }
   }
}


//----------------------------------------------------------------------
// For DS1961S compatibility, must be a multiple of 47 bytes in length.
// This function FFs out appropriate bytes so that computation is
// identical to secret computation on the DS1963S and the DS1961S.
//
// 'auth_secret'     - input secret buffer
// 'secret_length'   - length of the secret buffer (must be divisible
//                      by 47)
//
void ReformatSecretFor1961S(uchar* auth_secret, int secret_length)
{
   int i;
   for(i=0; i<secret_length; i+=47)
   {
      memset(&auth_secret[i + 32], 0x0FF, 4);
      memset(&auth_secret[i + 44], 0x0FF, 3);
   }
}



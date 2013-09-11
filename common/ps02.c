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
// ps02.c - contains functions for communication to the DS1991
//


#include "ownet.h"
#include "ps02.h"

// Global of codes
uchar pscodes[9][8];


/**
 * Sets up the block codes for the copy scratchpad command.
 *
 * @param codes a 2 dimensional array [9][8] to contain the
 * codes.
 */
void initBlockCodes()
{
   pscodes[8][0] = 0x56;            //ALL 64 bytes address 0x00 to 0x3F
   pscodes[8][1] = 0x56;
   pscodes[8][2] = 0x7F;
   pscodes[8][3] = 0x51;
   pscodes[8][4] = 0x57;
   pscodes[8][5] = 0x5D;
   pscodes[8][6] = 0x5A;
   pscodes[8][7] = 0x7F;
   pscodes[0][0] = 0x9A;   //identifier (block 0)
   pscodes[0][1] = 0x9A;
   pscodes[0][2] = 0xB3;
   pscodes[0][3] = 0x9D;
   pscodes[0][4] = 0x64;
   pscodes[0][5] = 0x6E;
   pscodes[0][6] = 0x69;
   pscodes[0][7] = 0x4C;
   pscodes[1][0] = 0x9A;   //password  (block 1)
   pscodes[1][1] = 0x9A;
   pscodes[1][2] = 0x4C;
   pscodes[1][3] = 0x62;
   pscodes[1][4] = 0x9B;
   pscodes[1][5] = 0x91;
   pscodes[1][6] = 0x69;
   pscodes[1][7] = 0x4C;
   pscodes[2][0] = 0x9A;   //address 0x10 to 0x17  (block 2)
   pscodes[2][1] = 0x65;
   pscodes[2][2] = 0xB3;
   pscodes[2][3] = 0x62;
   pscodes[2][4] = 0x9B;
   pscodes[2][5] = 0x6E;
   pscodes[2][6] = 0x96;
   pscodes[2][7] = 0x4C;
   pscodes[3][0] = 0x6A;            //address 0x18 to 0x1F  (block 3)
   pscodes[3][1] = 0x6A;
   pscodes[3][2] = 0x43;
   pscodes[3][3] = 0x6D;
   pscodes[3][4] = 0x6B;
   pscodes[3][5] = 0x61;
   pscodes[3][6] = 0x66;
   pscodes[3][7] = 0x43;
   pscodes[4][0] = 0x95;   //address 0x20 to 0x27  (block 4)
   pscodes[4][1] = 0x95;
   pscodes[4][2] = 0xBC;
   pscodes[4][3] = 0x92;
   pscodes[4][4] = 0x94;
   pscodes[4][5] = 0x9E;
   pscodes[4][6] = 0x99;
   pscodes[4][7] = 0xBC;
   pscodes[5][0] = 0x65;            //address 0x28 to 0x2F  (block 5)
   pscodes[5][1] = 0x9A;
   pscodes[5][2] = 0x4C;
   pscodes[5][3] = 0x9D;
   pscodes[5][4] = 0x64;
   pscodes[5][5] = 0x91;
   pscodes[5][6] = 0x69;
   pscodes[5][7] = 0xB3;
   pscodes[6][0] = 0x65;            //address 0x30 to 0x37  (block 6)
   pscodes[6][1] = 0x65;
   pscodes[6][2] = 0xB3;
   pscodes[6][3] = 0x9D;
   pscodes[6][4] = 0x64;
   pscodes[6][5] = 0x6E;
   pscodes[6][6] = 0x96;
   pscodes[6][7] = 0xB3;
   pscodes[7][0] = 0x65;            //address 0x38 to 0x3F  (block 7)
   pscodes[7][1] = 0x65;
   pscodes[7][2] = 0x4C;
   pscodes[7][3] = 0x62;
   pscodes[7][4] = 0x9B;
   pscodes[7][5] = 0x91;
   pscodes[7][6] = 0x96;
   pscodes[7][7] = 0xB3;
}


/**
 * Writes the data to the scratchpad from the given address.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire network.
 * addr     address to begin writing.  Must be between
 *          0x00 and 0x3F.
 * data     data to write.
 * len      the length of the data to write
 *
 * return:  TRUE  if the write worked
 *          FALSE if there was an error in writing
 */
SMALLINT writeScratchpad (int portnum, int addr, uchar *data, int len)
{
   int   dataRoom,i;
   uchar buffer[96];

   if(owAccess(portnum))
   {
      //confirm that data will fit
      if (addr > 0x3F)
      {
         OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
         return FALSE;
      }

      dataRoom = 0x3F - addr + 1;

      if (dataRoom < len)
      {
         OWERROR(OWERROR_DATA_TOO_LONG);
         return FALSE;
      }

      buffer[0] = ( uchar ) WRITE_SCRATCHPAD_COMMAND;
      buffer[1] = ( uchar ) (addr | 0xC0);
      buffer[2] = ( uchar ) (~buffer [1]);

      for(i=0;i<len;i++)
         buffer[i+3] = data[i];

      //send command block
      if((len+3) > 64)
      {
         if(!owBlock(portnum,FALSE,buffer,64))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
         else if(!owBlock(portnum,FALSE,&buffer[64],3))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
      }
      else if(!owBlock(portnum,FALSE,buffer,len+3))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return TRUE;
}

/**
 * Reads the entire scratchpad.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network
 * data     the data that was read from the scratchpad.
 *
 * return   TRUE  if the data was read without an error
 *
 */
SMALLINT readScratchpad(int portnum, uchar *data)
{
   uchar buffer[96];
   int i;

   if(owAccess(portnum))
   {
      buffer[0] = READ_SCRATCHPAD_COMMAND;
      buffer[1] = 0xC0;   //Starting address of scratchpad
      buffer[2] = 0x3F;

      for(i = 3; i < 67; i++)
         buffer[i] = 0xFF;

      //send command block
      if(!owBlock(portnum,FALSE,buffer,64))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
      else if(!owBlock(portnum,FALSE,&buffer[64],3))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

      for(i=0;i<64;i++)
         data[i] = buffer[i+3];
   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return TRUE;
}

/**
 * Writes the data from the scratchpad to the specified block or
 * blocks.  Note that the write will erase the data from the
 * scratchpad.
 *
 * portnum      the port number of the port being used for the 
 *              1-Wire Network.
 * key          subkey being written
 * passwd       password for the subkey being written
 * blockNum     number of the block to be copied (see page 7 of the
 *              DS1991 data sheet) block 0-7, or 8 to copy all 64 bytes.
 *
 * return       TRUE  if the copy scratchpad was successful.
 */
SMALLINT copyScratchpad(int portnum, int key, uchar *passwd, int blockNum)
{
   uchar buffer[96];
   int i;

   if(owAccess(portnum))
   {
      //confirm that input is OK
      if ((key < 0) || (key > 2))
      {
         OWERROR(OWERROR_KEY_OUT_OF_RANGE);
         return FALSE;
      }

      if ((blockNum < 0) || (blockNum > 8))
      {
         OWERROR(OWERROR_BLOCK_ID_OUT_OF_RANGE);
         return FALSE;
      }
   
      buffer[0] = COPY_SCRATCHPAD_COMMAND;
      buffer[1] = (uchar) (key << 6);
      buffer[2] = (uchar) (~buffer [1]);

      //set up block selector code
      for(i=0;i<8;i++)
         buffer[i+3] = pscodes[blockNum][i];

      //set up password
      for(i=0;i<8;i++)
         buffer[i+11] = passwd[i];

      //send command block
      if(!owBlock(portnum,FALSE,buffer,19))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return TRUE;
}


/**
 * Reads the subkey requested with the given key name and password.
 * Note that this method allows for reading from the subkey data
 * only which starts at address 0x10 within a key. It does not allow
 * reading from any earlier address within a key because the device
 * cannot be force to allow reading the password. This is why the
 * subkey number is or-ed with 0x10 in creating the address in bytes
 * 1 and 2 of the sendBlock.
 *
 * portnum    the port number of the port being used for the 
 *            1-Wire Network.
 * data       buffer of length 64 into which to write the data
 * key        number indicating the key to be read: 0, 1, or 2
 * passwd     password of destination subkey
 *
 * return:    TRUE  if reading the subkey was successful.
 */
SMALLINT readSubkey(int portnum, uchar *data, int key, uchar *passwd)
{
   uchar buffer[96];
   int i;

   if(owAccess(portnum))
   {
      //confirm key within legal parameters
      if (key > 0x03)
      {
         OWERROR(OWERROR_KEY_OUT_OF_RANGE);
         return FALSE;
      }

      buffer[0] = READ_SUBKEY_COMMAND;
      buffer[1] = (uchar) ((key << 6) | 0x10);
      buffer[2] = (uchar) (~buffer [1]);

      //prepare buffer to receive
      for (i = 3; i < 67; i++)
         buffer[i] = 0xFF;

      //insert password data
      for(i=0;i<8;i++)
         buffer[i+11] = passwd[i];

      //send command block
      if(!owBlock(portnum,FALSE,buffer,64))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }
      else if(!owBlock(portnum,FALSE,&buffer[64],3))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

      //create block to send back
      for(i=0;i<64;i++)
         data[i] = buffer[i+3];
   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return TRUE;
}

/**
 * Writes a new identifier and password to the secure subkey iButton
 *
 * portnum    the port number of the port being used for the 
 *            1-Wire Network.
 * key        number indicating the key to be read: 0, 1, or 2
 * oldName    identifier of the key used to confirm the correct
 *            key's password to be changed.  Must be exactly length 8.
 * newName    identifier to be used for the key with the new
 *            password.  Must be exactly length 8.
 * newPasswd  new password for destination subkey.  Must be
 *            exactly length 8.
 *
 * return     TRUE if the write was successful  
 */
SMALLINT writePassword(int portnum, int key, uchar *oldName, 
                       uchar *newName, uchar *newPasswd)
{
   uchar buffer[96];
   int   i;

   if(owAccess(portnum))
   {
      //confirm key names and passwd within legal parameters
      if (key > 0x03)
      {
         OWERROR(OWERROR_KEY_OUT_OF_RANGE);
         return FALSE;
      }

      buffer[0] = WRITE_PASSWORD_COMMAND;
      buffer[1] = (uchar) (key << 6);
      buffer[2] = (uchar) (~buffer[1]);

      //prepare buffer to receive 8 bytes of the identifier
      for(i = 3; i < 11; i++)
         buffer [i] = 0xFF;

      if(!owBlock(portnum,FALSE,buffer,11))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

      //prepare same subkey identifier for confirmation
      for(i=0;i<8;i++)
         buffer[i] = buffer[i+3];

      //prepare new subkey identifier
      for(i=0;i<8;i++)
         buffer[i+8] = newName[i];

      //prepare new password for writing
      for(i=0;i<8;i++)
         buffer[i+16] = newPasswd[i];

      //send command block
      if(!owBlock(portnum,FALSE,buffer,24))
      {
         OWERROR(OWERROR_BLOCK_FAILED);
         return FALSE;
      }

   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return TRUE;
}

/**
 * Writes new data to the secure subkey
 *
 * portnum   the port number of the port being used for the 
 *           1-Wire Network.
 * key       number indicating the key to be written: 0, 1, or 2
 * addr      address to start writing at ( 0x00 to 0x3F )
 * passwd    passwird for the subkey
 * data      data to be written
 * len       the length of the data to be written
 *
 * return    TRUE if the write was successful
 */
SMALLINT writeSubkey (int portnum, int key, int addr, uchar *passwd,
                      uchar *data, int len)
{
   uchar buffer[96];
   int i;

   if(owAccess(portnum))
   {
      //confirm key names and passwd within legal parameters
      if (key > 0x03)
      {
         OWERROR(OWERROR_KEY_OUT_OF_RANGE);
         return FALSE;
      }

      if ((addr < 0x00) || (addr > 0x3F))
      {
         OWERROR(OWERROR_WRITE_OUT_OF_RANGE);
         return FALSE;
      }

      buffer[0] = WRITE_SUBKEY_COMMAND;
      buffer[1] = (uchar) ((key << 6) | addr);
      buffer[2] = (uchar) (~buffer [1]);

      //prepare buffer to receive 8 bytes of the identifier
      for(i = 3; i < 11; i++)
         buffer[i] = 0xFF;

      //prepare same subkey identifier for confirmation
      for(i=0;i<8;i++)
         buffer[i+11] = passwd[i];

      //prepare data to write
      for(i=0;i<len;i++)
         buffer[i+19] = data[i];
      
      //send command block
      if(len+19 > 64)
      {
         if(!owBlock(portnum,FALSE,&buffer[0],64))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
         else if(!owBlock(portnum,FALSE,&buffer[64],(len+19)-64))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
      }
      else
      {
         if(!owBlock(portnum,FALSE,buffer,len+19))
         {
            OWERROR(OWERROR_BLOCK_FAILED);
            return FALSE;
         }
      }
   }
   else
   {
      OWERROR(OWERROR_DEVICE_SELECT_FAIL);
      return FALSE;
   }

   return TRUE;
}




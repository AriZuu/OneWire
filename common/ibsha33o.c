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
//  ibsha33o.C - SHA-iButton utility functions
//
//  Version:   2.00
//  History:
//
#include <stdio.h>
#include "ownet.h"
#include "ibsha33.h"

// globals
int in_overdrive[MAX_PORTNUM];


//----------------------------------------------------------------------
// Read the scratchpad with CRC16 verification
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - pointer to address that is read from scratchpad
// 'es'          - pointer to offset byte read from scratchpad
// 'data'        - pointer data buffer read from scratchpad
//
// Return: TRUE - scratch read, address, es, and data returned
//         FALSE - error reading scratch, device not present
//
//
int ReadScratchSHAEE(int portnum, ushort *address, uchar *es, uchar *data)
{
   short send_cnt=0;
   uchar send_block[50];
   int i;
   ushort lastcrc16;

   // access the device
   if (SelectSHAEE(portnum) == 1)
   {
      // read scratchpad command
      send_block[send_cnt++] = 0xAA;
      // now add the read bytes for data bytes and crc16
      for(i=0; i<13; i++)
         send_block[send_cnt++] = 0xFF;

      // now send the block
      if (owBlock(portnum,FALSE,send_block,send_cnt))
      {
         // copy data to return buffers
         *address = (send_block[2] << 8) | send_block[1];
         *es = send_block[3];

         // calculate CRC16 of result
         setcrc16(portnum,0);
         for (i = 0; i < send_cnt ; i++)
            lastcrc16 = docrc16(portnum,send_block[i]);

         // verify CRC16 is correct
         if (lastcrc16 == 0xB001)
         {
            for (i = 0; i < 8; i++)
               data[i] = send_block[4 + i];
            // success
            return TRUE;
         }
         else
            printf("ERROR DUE TO CRC %04X\n",lastcrc16);
      }
   }

   return FALSE;
}

//----------------------------------------------------------------------
// Write the scratchpad with CRC16 verification.  The data is padded
// until the offset is 0x1F so that the CRC16 is retrieved.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number is provided to
//               indicate the symbolic port number.
// 'address'   - address to write data to
// 'data'      - data to write
// 'data_len'  - number of bytes of data to write
//
// Return: TRUE - write to scratch verified
//         FALSE - error writing scratch, device not present, or HIDE
//                 flag is in incorrect state for address being written.
//
int WriteScratchSHAEE(int portnum, ushort address, uchar *data, int data_len)
{
   uchar send_block[50];
   short send_cnt=0,i;
   ushort lastcrc16;

   // access the device
   if (SelectSHAEE(portnum) == 1)
   {
      setcrc16(portnum,0);
      // write scratchpad command
      send_block[send_cnt] = 0x0F;
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
      // address 1
      send_block[send_cnt] = (uchar)(address & 0xFF);
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
      // address 2
      send_block[send_cnt] = (uchar)((address >> 8) & 0xFF);
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
      // data
      for (i = 0; i < data_len; i++)
      {
         send_block[send_cnt] = data[i];
         lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
      }

      // CRC16
      send_block[send_cnt++] = 0xFF;
      send_block[send_cnt++] = 0xFF;

      // now send the block
      if (owBlock(portnum,FALSE,send_block,send_cnt))
      {

         // perform CRC16 of last 2 byte in packet
         for (i = send_cnt - 2; i < send_cnt; i++)
            lastcrc16 = docrc16(portnum,send_block[i]);

         // verify CRC16 is correct
         if (lastcrc16 == 0xB001)
            return TRUE;
         else
            printf("ERROR DUE TO CRC %04X\n",lastcrc16);
      }
   }

   return FALSE;
}

//----------------------------------------------------------------------
// Read the memory pages.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - pointer to address that is read from scratchpad
// 'data'        - pointer data buffer read from memory
//
// Return: TRUE - read was successful
//         FALSE - error reading memory
//
int ReadMem(int portnum, ushort address, uchar *data)
{
   uchar send_block[64];
   short send_cnt=0,i,j;

   // access the device
   if (SelectSHAEE(portnum) == 1)
   {

      // read memory command
      send_block[send_cnt++] = 0xF0;
      // address 1
      send_block[send_cnt++] = (uchar)(address & 0xFF);
      // address 2
      send_block[send_cnt++] = (uchar)((address >> 8) & 0xFF);

      // data
      for (i = 0; i < 32; i++)
         send_block[send_cnt++] = 0xFF;

      // now send the block
      if (owBlock(portnum,FALSE,send_block,send_cnt))
      {
         for(j=3;j<send_cnt;j++)
            data[j-3] = send_block[j];

         return TRUE;
      }

   }

   return FALSE;
}

//----------------------------------------------------------------------
// Loads data into memory without a copy scratchpad
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - pointer to address that is read from scratchpad
// 'secret'      - pointer to secret data to be loaded
// 'data_len'    - length of data
//
// Return: TRUE - load first secret was successful
//         FALSE - error loading secret
//
//
int LoadFirSecret(int portnum, ushort address, uchar *secret, int data_len)
{
   ushort tempadd;
   uchar  es,data[8],test;
   uchar  send_block[32];
   int send_cnt=0;
   uchar mem[32];

   if(ReadMem(portnum,128,mem))
   {
      if((mem[8] == 0xAA) || (mem[8] == 0x55))
      {
         printf("SECRET IS WRITE PROTECTED CAN NOT LOAD FIRST SECRET\n");
         return FALSE;
      }

      if(((address < 32)) &&
         (((mem[9] == 0xAA) || (mem[9] == 0x55)) ||
          ((mem[13] == 0xAA) || (mem[13] == 0x55))))
      {
         printf("PAGE 0 IS WRITE PROTECTED\n");
         return FALSE;
      }

      if(((address < 128)) &&
         ((mem[9] == 0xAA) || (mem[9] == 0x55)))
      {
         printf("PAGE 0 TO 3 WRITE PROTECTED\n");
         return FALSE;
      }
   }

   if(WriteScratchSHAEE(portnum,address,secret,data_len) &&
      ReadScratchSHAEE(portnum,&tempadd,&es,data))
   {

      // access the device
      if (SelectSHAEE(portnum) == 1)
      {
         // write scratchpad command
         send_block[send_cnt++] = 0x5A;
         // address 1
         send_block[send_cnt++] = (uchar)(tempadd & 0xFF);
         // address 2
         send_block[send_cnt++] = (uchar)((tempadd >> 8) & 0xFF);
         // ending address with data status
         send_block[send_cnt++] = es;

         if(owBlock(portnum,FALSE,send_block,send_cnt))
         {
            // 10ms delay for writing first secret
            msDelay(10);

            test = owReadByte(portnum);

            if((test == 0xAA) || (test == 0x55))
               return TRUE;
            else
               printf("ERROR DUE TO INVALID LOAD\n");
         }
      }
   }
   else
      printf("ERROR DUE TO READ/WRITE\n");

   return FALSE;
}

//----------------------------------------------------------------------
// Copy the scratchpad with verification.  Send the MAC to the part.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - address of destination
// 'secret'      - secret data
// 'sn'          - serial number of part
// 'memory'      - the memory on the given page to copy to
//
// Return: TRUE - copy scratch verified
//         FALSE - error during copy scratch, device not present, or HIDE
//                 flag is in incorrect state for address being written.
//
int CopyScratchSHAEE(int portnum, ushort address, uchar *secret, uchar *sn, uchar *memory)
{
   short send_cnt=0;
   ushort tmpadd;
   uchar send_block[80];
   int i;
   uchar scratch[8],es,test;
   unsigned MT[64];
   long A,B,C,D,E;
   long Temp;

   for(i=0;i<4;i++)
      MT[i] = secret[i];

   for(i=4;i<32;i++)
      MT[i] = memory[i-4];

   if(ReadScratchSHAEE(portnum, &tmpadd, &es, &scratch[0]))
   {
      for(i=32;i<40;i++)
         MT[i] = scratch[i-32];
   }
   else
   {
      printf("ERROR DUE TO READING SCRATCH PAD DATA\n");
      return FALSE;
   }

   MT[40] = (address & 0xf0) >> 5;

   for(i=41;i<48;i++)
      MT[i] = sn[i-41];

   for(i=48;i<52;i++)
      MT[i] = secret[i-44];

   for(i=52;i<55;i++)
      MT[i] = 0xff;

   MT[55] = 0x80;

   for(i=56;i<62;i++)
      MT[i] = 0x00;

   MT[62] = 0x01;
   MT[63] = 0xB8;

   ComputeSHAEE(MT,&A,&B,&C,&D,&E);

   // access the device
   if (SelectSHAEE(portnum) == 1)
   {
      // Copy command
      send_block[send_cnt++] = 0x55;

      // address 1
      send_block[send_cnt++] = (uchar)(tmpadd & 0xFF);
      // address 2
      send_block[send_cnt++] = (uchar)((tmpadd >> 8) & 0xFF);

      // ending address with data status
      send_block[send_cnt++] = es;

      if(owBlock(portnum,FALSE,send_block,send_cnt))
      {
         send_cnt = 0;

         // sending MAC
         Temp = E;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
//            printf("%02X",send_block[send_cnt-1]);
            Temp >>= 8;
         }
//         printf("\n");

         Temp = D;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
//            printf("%02X",send_block[send_cnt-1]);
            Temp >>= 8;
         }
//         printf("\n");

         Temp = C;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
//            printf("%02X",send_block[send_cnt-1]);
            Temp >>= 8;
         }
//         printf("\n");

         Temp = B;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
//            printf("%02X",send_block[send_cnt-1]);
            Temp >>= 8;
         }
//         printf("\n");

         Temp = A;
         for(i=0;i<4;i++)
         {
            send_block[send_cnt++] = (uchar) (Temp & 0x000000FF);
//            printf("%02X",send_block[send_cnt-1]);
            Temp >>= 8;
         }
//         printf("\n");

         msDelay(2);

         if(owBlock(portnum,FALSE,send_block,send_cnt))
         {
            msDelay(10);

            test = owReadByte(portnum);

            if((test == 0xAA) || (test == 0x55))
               return TRUE;
            else
            {
               if(test == 0xFF)
                  printf("THAT AREA OF MEMORY IS WRITE-PROTECTED.\n");
               else if(test == 0x00)
                  printf("ERROR DUE TO NOT MATCHING MAC.\n");
            }
         }
      }
   }

   return FALSE;
}


//----------------------------------------------------------------------
// Calculate the next secret using the current one.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - pointer to address that is read from scratchpad
// 'secret'      - pointer to current secret and returns new secret
//
// Return: TRUE - next secret calculated correctly
//         FALSE - error calculating next secret
//
//
int NextSecret(int portnum, ushort address, uchar *secret)
{
   int i;
   unsigned MT[64];
   long A,B,C,D,E;
   long Temp;
   uchar memory[32],scratch[8],es;
   ushort tmpadd;
   uchar send_block[32];
   int send_cnt=0;

   for(i=0;i<8;i++)
      scratch[i] = 0x00;

   if(!WriteScratchSHAEE(portnum,address,scratch,8))
      return FALSE;

   for(i=0;i<4;i++)
      MT[i] = secret[i];

   if(ReadMem(portnum,(ushort)((address/((ushort)32))*32),memory))
      for(i=4;i<36;i++)
         MT[i] = memory[i-4];
   else
   {
      printf("ERROR DUE TO READING MEMORY PAGE DATA\n");
      return FALSE;
   }

   for(i=36;i<40;i++)
      MT[i] = 0xFF;

   if(ReadScratchSHAEE(portnum, &tmpadd, &es, scratch))
   {
      MT[40] = scratch[0] & 0x3F;

      for(i=41;i<48;i++)
         MT[i] = scratch[i-40];
      for(i=48;i<52;i++)
         MT[i] = secret[i-44];
   }
   else
   {
      printf("ERROR DUT TO READING SCRATCH PAD DATA\n");
      return FALSE;
   }

   for(i=52;i<55;i++)
      MT[i] = 0xFF;

   MT[55] = 0x80;

   for(i=56;i<62;i++)
      MT[i] = 0x00;

   MT[62] = 0x01;
   MT[63] = 0xB8;

   ComputeSHAEE(MT,&A,&B,&C,&D,&E);

   Temp = E;
   for(i=0;i<4;i++)
   {
      secret[i] = (uchar) (Temp & 0x000000FF);
//      printf("%02X",secret[i]);
      Temp >>= 8;
   }
//   printf("\n");

   Temp = D;
   for(i=0;i<4;i++)
   {
      secret[i+4] = (uchar) (Temp & 0x000000FF);
//      printf("%02X",secret[i+4]);
      Temp >>= 8;
   }
//   printf("\n");

   // access the device
   if (SelectSHAEE(portnum) == 1)
   {
      // Next Secret command
      send_block[send_cnt++] = 0x33;

      // address 1
      send_block[send_cnt++] = (uchar)(address & 0xFF);
      // address 2
      send_block[send_cnt++] = (uchar)((address >> 8) & 0xFF);


      if(owBlock(portnum,FALSE,send_block,send_cnt))
      {
         msDelay(12);

         if(ReadScratchSHAEE(portnum, &tmpadd, &es, scratch))
         {
            for(i=0;i<8;i++)
               if(scratch[i] != 0xAA)
               {
                  printf("ERROR IN SCRATCHPAD DATA BEING 0xAA\n");
                  return FALSE;
               }

            return TRUE;
         }
      }
   }

   return FALSE;

}


//----------------------------------------------------------------------
// Read Authenticated Page.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - address of page to do a read authenticate
// 'secret'      - current secret
// 'SerNum'      - serial number for the part
// 'data'        - buffer to how data read from page
// 'challenge'   - the challenge that is to be written to the scratchpad
//
// Return: TRUE - read sucessfull
//         FALSE - CRC error, device not present
//
int ReadAuthPageSHAEE(int portnum, ushort address, uchar *secret, uchar *SerNum,
                    uchar *data, uchar *challenge)
{
   short send_cnt=0;
   uchar send_block[80];
   int i;
   long A,B,C,D,E;
   long Temp;
   unsigned MT[64];
   uchar memory[32],scratch[8],es;
   ushort tmpadd;
   ushort lastcrc16;

   if(!WriteScratchSHAEE(portnum,address,challenge,8))
      return FALSE;

   for(i=0;i<4;i++)
      MT[i] = secret[i];

   if(ReadMem(portnum,(ushort)((address/((ushort)32))*32),memory))
      for(i=4;i<36;i++)
         MT[i] = memory[i-4];
   else
   {
      printf("ERROR DUE TO READING MEMORY PAGE DATA\n");
      return FALSE;
   }

   for(i=36;i<40;i++)
      MT[i] = 0xFF;

   if(ReadScratchSHAEE(portnum, &tmpadd, &es, scratch))
   {

      MT[40] = (0x40 | (uchar)(((tmpadd << 3) & 0x08) |
                       (uchar)((tmpadd >> 5) & 0x07)));

      for(i=41;i<48;i++)                                                                              
         MT[i] = SerNum[i-41];
      for(i=48;i<52;i++)
         MT[i] = secret[i-44];
      for(i=52;i<55;i++)
         MT[i] = scratch[i-48];
   }
   else
   {
      printf("ERROR DUT TO READING SCRATCH PAD DATA\n");
      return FALSE;
   }

   MT[55] = 0x80;

   for(i=56;i<62;i++)
      MT[i] = 0x00;

   MT[62] = 0x01;
   MT[63] = 0xB8;

   ComputeSHAEE(MT,&A,&B,&C,&D,&E);

   // access the device
   if (SelectSHAEE(portnum) == 1)
   {
      setcrc16(portnum,0);

      // Read Authenticated Command
      send_block[send_cnt] = 0xA5;
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // address 1
      send_block[send_cnt] = (uchar)(tmpadd & 0xFF);
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
      // address 2
      send_block[send_cnt] = (uchar)((tmpadd >> 8) & 0xFF);
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

      // data + FF byte
      for (i = 0; i < 35; i++)
         send_block[send_cnt++] = 0xFF;

      // now send the block
      if (owBlock(portnum,FALSE,send_block,send_cnt))
      {
         // calculate CRC16 of result
         for (i = 3; i < send_cnt; i++)
            lastcrc16 = docrc16(portnum,send_block[i]);

         // verify CRC16 is correct
         if (lastcrc16 != 0xB001)
         {
            printf("FIRST CRC TEST FAILED %04X\n",lastcrc16);
            return FALSE;
         }

         for(i=3;i<35;i++)
            data[i-3] = send_block[i];

         send_cnt = 0;
         for(i=0;i<22;i++)
            send_block[send_cnt++] = 0xFF;

         if(owBlock(portnum,FALSE,send_block,send_cnt))
         {
            // calculate CRC16 of result
            setcrc16(portnum,0);
            for (i = 0; i < send_cnt ; i++)
               lastcrc16 = docrc16(portnum,send_block[i]);

            // verify CRC16 is correct
            if (lastcrc16 != 0xB001)
            {
               printf("SECOND CRC TEST FAILED %04X\n",lastcrc16);
               return FALSE;
            }

            send_cnt = 0;
            Temp = E;
            for(i=0;i<4;i++)
            {
               if(send_block[send_cnt++] != (uchar) (Temp & 0x000000FF))
               {
                  printf("COMPARING MAC FAILED\n");
                  return FALSE;
               }
               Temp >>= 8;
            }

            Temp = D;
            for(i=0;i<4;i++)
            {
               if(send_block[send_cnt++] != (uchar) (Temp & 0x000000FF))
               {
                  printf("COMPARING MAC FAILED\n");
                  return FALSE;
               }
               Temp >>= 8;
            }

            Temp = C;
            for(i=0;i<4;i++)
            {
               if(send_block[send_cnt++] != (uchar) (Temp & 0x000000FF))
               {
                  printf("COMPARING MAC FAILED\n");
                  return FALSE;
               }
               Temp >>= 8;
            }

            Temp = B;
            for(i=0;i<4;i++)
            {
               if(send_block[send_cnt++] != (uchar) (Temp & 0x000000FF))
               {
                  printf("COMPARING MAC FAILED\n");
                  return FALSE;
               }
               Temp >>= 8;
            }

            Temp = A;
            for(i=0;i<4;i++)
            {
               if(send_block[send_cnt++] != (uchar) (Temp & 0x000000FF))
               {
                  printf("COMPARING MAC FAILED\n");
                  return FALSE;
               }
               Temp >>= 8;
            }

            return TRUE;
         }

      }
   }

   return FALSE;
}



//--------------------------------------------------------------------------
// Select the current device and attempt overdrive if possible.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Return: TRUE - device selected
//         FALSE - device not select
//
int SelectSHAEE(int portnum)
{
   int rt,cnt=0;

   // loop to access the device and optionally do overdrive
   do
   {
      rt = owVerify(portnum,FALSE);

      // check not present
      if (rt != 1)
      {
         // if in overdrive, drop back
         if (in_overdrive[portnum])
         {
            // set to normal speed
            if(MODE_NORMAL != owSpeed(portnum,MODE_NORMAL))
            {
               OWERROR(OWERROR_WRITE_BYTE_FAILED);
               return FALSE;
            }
            in_overdrive[portnum] = FALSE;
         }
      }
      // present but not in overdrive
      else if (!in_overdrive[portnum])
      {
         // put all devices in overdrive
         if (owTouchReset(portnum))
         {
            if (owWriteByte(portnum,0x3C))
            {
               // set to overdrive speed
               if(MODE_OVERDRIVE != owSpeed(portnum,MODE_OVERDRIVE))
               {
                  OWERROR(OWERROR_WRITE_BYTE_FAILED);
                  return FALSE;
               }
               in_overdrive[portnum] = TRUE;
            }
         }
         rt = 0;
      }
      else
         break;
   }
   while ((rt != 1) && (cnt++ < 3));

   return rt;
}


//----------------------------------------------------------------------
// Compute the 160-bit MAC
//
// 'MT'  - input data
// 'A'   - part of the 160 bits
// 'B'   - part of the 160 bits
// 'C'   - part of the 160 bits
// 'D'   - part of the 160 bits
// 'E'   - part of the 160 bits
//
// Note: This algorithm is the SHA-1 algorithm as specified in the
// datasheet for the DS1961S, where the last step of the official
// FIPS-180 SHA routine is omitted (which only involves the addition of
// constant values).
//
//
void ComputeSHAEE(unsigned int *MT,long *A,long *B, long *C, long *D,long *E)
{
   unsigned long MTword[80];
   int i;
   long ShftTmp;
   long Temp;

   for(i=0;i<16;i++)
      MTword[i] = (MT[i*4] << 24) | (MT[i*4+1] << 16) |
                  (MT[i*4+2] << 8) | MT[i*4+3];

   for(i=16;i<80;i++)
   {
      ShftTmp = MTword[i-3] ^ MTword[i-8] ^ MTword[i-14] ^ MTword[i-16];
      MTword[i] = ((ShftTmp << 1) & 0xfffffffe) |
                  ((ShftTmp >> 31) & 0x00000001);
   }

   *A=0x67452301;
   *B=0xefcdab89;
   *C=0x98badcfe;
   *D=0x10325476;
   *E=0xc3d2e1f0;

   for(i=0;i<80;i++)
   {
      ShftTmp = ((*A << 5) & 0xffffffe0) | ((*A >> 27) & 0x0000001f);
      Temp = NLF(*B,*C,*D,i) + *E + KTN(i) + MTword[i] + ShftTmp;
      *E = *D;
      *D = *C;
      *C = ((*B << 30) & 0xc0000000) | ((*B >> 2) & 0x3fffffff);
      *B = *A;
      *A = Temp;
   }
}

// calculation used for the MAC
long KTN (int n)
{
   if(n<20)
      return 0x5a827999;
   else if (n<40)
      return 0x6ed9eba1;
   else if (n<60)
      return 0x8f1bbcdc;
   else
      return 0xca62c1d6;
}

// calculation used for the MAC
long NLF (long B, long C, long D, int n)
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



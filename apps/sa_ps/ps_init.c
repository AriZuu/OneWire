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
// ps_init.c - main function for initializing software authentication
//             application.
//

#include "ownet.h"
#include "ps02.h"
#include "findtype.h"

// file where the hard code data is for authentication
#define MAXDEVICES  4

// local functions
void     ComputeSHAVM(uchar* MT, long* hash);
long     NLF (long B, long C, long D, int n);
SMALLINT ConvertPS(int portnum, uchar *MT, int key, int len);
int getNumber (int min, int max);


//constants used in SHA computation
static const long KTN[4] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };


int main(int argc, char** argv)
{
   int    i = 0;
   int    portnum = 0;
   int    length;
   uchar  MT[96];

   puts("\nStarting Software Authentication setup Application\n");

   // check for required port name
   if (argc != 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }

   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      printf("Failed to acquire port.\n");
      exit(1);
   }


   printf("Enter the 8 byte secret you want to use for the data.\n");
   printf("Data Entry Mode\n");
   printf("(0) Text (single line)\n");
   printf("(1) Hex (XX XX XX XX ...)\n");
   length = getData(&MT[0],8,getNumber(0,1));
   if(length < 8)
   {
      for(i=length;i<8;i++)
         MT[i] = 0x00;
   }

   printf("Enter the 8 byte ID you want to use for the data.\n");
   printf("Data Entry Mode\n");
   printf("(0) Text (single line)\n");
   printf("(1) Hex (XX XX XX XX ...)\n");
   length = getData(&MT[8],8,getNumber(0,1));
   if(length < 8)
   {
      for(i=length;i<8;i++)
         MT[i+8] = 0x00;
   }

   printf("Enter the 45 byte data you want to use.\n");
   printf("Data Entry Mode\n");
   printf("(0) Text (single line)\n");
   printf("(1) Hex (XX XX XX XX ...)\n");
   length = getData(&MT[16],45,getNumber(0,1));
   if(length < 45)
   {
      for(i=length;i<45;i++)
         MT[i+16] = 0x00;
   }

   // initialize the block codes for the DS1991
   initBlockCodes();

   // for the first secret, id and data
   if(!ConvertPS(portnum,MT,0,length))
      OWERROR_DUMP(stderr);

   // for the second secret, id and data
   if(!ConvertPS(portnum,MT,1,length))
      OWERROR_DUMP(stderr);

   // for the third secret, id and data
   if(!ConvertPS(portnum,MT,2,length))
      OWERROR_DUMP(stderr);

   // release the port
   owRelease(portnum);

   return 1;
}

//----------------------------------------------------------------------
// computes a SHA given the 64 byte MT digest buffer.  The resulting 5
// long values are stored in the given long array, hash.
//
// Note: This algorithm before's the SHA-1 algorithm as specified in the
// datasheet for the DS1961S, where the last step (which only involves
// math with constant values) is omitted.
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

// calculation used for the SHA MAC
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

SMALLINT ConvertPS(int portnum,uchar *MT, int key, int len)
{
   int    i,j;
   long   temp;
   long   hash[5];
   uchar  secret[8];
   uchar  ID[8];
   uchar  psxor[4];
   uchar  oldName[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
   uchar  data[64];
   ushort crc;
   int     NumDevices;
   uchar   AllSN[MAXDEVICES][8];
   uchar   tempMT[64];
   uchar   MTtwo[64];

   for(i=0;i<64;i++)
   {
      tempMT[i] = MT[i];
      MTtwo[i]  = MT[i];
   }

   MTtwo[16] = (uchar) len;
   for(i=16;i<len+16;i++)
      MTtwo[i+1] = tempMT[i];
   setcrc16(portnum,(ushort)key);
   for(i=0;i<len+1;i++)
   {
      crc = docrc16(portnum,MTtwo[i+16]);
   }
   MTtwo[len+17] = (uchar) ~crc & 0xFF;
   MTtwo[len+18] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   tempMT[16] = 0x2D;
   for(i=17;i<62;i++)
   {
      tempMT[i]      = (uchar) i;
   }

   setcrc16(portnum,(ushort)key);
   for(i=16;i<62;i++)
      crc = docrc16(portnum,tempMT[i]);
   tempMT[62] = (uchar) ~crc & 0xFF;
   tempMT[63] = (uchar) ((~crc & 0xFFFF) >> 8) & 0xFF;

   ComputeSHAVM(tempMT,hash);

   temp = hash[0];
   for(i=0;i<4;i++)
   {
      secret[i] = (uchar) temp;
      tempMT[i+8]     = secret[i];
      temp >>= 8;
   }
   temp = hash[1];
   for(i=4;i<8;i++)
   {
      secret[i] = (uchar) temp;
      tempMT[i+8]     = secret[i];
      temp >>= 8;
   }
   temp = hash[2];
   for(i=0;i<4;i++)
   {
      ID[i]   = (uchar) temp;
      tempMT[i] = ID[i];
      temp >>= 8;
   }
   temp = hash[3];
   for(i=4;i<8;i++)
   {
      ID[i]    = (uchar) temp;
      tempMT[i] = ID[i];
      temp >>= 8;
   }
   temp = hash[4];
   for(i=0;i<4;i++)
   {
      psxor[i] = (uchar) temp;
      for(j=0;j<12;j++)
      {
         tempMT[i*12+j+16] = (~psxor[i] &  MTtwo[i*12+j+16]) |
                             (psxor[i]  & ~MTtwo[i*12+j+16]);
      }
      temp >>= 8;
   }

   NumDevices = FindDevices(portnum,AllSN,0x02,MAXDEVICES);

   if(NumDevices == 0)
   {
      printf("Please place the DS1991 on the 1-Wire Network.\n");
      for(;;)
      {
         NumDevices = FindDevices(portnum,AllSN,0x02,MAXDEVICES);
         if(NumDevices > 0)
         {
            msDelay(10);
            break;
         }
      }
   }

   for(i=0;i<NumDevices;i++)
   {
      owSerialNum(portnum,&AllSN[i][0],FALSE);

      if(!writePassword(portnum,key,oldName,ID,secret))
      {
         return FALSE;
      }

      if(writeScratchpad(portnum,0,tempMT,64))
      {
         if(readScratchpad(portnum,data))
         {
            for(j=0;j<64;j++)
            {
               if(tempMT[j] != data[j])
               {
                  OWERROR(OWERROR_READ_SCRATCHPAD_VERIFY);
                  return FALSE;
               }
            }
         }
         else
            return FALSE;

         if(!copyScratchpad(portnum,key,secret,8))
            return FALSE;

/*  The write sub key is commented out so the
    write scratchpad, read scratchpad and copy
    scracthpad may be use to copy the data into
    the subkey.

      if(!writeSubkey(portnum,key,16,secret,data,48))
      {
         OWERROR_DUMP(stderr);
         printf("the writing of the data didn't work.\n");
      }
      else
      {
*/
         if(!readSubkey(portnum,&data[0],key,secret))
         {
            return FALSE;
         }
         else
         {
            for(j=0;j<64;j++)
            {
               if(tempMT[j] != data[j])
               {
                  OWERROR(OWERROR_READ_VERIFY_FAILED);
                  return FALSE;
               }
            }
         }
      }
      else
         return FALSE;
   }

   return TRUE;
}

/**
 * Retrieve user input from the console.
 *
 * min  minimum number to accept
 * max  maximum number to accept
 *
 * @return numeric value entered from the console.
 */
int getNumber (int min, int max)
{
   int value = min,cnt;
   int done = FALSE;
   do
   {
      cnt = scanf("%d",&value);
      if(cnt>0 && (value>max || value<min))
      {
         printf("Value (%d) is outside of the limits (%d,%d)\n",value,min,max);
         printf("Try again:\n");
      }
      else
         done = TRUE;

   }
   while(!done);

   return value;

}

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
// tm_check.c - A sample program showing how to use the DS1994/DS1427/DS2404
//              to secure a software package and tie it to a timed license
//              held in the 1-Wire device, basically providing a software timed
//              trial.
//
//              This program assumes that the 1-Wire device has been initialized
//              appropriately by the tm_init program, and continuously polls
//              the 1-Wire device to check for a valid license
//              and/or part expiration.  See pseudocode specification at the
//              bottom of this file.
//

#include <stdio.h>
#include <stdlib.h>
#include "ownet.h"
#include "findtype.h"
#include "owfile.h"
#include "time04.h"
#include "memory.h"
#include "string.h"

// defines
#define MAXDEVICES    4
#define MAXTIMEOFFSET 378691201                // about 12 years (should exceed battery life).

// error defines
#define NO_ERROR   1
#define NO_DEVICE  2
#define NO_FILES   3

// local functions
void     ComputeSHAVM(uchar* MT, long* hash);  // function to compute sha-1
long     NLF2 (long B, long C, long D, int n); // used by above function to compute sha-1
int      getNumber (int min, int max);         // help with user input

//constants used in SHA computation to create the new MAC
static const long KTN[4] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

int main(int argc, char** argv)
{
   int        i = 0;
   int        j = 0;
   int        portnum = 0;             // port number where 1-Wire net is located
   int        num = 0;                 // number of 1-Wire Clock devices found on 1-Wire Net
   timedate   td;                      // timedate struct
   ulong      RTCTime = 0;             // variable to hold the RTC time in seconds.
   ulong      RTCATime = 0;            // variable to hold the RTCA time in seconds.
   ulong      SoftTime = 0;            // get "Soft" time with this value.
   uchar      SoftTimeArray[5];        // holds the soft time in an array of uchar
   uchar      secret[51];              // holds the secret input by the user.
   int        secretlength;            // secret length in number of bytes.
   uchar      usertext[51];            // holds the usertext retrieved from 1-Wire file.
   int        usertextlength;          // user text length in number of bytes
   uchar      inputforsha[64];         // ROM + UserText + SOFTTIME + Secret
   long       longMAC[5];              // MAC resulting from SHA1 hash as 5 ulong integers
   uchar      MAC[20];                 // MAC resulting from SHA1 hash of inputforsha array as a byte array
   uchar      newMAC[20];              // MAC constructed from ROM + UserText + SOFTTIME + Secret
   uchar      filedata[76];            // file data
   int        filedatalength = 0;      // length of file data
   short      filehandle = 0;          // handle to 1-Wire file
   SMALLINT   comparisonerror = FALSE; // boolean for comparing old and new MACs
   FileEntry  fe;                      // 1-Wire file info used to create a 1-Wire file
   SMALLINT   errordetect = NO_ERROR;  // flag when an error occurrs in communicating to 1-Wire device
   uchar      TimeSN[MAXDEVICES][8];   // the serial numbers (1-Wire Net Addresses) for the devices
   ulong      delayTime = 0;           // delay between main loop iterations.
   ulong      noDeviceDelayTime = 1000;// delay between no-device loop iterations.

   // Make sure we can talk to the part when it is alarming.
   // The following variable should always be set when
   // communicating to an alarming 1-Wire Clock device.
   FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = TRUE;

   // initialize timedate struct
   td.day = 0;
   td.hour = 0;
   td.minute = 0;
   td.month = 0;
   td.second = 0;
   td.year = 0;

   // initialize secret and usertext to be full of zeroes
   memset(&secret[0], 0x00, 51);
   memset(&usertext[0], 0x00, 51);

   memset(&SoftTimeArray[0], 0x00, 5);
   memset(&inputforsha[0], 0x00, 64);
   memset(&longMAC[0], 0x00, 20);
   memset(&MAC[0], 0x00, 20);
   memset(&newMAC[0], 0x00, 20);
   memset(&filedata[0], 0x00, 76);

   // initialize file entry.
   // Name the file and extension to read in FileEntry struct.
   fe.Name[0] = 0x45; //'E'
   fe.Name[1] = 0x58; //'X'
   fe.Name[2] = 0x50; //'P'
   fe.Name[3] = 0x20; // blank
   fe.Ext = 0x00;

   puts("\nStarting 'Timed Trial' checking program for 1-Wire Clocks\n");

   // check for required port name
   if (argc != 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }
   // acquire the port
   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      printf("Failed to acquire port.\n");
      exit(1);
   }
   // success
   printf("Port opened: %s\n",argv[1]);

   // prompt the user to enter the secret
   printf("\nPlease enter the secret, either as text or in hexadecimal.\n\n");
   printf("Data Entry Mode\n");
   printf("(0) Text (single line)\n");
   printf("(1) Hex (XX XX XX XX ...)\n");
   secretlength = getData(&secret[0],51,getNumber(0,1));

   // get list of 1-Wire Clock devices
   num = FindDevices(portnum, &TimeSN[0],TIME_FAM, MAXDEVICES);

   // check if more than 1 present
   if (num > 1)
   {
	   printf("\nMore than one 1-Wire clock discovered, picking ");
      printf("\nthe first device found.\n");
   }

   // loop to read 1-Wire part and display status.
   while (TRUE)
   {
      // put in an extra return
      printf("\n");

      // get list of 1-Wire Clock devices
      num = FindDevices(portnum, &TimeSN[0],TIME_FAM, MAXDEVICES);

      // check if Clock not present
      if (num == 0)
      {
         printf("NO DEVICE PRESENT");
         msDelay((int)noDeviceDelayTime);
         continue;
      }

      // read RTC and RTCA
      if (!getRTC(portnum, &TimeSN[0][0], &td)) // read RTC
      {
         errordetect = NO_DEVICE;
      }
      RTCTime = DateToSeconds(&td); // convert timedate struct to number of seconds
      if (!getRTCA(portnum, &TimeSN[0][0], &td)) // read RTCA
      {
         errordetect = NO_DEVICE;
      }
      RTCATime = DateToSeconds(&td);

      // retrieve file from part.
      if(!owOpenFile(portnum,&TimeSN[0][0],&fe,&filehandle))
      {
         errordetect = NO_FILE;
      }
      else if(!owReadFile(portnum,&TimeSN[0][0],filehandle,&filedata[0],76,&filedatalength))
      {
         errordetect = NO_FILE;
      }
      else if(!owCloseFile(portnum,&TimeSN[0][0],filehandle))
      {
         errordetect = NO_FILE;
      }

      // deal with NO_DEVICE error
      if (errordetect == NO_DEVICE || RTCTime < 1)  // some bad RTC reads give RTCTime as 0.
      {
         printf("NO DEVICE PRESENT");
         errordetect = NO_ERROR;
         msDelay((int)noDeviceDelayTime);
         continue;
      }

      // print out the 1-Wire net address of clock
      i = 0;
      for (j = 7; j >= 0; j--)
      {
         printf("%02X",TimeSN[i][j]);
      }
      printf(",");

      // deal with NO_FILE error
      if (errordetect == NO_FILE)
      {
         printf(" Token NOT VALID, no license file");
         errordetect = NO_ERROR;
         msDelay((int)delayTime);
         continue;
      }

      usertextlength = filedatalength - 25;  // 25 is MAC length (20) plus SoftTime length (5)

      // parse file to retrieve the user text, the soft expiration time, and the MAC.
      memcpy(&usertext[0], &filedata[0], usertextlength);      // get usertext
      memcpy(&SoftTimeArray[0], &filedata[usertextlength], 5); // get SoftTime as bytes
      memcpy(&MAC[0], &filedata[(usertextlength + 5)], 20);    // get MAC from file as 20 bytes

      // Construct a new MAC with  ROM + UserText + SOFTTIME + Secret =>(SHA)=>  newMAC
      // First, build 64-byte inputforsha array: ROM + UserText + SOFTTIME + Secret
      memcpy(&inputforsha[0],&TimeSN[0][0], 8);
      memcpy(&inputforsha[8],&usertext[0], usertextlength);
      memcpy(&inputforsha[(usertextlength + 8)],&SoftTimeArray[0], 5);
      SoftTime = uchar_to_bin(&SoftTimeArray[1], 4); // convert SoftTime to number of seconds
      memcpy(&inputforsha[(usertextlength + 13)], &secret, secretlength);

      // Create the SHA1 MAC from the inputforsha array
      ComputeSHAVM(&inputforsha[0], &longMAC[0]);

      // Make new MAC as a byte array from the longMAC long integer array
      memcpy(&newMAC[0], &longMAC[0], 20);

      // Compare old MAC from clock to new MAC against the entered secret
      comparisonerror = FALSE;
      for (i = 0; i < 20; i++)
      {
         if (MAC[i] != newMAC[i]) comparisonerror = TRUE;
      }
      if (comparisonerror == TRUE)
      {
         printf(" Token NOT VALID, secret must be different");
         comparisonerror = FALSE; // reset error
      }
      else
      {
         printf(" Token VALID,");

         if (RTCTime < SoftTime)
         {
            printf(" License VALID, ");
            PrintChars(&usertext[0], usertextlength);
         }
         else
         {
            printf(" License EXPIRED");
            if (RTCTime >= RTCATime)
            {
               printf(", Device EXPIRED");
            }
         }
      }

      // reset errordetect
      errordetect = NO_ERROR;
      msDelay((int)delayTime);
   }

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
// Parameters:
//  MT         - pointer to a buffer containing the message digest
//  hash       - resulting buffer
//
// Returns:
//             - see hash variable above
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
      Temp = NLF2(hash[1],hash[2],hash[3],i) + hash[4]
               + KTN[i/20] + MTword[i] + ShftTmp;
      hash[4] = hash[3];
      hash[3] = hash[2];
      hash[2] = ((hash[1] << 30) & 0xC0000000) | ((hash[1] >> 2) & 0x3FFFFFFF);
      hash[1] = hash[0];
      hash[0] = Temp;
   }
}

//----------------------------------------------------------------------
// calculation used for the SHA MAC
//
long NLF2 (long B, long C, long D, int n)
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
// Retrieve user input from the console.
//
// Parameters:
//  min  minimum number to accept
//  max  maximum number to accept
//
// Returns:
//  numeric value entered from the console.
//
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

//-------------------------------------------------------------------
//
//       PSEUDOCODE SPECIFICATION:
//
//         TM_Check - Prompt for Secret. Check for valid token present
//         by doing the following:
//
//         loop
//         {
//            if (DS1994 PRESENT)
//            {
//               print ROM
//
//               Read Real-Time Clock (RTC) and Alarm (RTCA)
//               Read EXP.000 and extract: UserText, SOFTTIME and MAC
//
//               if (failed to read EXP.000)
//               {
//                  print 'Token NOT VALID, no license file'
//                  continue
//               }
//
//               Construct a new MAC with
//                    ROM + UserText + SOFTTIME + Secret =>(SHA)=>  MAC'
//
//               if (MAC != MAC')
//                  print 'Token NOT VALID, secret must be different'
//               else
//               {
//                  print 'Token VALID'
//
//                  if (RTC < SOFTTIME)
//                      print 'License VALID' + UserText
//                  else
//                      print 'License EXPIRED'
//
//                  if (RTC >= RTCA)
//                      print 'Device EXPIRED'
//               }
//            }
//            else
//               print 'NO DEVICE PRESENT'
//         }
//
//         So the possible output may be:
//
//         NO DEVICE PRESENT
//         NO DEVICE PRESENT
//         5400000011227704, Token NOT VALID, no license file
//         NO DEVICE PRESENT
//         NO DEVICE PRESENT
//         1100000055266704, Token NOT VALID, secret must be different
//         NO DEVICE PRESENT
//         D7000000068C7A04, Token VALID, License VALID, MyTextGoesHere
//         D7000000068C7A04, Token VALID, License VALID, MyTextGoesHere
//         D7000000068C7A04, Token VALID, License VALID, MyTextGoesHere
//         D7000000068C7A04, Token VALID, License EXPIRED
//         D7000000068C7A04, Token VALID, License EXPIRED
//         D7000000068C7A04, Token VALID, License EXPIRED
//         D7000000068C7A04, Token VALID, License EXPIRED, Device EXPIRED
//

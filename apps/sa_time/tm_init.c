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
// tm_init.c - program for initializing software authentication "Timed Trial"
//             application using specific 1-Wire clocks (DS1994/DS1427/DS2404).
//
//             Once the clock is setup with this program, then the companion
//             program, tm_check, can be run to verify the software license.
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
#define MAXDEVICES  4
#define MAXTIMEOFFSET 378691201      // about 12 years.

// External functions
extern void owClearError(void);

// local functions
void     ComputeSHAVM(uchar* MT, long* hash);
long     NLF2 (long B, long C, long D, int n);
int      getNumber (int min, int max);

//constants used in SHA computation
static const long KTN[4] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

/*
         TM_Init - Prompt for two expiration time limits.
         The first is a hard limit that will 'expire' the device.
         This is the value that will go into the Clock Alarm register.
         The second expiration is a soft limit that will provide
         a license period that does not expire the device.  This
         soft expiration will be a SHA validated value in a
         memory file in the DS1994.  Here is how it will work:

          1. Prompt for 'hard' device expire limit
             (seconds for demo purposes)
          2. Prompt for 'soft' license expire limit
          3. Prompt for Secret
          4. Prompt for User text
             (size limit =  64 - 8 - 5 - secretsize)
          5. Set the Real-Time clock (RTC) to the PC's time
          6. Set the Clock alarm (RTCA) to the 'hard' limit
             (Real-Time + hard)
          7. Write a SHA validated 'soft' limit in a file.
             Create MAC with:
                 ROM + UserText + SOFTTIME + Secret =>(SHA)=>  MAC
             Write UserText + SOFTTIME + MAC in a file called EXP.000
          8. Optionally set the Real-Time Clock LOCK bit
             (command line switch???)
			 51 + 5 + 20
*/

int main(int argc, char** argv)
{
   int    i = 0;
   int    j = 0;
   char   wpparameter[6];    // command line parameter to see if clock should be write-protected
   SMALLINT wpenable = FALSE;// if true, then the part gets write-protected
   int    portnum = 0;       // the port on which to communicate to 1-Wire devices
   int    num = 0;           // number of 1-Wire Clock devices
   timedate td;              // timedate struct
   ulong  RTCAOffset = 180;  // Real-Time Clock Alarm offset from PC time in seconds (entered by user)
   ulong  SoftOffset = 120;  // "Soft" clock offset used to calculate expiry time to be placed in RAM of the part.
   ulong RTCTime = 0;        // set RTC with this time in seconds.
   ulong RTCATime = 0;       // set RTCA with this time in seconds.
   ulong SoftTime = 0;       // set "Soft" time with this value.
   uchar SoftTimeArray[5];   // holds the soft time in an array of uchar
   uchar secret[51];         // holds the secret input by the user.
   int secretlength;         // secret length in number of bytes.
   uchar usertext[51];       // holds the usertext input by the user.
   int usertextlength;       // user text length in number of bytes
   uchar inputforsha[64];    // ROM + UserText + SOFTTIME + Secret
   long  longMAC[5];         // MAC resulting from SHA1 hash with
   uchar MAC[20];            // MAC resulting from SHA1 hash of inputforsha array
   uchar filedata[76];       // file data
   int filedatalength = 76;  // length of file data
   short filehandle = 0;     // handle to 1-Wire file
   int maxwrite;             // maximum number of bytes
   FileEntry fe;             // 1-Wire file info used to create a 1-Wire file


   uchar TimeSN[MAXDEVICES][8]; // array to hold the serial numbers for the devices found
   SMALLINT RTCAlarmTriggerBit = 0;
   ulong secondsfromdate =0;

   // Make sure we can talk to the part when it is alarming.
   // The following variable should always be set when
   // wanting to communicate to an alarming 1-Wire Clock device.
   FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = TRUE;

   // initialize timedate struct
   td.day = 0;
   td.hour = 0;
   td.minute = 0;
   td.month = 0;
   td.second = 0;
   td.year = 0;

   // initialize secret and usertext to be full of zeroes
   memset(&wpparameter[0], 0x00, 6);
   memset(&secret[0], 0x00, 51);
   memset(&usertext[0], 0x00, 51);
   memset(&SoftTimeArray[0], 0x00, 5);
   memset(&inputforsha[0], 0x00, 64);
   memset(&longMAC[0], 0x00, 20);
   memset(&filedata[0], 0x00, 76);

   puts("\nStarting 'Timed Trial' setup Application for 1-Wire Clocks\n");

   // check for required port name
   if (argc < 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }

   if (argc == 3)    // possible write-protect switch
   {
      sprintf(wpparameter, "-wp");
      if(strcmp(argv[2], wpparameter) == 0)
      {
         wpenable = TRUE;  // the program will try to write-protect the clock
      }
   }

   // acquire the port
   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      printf("Failed to acquire port.\n");
      exit(1);
   }

   // success
   printf("Port opened: %s\n",argv[1]);

   // get list of 1-Wire Clock devices
   num = FindDevices(portnum, &TimeSN[0], TIME_FAM, MAXDEVICES);

   // check if not present or more than 1 present
   if (num == 0)
      ExitProg("1-Wire Clock not present on 1-Wire\n",1);

   if (num > 1)
   {
	   printf("\nMore than 1 1-Wire clock discovered, picking ");
      printf("\nthe first device found.\n");
   }

   // print out the 1-Wire net address of clock
   printf("\n1-Wire Address of clock found:  ");
   i = 0;
   for (j = 7; j >= 0; j--)
   {
      printf("%02X",TimeSN[i][j]);
   }
   printf("\n\n");

   // prompt the user for RTCA info
   printf("The first step is to set the clock alarm on the 1-Wire device.\n");
   printf("The Real-Time Clock Alarm (RTCA) value, 'hard' alarm, will be \n");
   printf("an offset from the PC's clock in seconds.\n\n");

   // retrieve the real-time clock offset
   EnterNum("Please enter the 'hard' offset value: ", 10, &RTCAOffset, 1,MAXTIMEOFFSET);
   printf("\nThe number entered is:  %i\n", RTCAOffset);

   // retrieve the "soft" expiration date offset
   printf("\nThe second step is to set an alarm time to be written to the part.\n");
   printf("The 'soft' alarm value will also be an offset from the PC's clock\n");
   printf("in seconds.  Ideally, it should be less than the RTCA offset.\n\n");
   EnterNum("Please enter the 'soft' offset value: ", 10, &SoftOffset, 1, MAXTIMEOFFSET);
   printf("\nThe number entered is:  %i\n", SoftOffset);

   // retrieve the secret
   printf("\nThe third step is to provide a secret, either as text or in hexadecimal.\n\n");
   printf("Please enter the secret (up to 51 bytes)\n");
   printf("Data Entry Mode\n");
   printf("(0) Text (single line)\n");
   printf("(1) Hex (XX XX XX XX ...)\n");
   secretlength = getData(&secret[0],51,getNumber(0,1));

   // retrieve the user text
   printf("\nThe fourth step is to provide user text to be stored on the device.\n");
   printf("It is general purpose text to be used, for example, as configuration\n");
   printf("information for a software application. \n\n");
   printf("Please enter the user text (up to %i bytes)\n",(51-secretlength));
   printf("Data Entry Mode\n");
   printf("(0) Text (single line)\n");
   printf("(1) Hex (XX XX XX XX ...)\n");
   usertextlength = getData(&usertext[0],(51 - secretlength),getNumber(0,1));

   // set the RTC and RTCA of 1-Wire device and calculate SoftTime
   getPCTime(&td);                       // get the time from the PC
   RTCTime = DateToSeconds(&td);         // convert time to seconds since Jan. 1, 1970
   if (!setRTC(portnum, &TimeSN[0][0], RTCTime, TRUE)) // set the RTC of 1-Wire device
   {
      printf("\nError in setting the Real-Time Clock, exiting program..\n");
	   OWERROR_DUMP(stdout);
      exit(1);
   }
   RTCATime = RTCTime + RTCAOffset;
   if (!setRTCA(portnum, &TimeSN[0][0], RTCATime, TRUE)) // set RTCA with offset
   {
      printf("\nError in setting the Real-Time Clock alarm, exiting program..\n");
	   OWERROR_DUMP(stdout);
      exit(1);
   }
   SoftTime = RTCTime + SoftOffset;         // calculate Soft time
   memcpy(&SoftTimeArray[1], &SoftTime, 4); // convert SoftTime to an array

   // build 64-byte inputforsha array: ROM + UserText + SOFTTIME + Secret
   memcpy(&inputforsha[0],&TimeSN[0][0], 8);
   memcpy(&inputforsha[8],&usertext[0], usertextlength);
   memcpy(&inputforsha[(usertextlength + 8)],&SoftTimeArray[0], 5);
   memcpy(&inputforsha[(usertextlength + 13)], &secret, secretlength);

   // Create the SHA1 MAC from the inputforsha array
   ComputeSHAVM(&inputforsha[0], &longMAC[0]);

   // Convert SHA1 results to a 20-byte array
   memcpy(&MAC[0], &longMAC[0], 20);

   // Write UserText + SOFTTIME + MAC in a file called EXP.000
   // Name the file and extension in FileEntry struct.
   fe.Name[0] = 0x45; //'E'
   fe.Name[1] = 0x58; //'X'
   fe.Name[2] = 0x50; //'P'
   fe.Name[3] = 0x20; // blank
   fe.Ext = 0x00;

   // Fill in the file buffer to be eventually written to Clock.
   memcpy(&filedata[0], &usertext, usertextlength);
   memcpy(&filedata[usertextlength], &SoftTimeArray, 5);
   memcpy(&filedata[(usertextlength + 5)], &MAC, 20);
   filedatalength = usertextlength + 5 + 20;

   // format the 1-Wire device, erasing previous file system
   if (!owFormat(portnum, &TimeSN[0][0]))
   {
      printf("\nError in formatting 1-Wire Clock's file system, exiting program..\n");
      OWERROR_DUMP(stdout);
	   exit(1);
   }

   // create file
   if (!owCreateFile(portnum, &TimeSN[0][0], &maxwrite, &filehandle, &fe))
   {
	   printf("\nError creating 1-Wire Clock's file system, exiting program..\n");
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // write the file
   if(!owWriteFile(portnum,&TimeSN[0][0],filehandle,&filedata[0],filedatalength))
   {
      printf("\nError writing a file to the 1-Wire Clock's file system, exiting program..\n");
      OWERROR_DUMP(stderr);
	   exit(1);
   }

   // write-protect the part
   if (wpenable)
   {
      // retrieve y or n from user
      printf("\nYou have selected to write-protect the clock and clock alarm.\n");
      printf("When the part alarms, the entire part will become read-only\n");
      printf("Are you sure you want to do this? (y or n) ");
      j = getkeystroke();
      if ((j == 0x79) || (j == 0x59))
      {
         if (!setWriteProtectionAndExpiration(portnum, &TimeSN[0][0], TRUE, TRUE))
         {
            printf("\nError trying to write-protect the Real-Time Clock, exiting program...\n");
            OWERROR_DUMP(stdout);
            exit(1);
         }
      }
   }

   printf("\n\nThe part has been successfully set-up!\n\n");

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

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
//--------------------------------------------------------------------------
//
//  jibmodpw.c - test teh  a JiBLet TestModPow on a Java Powered iButton
//
//  version 3.00 Beta
//

#include "jib96.h"
#include <string.h>
#include <time.h>

#define MAXDEVICES 10
#define LOAD_UNLOAD_TIME  500
#define EXECUTE_TIME      1500

// external functions
extern SMALLINT  owAcquire(int,char *);
extern void      owRelease(int);
extern SMALLINT  FindDevices(int,uchar FamilySN[][8],SMALLINT,int);
extern void      PrintSerialNum(uchar*);
extern SMALLINT  owOverdriveAccess(int);
extern void      owSerialNum(int,uchar *,SMALLINT);
extern int       key_abort(void);

// local prototypes
uchar *getRandData(uchar  p_Len, uchar *p_lpData);
uchar CycleTest(int portnum, uchar *dev);

// globals
int     g_NumCompletions  = 0,
        g_NumFailures     = 0;

//--------------------------------------------------------------------------
// Main for the Jib test
//
int main(int argc, char **argv)
{
   int i, portnum=0, NumDevices;
   uchar FamilySN[MAXDEVICES][8];

   // check for required port name
   if (argc != 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }

   // attempt to acquire the 1-Wire Net
   if ((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // success
   printf("Port opened: %s\n",argv[1]);

   // initialize the random number generator
   srand((unsigned)time(NULL));

   // Find the device(s)
   NumDevices = FindDevices(portnum, &FamilySN[0], 0x16, MAXDEVICES);

   if (NumDevices>0)
   {
      printf("\n");
      printf("JiB(s) Found: \n");
      for (i = 0; i < NumDevices; i++)
      {
         PrintSerialNum(FamilySN[i]);
         printf("\n");
      }
      printf("\n\n");

      do
      {
          // loop through the devices and do a test on each
         for (i = 0; i < NumDevices; i++)
            CycleTest(portnum, FamilySN[i]);
      }
      while (!key_abort());
   }
   else
      printf("No JiB's Found!\n");

   // release the 1-Wire Net
   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);
   return 0;
}

//--------------------------------------------------------------------------
// Get random numbers
//
uchar *getRandData(uchar  p_Len, uchar *p_lpData)
{
  int i;
  p_lpData[0] =  p_Len;

  for(i = 1; i <= p_Len; i++)
    p_lpData[i] = (uchar)rand();

  p_lpData[1] |= 0x80;
  return p_lpData;
}

//--------------------------------------------------------------------------
// This is a simple function to verify the coprocessor functions using
// the JiBlet TestModExp.jib.
// NOTE: TestModExp must be loaded and selected before executing this function.
//
uchar CycleTest(int portnum, uchar *dev)
{

   LPRESPONSEAPDU  l_lpResponseAPDU;
   uchar           l_RefBuffer[256];
   uchar           l_Message[129],
                   l_Modulus[129],
                   l_Exponent[129];
   uchar           l_i;

   // select the device and put in overdrive
   printf("\nDevice: ");
   PrintSerialNum(dev);
   printf("\n");

   owSerialNum(portnum,dev,0);
   if (!owOverdriveAccess(portnum))
      printf("ERROR, could not get device in overdrive!\n");

   // read the firmware string //
   l_lpResponseAPDU = GetFirmwareVersionString(portnum);

   if(l_lpResponseAPDU)
   {
      if(l_lpResponseAPDU->SW == (ushort)0x9000)
      {
         // trim out the length byte //
         memcpy(l_RefBuffer,l_lpResponseAPDU->Data+1,l_lpResponseAPDU->Len-1);

         // zero terminate the string //
         l_RefBuffer[l_lpResponseAPDU->Len-1] = 0;
         printf("Firmware: %s\n",l_RefBuffer);
      }
      else
         printf("GetFirmwareVersionString Error: SW = %04.4X",l_lpResponseAPDU->SW);
   }

   printf("Loading Message\n");
   l_lpResponseAPDU = Process(portnum, 0x80, 0x00, 0, 0, 128, getRandData(127,l_Message), LOAD_UNLOAD_TIME);

   if(l_lpResponseAPDU->SW != (ushort)0x9000)
   {
      printf("Load Message failed with SW = %04x\n",l_lpResponseAPDU->SW);
      return FALSE;
   }

   printf("Loading Exponent\n");
   l_lpResponseAPDU = Process(portnum, 0x80, 0x01, 0, 0, 128, getRandData(127,l_Exponent), LOAD_UNLOAD_TIME);

   if(l_lpResponseAPDU->SW != (ushort)0x9000)
   {
      printf("Load Exponent failed with SW = %04x\n",l_lpResponseAPDU->SW);
      return FALSE;
   }

   // make the modulus 1 byte larger than the exponent and message to insure that it is larger
   // without actually doing a "big number" compare (a quick cheat).
   printf("Loading Modulus\n");
   l_lpResponseAPDU = Process(portnum, 0x80, 0x02, 0, 0, 129, getRandData(128,l_Modulus), LOAD_UNLOAD_TIME);

   if(l_lpResponseAPDU->SW != (ushort)0x9000)
   {
      printf("Load Modulus failed with SW = %04x\n",l_lpResponseAPDU->SW);
      return FALSE;
   }

   printf("Performing Exponentiation\n");
   l_lpResponseAPDU = Process(portnum, 0x80, 0x03, 0, 0, 0, NULL, EXECUTE_TIME);

   if(l_lpResponseAPDU->SW != (ushort)0x9000)
   {
      printf("Exponentiation failed with SW = %04x\n",l_lpResponseAPDU->SW);
      return FALSE;
   }

   printf("Performing Read\n");
   l_lpResponseAPDU = Process(portnum, 0x80, 0x04, 0, 0, 0, NULL, LOAD_UNLOAD_TIME);

   if(l_lpResponseAPDU->SW != (ushort)0x9000)
   {
      printf("Read failed with SW = %04x\n",l_lpResponseAPDU->SW);
      return FALSE;
   }

   memcpy(l_RefBuffer,l_lpResponseAPDU->Data,l_lpResponseAPDU->Len);

   printf("Performing Verification\n");
   l_lpResponseAPDU = Process(portnum, 0x80, 0x03, 0, 0, 0, NULL, EXECUTE_TIME);

   if(l_lpResponseAPDU->SW != (ushort)0x9000)
   {
      printf("Verification failed with SW = %04x\n",l_lpResponseAPDU->SW);
      return FALSE;
   }

   printf("Performing Verification Read\n");
   l_lpResponseAPDU = Process(portnum, 0x80, 0x04, 0, 0, 0, NULL, LOAD_UNLOAD_TIME);

   if(l_lpResponseAPDU->SW != (ushort)0x9000)
   {
      printf("Read failed with SW = %04x\n",l_lpResponseAPDU->SW);
      return FALSE;
   }

   printf("Result: ");

   for(l_i = 0; l_i < l_lpResponseAPDU->Len; l_i++)
   {
    if(!(l_i%16))
      printf("\n");

    printf("%02X ",l_lpResponseAPDU->Data[l_i]);
   }

   printf("\n");
   g_NumCompletions++;

   if(!memcmp(l_RefBuffer,l_lpResponseAPDU->Data,l_lpResponseAPDU->Len))
   {
      printf("Buffer matches reference\nCompleted Iterations: %d, Failures: %d, Percent Failures: %2.2f\n",
              g_NumCompletions,g_NumFailures,(double)g_NumFailures/g_NumCompletions*100);
   }
   else
   {
      printf("Buffer does NOT match reference\nIterations: %d, Failures: %d\n",g_NumCompletions,++g_NumFailures);
   }

   return TRUE;
}


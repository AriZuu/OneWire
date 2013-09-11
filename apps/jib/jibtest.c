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
//  jibtest.c - Test each Java-powered iButton found on the 1-Wire
//
//  version 3.00B
//

#include "jib96.h"

#define MAXDEVICES 10

// external functions
extern SMALLINT  owAcquire(int,char *);
extern void      owRelease(int);
extern SMALLINT  FindDevices(int,uchar FamilySN[][8],SMALLINT,int);
extern void      PrintSerialNum(uchar*);
extern SMALLINT  owOverdriveAccess(int);
extern void      owSerialNum(int,uchar *,SMALLINT);

// local functions
static void TestJiB(int portnum, uchar *dev);

// global serial numbers
uchar FamilySN[MAXDEVICES][8];

//--------------------------------------------------------------------------
// Main for the Jib test
//
int main(int argc, char *argv[])
{
   int portnum = 0,i;
   SMALLINT NumDevices;

   printf("\nJiBTest, Java Powered iButton HW/SW Tester, Version 3.00B\n");

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

      // loop through the devices and do a test on each
      for (i = 0; i < NumDevices; i++)
         TestJiB(portnum, FamilySN[i]);
   }

   // release the 1-Wire Net
   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}

//--------------------------------------------------------------------------
// Run the Java-powered Button through most of the read functions.
//
void TestJiB(int portnum, uchar *dev)
{
   char            l_Buff[256];
   int             l_FailureCount  = 0,
                   l_TestCount     = 0,
                   l_AppletNum,
                   l_CurrentOffset = 0,
                   l_CurrentLength = 0;
   ulong           l_FreeRam       = 0;
   LPRESPONSEAPDU  l_lpResponseAPDU;

   // select the device and put in overdrive
   printf("                  Device: ");
   PrintSerialNum(dev);
   printf("\n");

   owSerialNum(portnum,dev,0);
   if (!owOverdriveAccess(portnum))
   {
      printf("ERROR, could not get device in overdrive!\n");
      l_FailureCount++;
   }

   l_TestCount++;

   // Get Firmware Version String
   l_lpResponseAPDU = GetFirmwareVersionString(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Firmware Version",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      // trim out the length byte
      memcpy(l_Buff,l_lpResponseAPDU->Data,l_lpResponseAPDU->Len);
      memcpy(l_lpResponseAPDU->Data,l_Buff+1,--(l_lpResponseAPDU->Len));
      // zero terminate the strings
      l_lpResponseAPDU->Data[l_lpResponseAPDU->Len] = 0;
      printf(" Firmware Version String: %s\n",l_lpResponseAPDU->Data);
   }

   // Get Real-Time Clock
   l_lpResponseAPDU = GetRealTimeClock(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Real Time Clock",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("Real Time Clock (1 of 2): 0x%08lX\n",
           ((ulong)l_lpResponseAPDU->Data[3]<<24)+
           ((ulong)l_lpResponseAPDU->Data[2]<<16)+
           ((ulong)l_lpResponseAPDU->Data[1]<<8)+
           l_lpResponseAPDU->Data[0]);
   }

   // Get Free RAM
   l_lpResponseAPDU = GetFreeRam(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Free RAM",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      l_FreeRam = ((ushort)l_lpResponseAPDU->Data[1]<<8)+l_lpResponseAPDU->Data[0];

      if(l_lpResponseAPDU->Len > 2)
         l_FreeRam += ((ulong)l_lpResponseAPDU->Data[2]<<16);

      printf("                Free RAM: %d bytes\n", l_FreeRam);
   }

   // Get Power On Reset Count
   l_lpResponseAPDU = GetPORCount(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","POR Count",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("         Power On Resets: %hd\n",
           ((ushort)l_lpResponseAPDU->Data[1]<<8)+
           l_lpResponseAPDU->Data[0]);
   }

   // Get Ephemeral GC Mode
   l_lpResponseAPDU = GetEphemeralGCMode(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Ephemeral GC Mode",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("            Ephemeral GC: %s\n",l_lpResponseAPDU->Data[0] ? "On":"Off");
   }

   // Get Applet GC Mode
   l_lpResponseAPDU = GetAppletGCMode(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Applet GC Mode",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("               Applet GC: %s\n",l_lpResponseAPDU->Data[0] ? "On":"Off");
   }

   // Get Restore Mode
   l_lpResponseAPDU = GetRestoreMode(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Restore Mode",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("            Restore Mode: %s\n",l_lpResponseAPDU->Data[0] ? "Enabled":"Disabled");
   }

   // Get Exception Mode
   l_lpResponseAPDU = GetExceptionMode(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Exception Mode",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("          Exception Mode: %s\n",l_lpResponseAPDU->Data[0] ? "Enabled":"Disabled");
   }

   // Get Commit Buffer Size
   l_lpResponseAPDU = GetCommitBufferSize(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Commit Buffer Size",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("      Commit Buffer Size: %hd bytes",
           ((ushort)l_lpResponseAPDU->Data[1]<<8)+
           l_lpResponseAPDU->Data[0]);
   }

   // Get Commit Buffer Fields
   l_lpResponseAPDU = GetCommitBufferFields(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("\n%s Test Failed with SW = 0x%04hX\n","Commit Buffer Fields",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf(" -> %hd fields\n",
           ((ushort)l_lpResponseAPDU->Data[1]<<8)+
           l_lpResponseAPDU->Data[0]);
   }

   // Get Real-Time Clock
   l_lpResponseAPDU = GetRealTimeClock(portnum);

   l_TestCount++;

   if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
   {
      printf("%s Test Failed with SW = 0x%04hX\n","Real Time Clock",l_lpResponseAPDU->SW);
      l_FailureCount++;
   }
   else
   {
      printf("Real Time Clock (2 of 2): 0x%08lX\n",
           ((ulong)l_lpResponseAPDU->Data[3]<<24)+
           ((ulong)l_lpResponseAPDU->Data[2]<<16)+
           ((ulong)l_lpResponseAPDU->Data[1]<<8)+
           l_lpResponseAPDU->Data[0]);
   }

   printf("       Installed Applets:\n");

   if(!(dev[0] & (uchar)0x80)) // quick test for crusty firmware
   {
      // Enumerate all applets
      for(l_AppletNum = 0; l_AppletNum < 16; l_AppletNum++)
      {
         l_lpResponseAPDU = GetAIDByNumber(portnum, (uchar)l_AppletNum);

         l_TestCount++;

         if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
         {
            // Not an error if no applet is installed at this position
            if(l_lpResponseAPDU->SW != 0x8453)
            {
               printf("%s Test Failed with SW = 0x%04hX\n","Get AID",l_lpResponseAPDU->SW);
               l_FailureCount++;
            }
         }
         else
         {
            if(l_lpResponseAPDU->Len)
            {
               // trim out the length byte
               memcpy(l_Buff,l_lpResponseAPDU->Data+1,(int)l_lpResponseAPDU->Data[0]);

               // zero terminate the string
               l_Buff[*(uchar *)l_lpResponseAPDU->Data] = 0;

               printf("%24c%2d) %s\n",' ',l_AppletNum,l_Buff);
            }
         }
      }
   }
   else // can get all AIDS in single APDU
   {
      l_TestCount++;
      l_lpResponseAPDU = GetAllAIDs(portnum);

      if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
      {
         if(l_lpResponseAPDU->SW != 0x8453)
         {
            printf("%s Test Failed with SW = 0x%04hX\n","Get All AIDs",l_lpResponseAPDU->SW);
            l_FailureCount++;
         }
         else
            printf("%24c No Applets found\n",' ');
      }
      else
      {
         while((unsigned) l_CurrentOffset < l_lpResponseAPDU->Len)
         {
            l_AppletNum = l_lpResponseAPDU->Data[l_CurrentOffset++];
            l_CurrentLength = l_lpResponseAPDU->Data[l_CurrentOffset++];
            memcpy(l_Buff,
                   l_lpResponseAPDU->Data+l_CurrentOffset,
                   l_CurrentLength);

            // zero terminate the string
            l_Buff[l_CurrentLength] = 0;

            l_CurrentOffset += l_CurrentLength;

            printf("%24c%2d) %s\n",' ',l_AppletNum,l_Buff);
         }
      }
   }


   if(l_FailureCount)
    printf("\n%d of %d tests failed! Check Hardware and Driver (if applicable) Installation\n\n",l_FailureCount,l_TestCount);
   else
    printf("\nHardware Installation Ok!\n\n");
}

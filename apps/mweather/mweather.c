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
//  mweather.c - Test application to read the 1-Wire Ney Weather Station.
//               The weather station consists of the following devices
//               on the trunk of the 1-Wire Net:
//                 DS1820 - temperature measurement
//                 DS2423 - counter for reading wind speed (page 15)
//                 DS2450 - isolate wind direction.
//                 DS2406 - switch to isolate 8 DS2401's for wind
//                          direction on channel B.
//
//               Commented out is an example on how to setup two weather stations.
//


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ownet.h"
#include "swt12.h"
#include "weather.h"
#include "cnt1d.h"
#include "temp10.h"

// defines
#define FALSE              0
#define TRUE               1
// misc
#define MAX2401            8

// local funcitons

// global serial numbers of Weather Station devices
uchar TempSN[8],CountSN[8],SwitchSN[8],
      DS2401[MAX2401][8];


//----------------------------------------------------------------------
//  Main Test for Weather Station
//
int main(int argc, char **argv)
{
   WeatherStruct weather1;
//   WeatherStruct weather1 = { {0x20, 0x7A, 0x01, 0x01, 0x00, 0x00, 0x00, 0xFA},
//                              {0x10, 0x2B, 0x75, 0x36, 0x00, 0x00, 0x00, 0x4B},
//                              {0x1D, 0xEE, 0xFE, 0x00, 0x00, 0x00, 0x00, 0xFD} };
//   WeatherStruct weather2 = { {0x12, 0x69, 0xBE, 0x06, 0x00, 0x00, 0x00, 0x30},
//                              {0x10, 0x7C, 0x8E, 0x15, 0x00, 0x00, 0x00, 0xCE},
//                              {0x1D, 0xCF, 0x33, 0x00, 0x00, 0x00, 0x00, 0xB2},
//                              {  {0x01, 0xD0, 0x57, 0x00, 0x02, 0x00, 0x00, 0x72},
//                                 {0x01, 0x01, 0x79, 0x73, 0x02, 0x00, 0x00, 0xFB},
//                                 {0x01, 0x34, 0xAB, 0x73, 0x02, 0x00, 0x00, 0x20},
//                                 {0x01, 0x3D, 0x82, 0x73, 0x02, 0x00, 0x00, 0xBD},
//                                 {0x01, 0x3D, 0x82, 0x73, 0x02, 0x00, 0x00, 0x1E},
//                                 {0x01, 0x6F, 0xB1, 0x73, 0x02, 0x00, 0x00, 0x37},
//                                 {0x01, 0xCC, 0x57, 0x00, 0x02, 0x00, 0x00, 0x54},
//                                 {0x01, 0xD4, 0x57, 0x00, 0x02, 0x00, 0x00, 0xAE}  }  };

   time_t tlong;
   struct tm *tstruct;
   float current_temp;
   double revolution_sec;
   int direction;
   int portnum;

   //----------------------------------------
   // Introduction header
   printf("\n/---------------------------------------------\n");
   printf("  NWeather - V2.00\n"
          "  The following is a test to excersize the two\n"
          "  1-Wire Net Weather Station containing\n"
          "    DS1820 - temperature measurement (at least 1)\n"
          "    DS2423 - counter for reading wind speed (page 15)\n"
          "    DS2450 - Isolate direction for wind\n"
          "       or the\n"
          "    DS2406 - switch to isolate 8 DS2401's for wind\n"
          "             direction on channel B.\n");

   printf("  Press any CTRL-C to stop this program.\n\n");
   printf("Output [Time, Temp1(F),Temp2(F).., Direction Serial Number(s), "
          "Wind speed(MPH)]\n\n");

   // check for required port name
   if (argc != 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }

   // attempt to acquire the 1-Wire Net
   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // success
   printf("Port opened: %s\n",argv[1]);

   // Setting up the DS2450 weather station
//   if(SetupWet(portnum, &weather1, 5))
//   {
//      if(ReadWet(portnum, &weather1, &current_temp, &direction, &revolution_sec))
//      {
//         time(&tlong);
//         tstruct = localtime(&tlong);
//         printf("%02d:%02d:%02d,",tstruct->tm_hour,tstruct->tm_min,tstruct->tm_sec);
//         printf("%5.1f, ",current_temp);
//         printf("%02d,",direction);
//         printf("%5.1f\n",revolution_sec);
//	  }
//   }

   // Setting up the DS2406 weather station
//   if(SetupWet(portnum, &weather2, 5))
//   {
//      if(ReadWet(portnum, &weather2, &current_temp, &direction, &revolution_sec))
//      {
//         time(&tlong);
//         tstruct = localtime(&tlong);
//         printf("%02d:%02d:%02d,",tstruct->tm_hour,tstruct->tm_min,tstruct->tm_sec);
//         printf("%5.1f, ",current_temp);
//         printf("%02d,",direction);
//         printf("%5.1f\n",revolution_sec);
//      }
//   }

   // If setting up the DS2406 in this program uncomment the weather2 code
   // and change the weather1 to weather2 in the following code
   if(FindSetupWeather(portnum, &weather1))
   {
      printf("The Found Weather Station\n");
      do
      {
         if(ReadWet(portnum, &weather1, &current_temp, &direction, &revolution_sec))
         {
            time(&tlong);
            tstruct = localtime(&tlong);
            printf("%02d:%02d:%02d,",tstruct->tm_hour,tstruct->tm_min,tstruct->tm_sec);
            printf("%5.1f, ",current_temp);
            printf("%02d,",direction);
            printf("%5.1f\n",revolution_sec);
         }
       }
       while (TRUE); //!key_abort());
   }

   // release the 1-Wire Net
   owRelease(portnum);
   printf("Closing port %s.\n", argv[1]);
   exit(0);

   return 0;
}


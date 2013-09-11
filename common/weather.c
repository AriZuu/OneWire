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
//  weather.c - To find what kind of weather station or if it is there.
//              There are also modules to find the direction of the weather
//              station and read/write output files
//
//  Version: 2.00
//
//
// --------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "ownet.h"
#include "swt12.h"
#include "findtype.h"
#include "weather.h"
#include "atod20.h"
#include "temp10.h"
#include "cnt1d.h"

// defines
#define MAXDEVICES           8

// family codes of devices
#define TEMP_FAMILY        0x10
#define SWITCH_FAMILY      0x12
#define ATOD_FAMILY        0X20
#define COUNT_FAMILY       0x1D
#define DIR_FAMILY         0x01
// misc
#define MAXTEMPS           1
#define MAXSWITCH          1
#define MAXCOUNT           1
#define MAXATOD            1
#define MAXDIR             8


static int conv[8] = {0,14,12,10,8,6,4,2};
static double conv_table[16][4] = { {0.06, 4.64, 4.64, 4.64},
                                    {0.06, 0.06, 4.60, 4.60},
                                    {4.64, 0.06, 4.64, 4.64},
                                    {4.62, 0.06, 0.06, 4.60},
                                    {4.64, 4.64, 0.06, 4.64},
                                    {4.60, 4.60, 0.06, 0.06},
                                    {4.64, 4.64, 4.64, 0.06},
                                    {2.36, 4.62, 4.60, 0.06},
                                    {2.38, 4.66, 4.66, 4.66},
                                    {3.20, 3.20, 4.66, 4.64},
                                    {4.66, 2.38, 4.66, 4.66},
                                    {4.66, 3.18, 3.20, 4.64},
                                    {4.66, 4.66, 2.38, 4.66},
                                    {4.66, 4.66, 3.18, 3.18},
                                    {4.66, 4.66, 4.66, 2.38},
                                    {0.06, 4.62, 4.62, 2.34} };


// -------------------------------------------------------------------------
// SUBROUTINE - FindWeather
//
// This routine finds which weather station is attached to the 1-wire
// network and returns the serial numbers for the weather station.  If
// it is the new weather station then it is not going to have an DS2401s.
//
// 'ds1820' - This will be the serial number for the DS1820 in the station
// 'ds2423' - This will be the serial number for the DS2423 in the station
// 'dsdir'  - This will either be the DS2450 or the DS2406 serial num.
// 'ds2401' - This will be the list of DS2401s if the old weather station.
//
// Returns: TRUE if a weather station is found and the listing of the
//          devices file is found or made.  FALSE if a weather station
//          is not found or the device file is not found or can not be made.
//
int FindSetupWeather(int portnum, WeatherStruct *wet)
{
   int ret = FALSE;
   FILE *fptr;
   char filename[] = "ini.txt";
   char TempSN[190];
   uchar SNs[90];
   int i,j;
   int num;
   char msg[45];
   uchar temp[MAXDIR][8];

   temp[0][0] = 0x00;
   wet->weather_a = FALSE;
   wet->weather_b = FALSE;
   wet->found_2401 = 0;
   wet->found_2423 = FALSE;
   wet->found_1820 = FALSE;
   wet->found_dir  = FALSE;
   wet->north = 0;

   if((fptr = fopen(filename, "r")) == NULL)
   {
      printf("error opening file but try to create file\n");

      // Create ini.txt if weather station has DS2450

      // find the DS1820 temperature
      num = FindDevices(portnum, &temp[0], TEMP_FAMILY, MAXTEMPS);

      // check if not at least 1 temperature device
      if(num == 0)
         wet->found_1820 = FALSE;
      else
      {
         for(i=0;i<8;i++)
            wet->ds1820[i] = temp[0][i]; 
         wet->found_1820 = TRUE;
      }

      // find the DS2450 switch
      num = FindDevices(portnum, &temp[0], ATOD_FAMILY, MAXATOD);

      // check if not at least 1 switch device
      if(num == 0)
         wet->found_dir = FALSE;
      else
      {
         for(i=0;i<8;i++)
            wet->dsdir[i] = temp[0][i];
         wet->found_dir = TRUE;
      }

      // find the DS2423 switch
      num = FindDevices(portnum, &temp[0], COUNT_FAMILY, MAXCOUNT);

      // check if not at least 1 count device
      if(num == 0)
         wet->found_2423 = FALSE;
      else
      {
         for(i=0;i<8;i++)
            wet->ds2423[i] = temp[0][i];
         wet->found_2423 = TRUE;
      }

      // Create the output file ini.txt
      if(wet->found_1820 && wet->found_dir && wet->found_2423)
      {
         if(SetupAtoDControl(portnum, &wet->dsdir[0], &wet->ctrl[0], &msg[0]))
            if(WriteAtoD(portnum, FALSE, &wet->dsdir[0], &wet->ctrl[0], 0x08, 0x11))
            {

               if((fptr = fopen(filename, "w")) == NULL)
                  printf("error tring to create file\n");
               else
               {
                  wet->weather_b = TRUE;
                  ret = TRUE;

                  // Printing the Serial number for DS2450
                  printf("Found DS2450\n");
                  for(i=7; i>=0; i--)
                     printf("%02X", wet->dsdir[i]);
                  printf("\n");
                  for(i=7; i>=0; i--)
                     fprintf(fptr, "%02X", wet->dsdir[i]);
                  fprintf(fptr, "\n");

                  // Printing the Serial number for DS1820
                  printf("Found DS1820\n");
                  for(i=7; i>=0; i--)
                     printf("%02X", wet->ds1820[i]);
                  printf("\n");
                  for(i=7; i>=0; i--)
                     fprintf(fptr, "%02X", wet->ds1820[i]);
                  fprintf(fptr, "\n");

                  // Printing the Serial number for DS2423
                  printf("Found DS2423\n");
                  for(i=7; i>=0; i--)
                     printf("%02X", wet->ds2423[i]);
                  printf("\n");
                  for(i=7; i>=0; i--)
                     fprintf(fptr, "%02X", wet->ds2423[i]);
                  fprintf(fptr, "\n");

                  // Printing the Default Wind Direction
                  printf("The Default Wind Direction is 0\n");
                  fprintf(fptr, "0\n");
               }
            }
      }
   }
   else
   {
      num = fread(TempSN, sizeof(char), 189, fptr);

      num = ParseData(TempSN, num, &SNs[0], 90);

      if(!wet->weather_a && !wet->weather_b)
      {
         if(SNs[7] == 0x01)
            wet->weather_a = TRUE;
         else
            wet->weather_b = TRUE;
      }

      j = 0;
      while((SNs[7+8*j] == 0x01) || (SNs[7+8*j] == 0x20) || (SNs[7+8*j] == 0x12) ||
            (SNs[7+8*j] == 0x1D) || (SNs[7+8*j] == 0x10))
      {
         if(SNs[7+8*j] == 0x01)
         {
            for(i=0; i<8; i++)
               wet->ds2401[wet->found_2401][7-i] = SNs[8*j + i];

            printf("Found DS2401\n");
            for(i=7; i>=0; i--)
               printf("%02X", wet->ds2401[wet->found_2401][i]);
            printf("\n");

            wet->found_2401++;
         }

         if(SNs[7+8*j] == 0x20)
         {
            if(!wet->found_dir)
            {
               for(i=0; i<8; i++)
                  wet->dsdir[7-i] = SNs[8*j + i];

               owSerialNum(portnum, wet->dsdir, FALSE);
               if(!owVerify(portnum, FALSE))
                  ret = FALSE;

               wet->found_dir = TRUE;
               printf("Found DS2450\n");
               for(i=7; i>=0; i--)
                  printf("%02X", wet->dsdir[i]);
               printf("\n");

               if(!SetupAtoDControl(portnum, wet->dsdir, &wet->ctrl[0], &msg[0]))
                  return FALSE;
               if(!WriteAtoD(portnum, FALSE, wet->dsdir, &wet->ctrl[0], 0x08, 0x11))
                  return FALSE;

            }
         }

         if(SNs[7+8*j] == 0x12)
         {
            if(!wet->found_dir)
            {
               for(i=0; i<8; i++)
                  wet->dsdir[7-i] = SNs[8*j + i];

               owSerialNum(portnum, wet->dsdir, FALSE);
               if(!owVerify(portnum, FALSE))
                  ret = FALSE;

               wet->found_dir = TRUE;
               printf("Found DS2406\n");
               for(i=7; i>=0; i--)
                  printf("%02X", wet->dsdir[i]);
               printf("\n");
            }
         }

         if(SNs[7+8*j] == 0x1D)
         {
            for(i=0; i<8; i++)
               wet->ds2423[7-i] = SNs[8*j + i];

            owSerialNum(portnum, wet->ds2423, FALSE);
            if(!owVerify(portnum, FALSE))
               ret = FALSE;

            wet->found_2423 = TRUE;
            printf("Found DS2423\n");
            for(i=7; i>=0; i--)
               printf("%02X", wet->ds2423[i]);
            printf("\n");
         }

         if(SNs[7+8*j] == 0x10)
         {
            for(i=0; i<8; i++)
               wet->ds1820[7-i] = SNs[8*j + i];

            owSerialNum(portnum, wet->ds1820, FALSE);
            if(!owVerify(portnum, FALSE))
               ret = FALSE;

            wet->found_1820 = TRUE;
            printf("Found DS1820\n");
            for(i=7; i>=0; i--)
               printf("%02X", wet->ds1820[i]);
            printf("\n");
         }

         j++;
      }

      if(wet->weather_a)
      {
         if((TempSN[0xBB] <= 0x40) && (TempSN[0xBB] >= 0x30))
            if((TempSN[0xBC] <= 0x40) && (TempSN[0xBC] >= 0x30))
               wet->north = (TempSN[0xBB]-0x30)*10 + (TempSN[0xBC]-0x30);
            else
               wet->north = TempSN[0xBB] - 0x30;
         else
            wet->north = 0;

         if(wet->found_dir && wet->found_1820 && wet->found_2423 &&
           (wet->found_2401 == 8))
            ret = TRUE;
      }
      else if(wet->weather_b)
      {
         if((TempSN[0x33] <= 0x40) && (TempSN[0x33] >= 0x30))
            if((TempSN[0x34] <= 0x40) && (TempSN[0x34] >= 0x30))
               wet->north = (TempSN[0x33]-0x30)*10 + (TempSN[0x34]-0x30);
            else
               wet->north = TempSN[0x33] - 0x30;
         else
            wet->north = 0;

         if(wet->found_dir && wet->found_1820 && wet->found_2423)
            ret = TRUE;
      }

   }

   if(wet->weather_a || wet->weather_b)
   {
      if(fptr != 0)
      {
         fclose(fptr);
      }
   }

   return ret;
}


// -------------------------------------------------------------------------
// SUBROUTINE - SetupWet
//
// This routine sets up a weather station given the serial numbers for the
// weather station.
//
// Returns: TRUE, if the weather station was setup and FALSE if it was not.
//
int SetupWet(int portnum, WeatherStruct *wet, int nor)
{
   int i,j;
   int ret = FALSE;
   char msg[45];

   wet->north = nor;

   if((wet->ds2401 == NULL) || wet->weather_b)
   {
      wet->weather_b = TRUE;
      wet->weather_a = FALSE;

      // Setting up the DS2450
      owSerialNum(portnum, wet->dsdir, FALSE);
      if(!owVerify(portnum, FALSE))
      {
         printf("The DS2450 was not found.\n");
         return FALSE;
      }
      else
      {
         // Printing the Serial number for DS2450
         printf("Set DS2450\n");
         for(i=7; i>=0; i--)
            printf("%02X", wet->dsdir[i]);
         printf("\n");
      }

      // Setting up the DS1820
      owSerialNum(portnum, wet->ds1820, FALSE);
      if(!owVerify(portnum, FALSE))
      {
         printf("The DS1820 was not found.\n");
         return FALSE;
      }
      else
      {
         // Printing the Serial number for DS1820
         printf("Set DS1820\n");
         for(i=7; i>=0; i--)
            printf("%02X", wet->ds1820[i]);
         printf("\n");
      }

      // Setting up the DS2423
      owSerialNum(portnum, wet->ds2423, FALSE);
      if(!owVerify(portnum, FALSE))
      {
         printf("The DS2423 was not found.\n");
         return FALSE;
      }
      else
      {
         // Printing the Serial number for DS2423
         printf("Set DS2423\n");
         for(i=7; i>=0; i--)
            printf("%02X", wet->ds2423[i]);
         printf("\n");
      }

      if(SetupAtoDControl(portnum, &wet->dsdir[0], &wet->ctrl[0], &msg[0]))
         if(WriteAtoD(portnum, FALSE, &wet->dsdir[0], &wet->ctrl[0], 0x08, 0x11))
            ret = TRUE;

   }
   else
   {
      wet->weather_a = TRUE;
      wet->weather_b = FALSE;

      printf("Set DS2401\n");
      for(i=0; i<8; i++)
      {
         // Printing the Serial number for DS2401
         for(j=7; j>=0; j--)
            printf("%02X", wet->ds2401[i][j]);
         printf("\n");
      }

      // Setting up the DS2423
      owSerialNum(portnum, wet->ds2423, FALSE);
      if(!owVerify(portnum, FALSE))
      {
         printf("The DS2423 was not found.\n");
         return FALSE;
      }
      else
      {
         // Printing the Serial number for DS2423
         printf("Set DS2423\n");
         for(i=7; i>=0; i--)
            printf("%02X", wet->ds2423[i]);
         printf("\n");
      }

      // Setting up the DS1820
      owSerialNum(portnum, wet->ds1820, FALSE);
      if(!owVerify(portnum, FALSE))
      {
         printf("The DS1820 was not found.\n");
         return FALSE;
      }
      else
      {
         // Printing the Serial number for DS1820
         printf("Set DS1820\n");
         for(i=7; i>=0; i--)
            printf("%02X", wet->ds1820[i]);
         printf("\n");
      }

      // Setting up the DS2406
      owSerialNum(portnum, wet->dsdir, FALSE);
      if(!owVerify(portnum, FALSE))
      {
         printf("The DS2406 was not found.\n");
         return FALSE;
      }
      else
      {
         // Printing the Serial number for DS2406
         printf("Set DS2450\n");
         for(i=7; i>=0; i--)
            printf("%02X", wet->dsdir[i]);
         printf("\n");
      }

      ret = TRUE;
   }

   return ret;
}


// -------------------------------------------------------------------------
// SUBROUTINE - ReadWet
//
// 'temp'  - The temperature from the DS1820 in F
// 'dir'   - The wind direction from 0-15
// 'revol' - The wind speed in revolutions per second
//
// Returns:  TRUE, if the reads were successfull and FALSE if they were not.
//
int ReadWet(int portnum, WeatherStruct *wet, float *temp, int *dir, double *revol)
{
   int ret = TRUE;
   unsigned long start_count=0, end_count=0, start_time, end_time;
   float current_temp;

   // read the current counter
   // use this reference to calculate wind speed later
   if (!ReadCounter(portnum, &wet->ds2423[0], 15, &start_count))
   {
      printf("\nError reading counter, verify device present:%d\n",
              (int)owVerify(portnum, FALSE));
      ret = FALSE;
   }

   // get a time stamp (mS)
   start_time = msGettick();

   // read the temperature and print in F
   if (ReadTemperature(portnum, &wet->ds1820[0],&current_temp))
      *temp = current_temp * 9 / 5 + 32;
   else
   {
      printf("\nError reading temperature, verify device present:%d\n",
              (int)owVerify(portnum, FALSE));
      ret = FALSE;
   }

   // read the wind direction
   *dir = TrueDir(portnum, wet);

   // read the counter again
   if (!ReadCounter(portnum, &wet->ds2423[0], 15, &end_count))
   {
      printf("Error reading counter, verify device present:%d\n",
              (int)owVerify(portnum, FALSE));
      ret = FALSE;
   }

   // get a time stamp (mS)
   end_time = msGettick();

   // calculate the wind speed based on the revolutions per second
   *revol = (((end_count - start_count) * 1000.0) /
              (end_time - start_time)) / 2.0;

   return ret;
}
// -------------------------------------------------------------------------
// SUBROUTINE - GetDir
//
// This routine looks at the DS2401 serial numbers or the DS2450 volts to
// determine which direction the wind is blowing.
//
// Returns: The direction in a 0-15 order and 16 for no DS2401s found
//
int GetDir(int portnum, WeatherStruct *wet)
{
   SwitchProps st;
   uchar temp_dir[MAXDIR][8];
   int i,j;
   int numdir = 0;
   int firstmatch = 0, secondmatch = 0;
   int firstindex = 0;
   int found = TRUE;
   int ret = 16;
   float prslt[4];

   temp_dir[0][0] = 0x00;

   if(wet->weather_a)
   {
      st.Chan_A = FALSE;
      st.Chan_B = TRUE;
      if(SetSwitch12(portnum, &wet->dsdir[0], st))
      {
         numdir = FindDevices(portnum, &temp_dir[0], DIR_FAMILY, MAXDIR);
      }
      else
         printf("Error on setting up the switch\n");

      if(numdir == 0)
         ret = 16;

      if(numdir == 2)
      {
         for(i=0; i<8; i++)
         {
            for(j=0; j<8; j++)
               if(temp_dir[0][j] != wet->ds2401[i][j])
               {
                  found = FALSE;
                  break;
               }
               else
                  found = TRUE;

            if(found)
            {
               firstindex = i;
               firstmatch = conv[i];
               found = FALSE;
               break;
            }
         }

         for(i=0; i<8; i++)
         {
            if(i != firstindex)
            {
               for(j=0; j<8; j++)
               {
                  if(temp_dir[1][j] != wet->ds2401[i][j])
                  {
                     found = FALSE;
                     break;
                  }
                  else
                     found = TRUE;
               }
            }
            if(found)
            {
               secondmatch = conv[i];
               break;
            }
         }

         if(((firstmatch == 0) || (secondmatch == 0)) &&
            ((firstmatch == 14) || (secondmatch == 14)))
            ret = 15;
         else
            ret = (firstmatch+secondmatch)/2;
      }

      if(numdir == 1)
      {
         for(i=0; i<8; i++)
         {
            found = TRUE;

            for(j=0; j<8; j++)
               if(temp_dir[0][j] != wet->ds2401[i][j])
                  found = FALSE;

            if(found)
            {
               ret = conv[i];
               break;
            }
         }

      }

      st.Chan_A = FALSE;
      st.Chan_B = FALSE;
      if(!SetSwitch12(portnum, &wet->dsdir[0], st))
      {
         msDelay(10);
         if(!SetSwitch12(portnum, &wet->dsdir[0], st))
         {
            printf("Failed to close channel B\n");
         }
      }
   }
   else if(wet->weather_b)
   {
      // read wind direction for the DS2450 weather station
      if(DoAtoDConversion(portnum, FALSE, &wet->dsdir[0]))
      {
         if(ReadAtoDResults(portnum, FALSE, &wet->dsdir[0], &prslt[0], &wet->ctrl[0]))
         {
            for(i=0; i<16; i++)
            {
               if( ((prslt[0] <= (float)conv_table[i][0]+0.25) &&
                    (prslt[0] >= (float)conv_table[i][0]-0.25)) &&
                   ((prslt[1] <= (float)conv_table[i][1]+0.25) &&
                    (prslt[1] >= (float)conv_table[i][1]-0.25)) &&
                   ((prslt[2] <= (float)conv_table[i][2]+0.25) &&
                    (prslt[2] >= (float)conv_table[i][2]-0.25)) &&
                   ((prslt[3] <= (float)conv_table[i][3]+0.25) &&
                    (prslt[3] >= (float)conv_table[i][3]-0.25)) )
               {
                  ret = i;
                  break;
               }
            }
         }
         else
         {
            printf("\n");
            printf("\nError reading channel, verify device present: %d\n",
            (int)owVerify(portnum, FALSE));
         }
      }
   }

   return ret;
}


// -------------------------------------------------------------------------
// SUBROUTINE - TrueDir
//
// This routine returns the direction given the north direction in 0-15.
//
// Returns: The direction in a 0-15 order with 0 north and clockwise from that.
//          16 is returned for no DS2401s found.
//
int TrueDir(int portnum, WeatherStruct *wet)
{
   int dir;
   int retdir;

   dir = GetDir(portnum, wet);

   if(dir != 16)
   {
      if( (dir-wet->north) < 0 )
         retdir = (dir - wet->north + 16);
      else
         retdir = dir - wet->north;
   }
   else
      retdir = 16;

   return retdir;
}


//----------------------------------------------------------------------
// Parse the raw file data in to an array of uchar's.  The data must
// be hex characters possible seperated by white space
//     12 34 456789ABCD EF
//     FE DC BA 98 7654 32 10
// would be converted to an array of 16 uchars.
// return the array length.  If an invalid hex pair is found then the
// return will be 0.
//
int ParseData(char *inbuf, int insize, uchar *outbuf, int maxsize)
{
   int ps, outlen=0, gotmnib=0;
   uchar mnib;

   // loop until end of data
   for (ps = 0; ps < insize; ps++)
   {
      // check for white space
      if (isspace(inbuf[ps]))
         continue;
      // not white space, make sure hex
      else if (isxdigit(inbuf[ps]))
      {
         // check if have first nibble yet
         if (gotmnib)
         {
            // this is the second nibble
            outbuf[outlen++] = (mnib << 4) | ToHex(inbuf[ps]);
            gotmnib = 0;
         }
         else
         {
            // this is the first nibble
            mnib = ToHex(inbuf[ps]);
            gotmnib = 1;
         }
      }
      else
         return 0;

      // if getting to the max return what we have
      if ((outlen + 1) >= maxsize)
         return outlen;
   }

   return outlen;
}


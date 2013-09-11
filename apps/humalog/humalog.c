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
//--------------------------------------------------------------------------
//
//  owMem.c - Goes through the testing the memory bank for different parts.
//  Version 2.00
//

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ownet.h"
#include "findtype.h"
#include "humutil.h"
#include "pw77.h"

void selectDevice(int portnum, uchar *SNum);
int menuSelect();
int getNumber (int min, int max);
SMALLINT getBoolean ();
float getFloat (float min, float max);

#define HYGRO_START_MISSION   0
#define HYGRO_STOP_MISSION    1
#define HYGRO_DOWNLOAD_DATA   2
#define HYGRO_GET_TEMPERATURE 3
#define HYGRO_GET_DATA        4
#define HYGRO_BM_READ_PASS    5
#define HYGRO_BM_RW_PASS      6
#define HYGRO_READ_PASS       7
#define HYGRO_RW_PASS         8
#define HYGRO_ENABLE_PASS     9
#define HYGRO_DISABLE_PASS    10
#define QUIT                  11


int main(int argc, char **argv)
{
   int portnum = 0;
   int length,i;
   uchar SNum[8];
   uchar state[96];
   uchar passw[8];
   char msg[256];
   SMALLINT done = FALSE;
   startMissionData settings;
   configLog config;
   double val,valsq,error;

   // check for required port name
   if (argc != 2)
   {
      sprintf(msg,"1-Wire Net name required on command line!\n"
                  " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
                  "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      printf("%s\n",msg);
      return 0;
   }

   printf("\n1-Wire Memory utility console application Version 0.01\n");

   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      return 0;
   }
   else
   {
      selectDevice(portnum,&SNum[0]);

      do
      {
         switch(menuSelect())
         {
            case HYGRO_START_MISSION :

               printf("Enter the sample rate in seconds.\n");
               settings.sampleRate = getNumber(1,982800);

               printf("Enter start delay in minutes.\n");
               settings.startDelay = getNumber(0,16777215);

               printf("Enable rollover? (T/F)\n");
               settings.enableRollover = getBoolean();

               printf("Should the iButton clock be sync'ed with the OS clock? (T/F)\n");
               settings.timeSync = getBoolean();

               printf("Do you want the temperture channel enabled? (T/F)\n");
               settings.tempEnabled = getBoolean();

               if(settings.tempEnabled)
               {
                  printf("Do you want to set a high temperature alarm? (T/F)\n");
                  settings.highTempAlarm = getBoolean();

                  if(settings.highTempAlarm)
                  {
                     printf("Enter the temperature for the high alarm.\n");
                     settings.highTemp = getFloat(-40.0,125.0);
                  }

                  printf("Do you want to set a low temperature alarm? (T/F)\n");
                  settings.lowTempAlarm = getBoolean();

                  if(settings.lowTempAlarm)
                  {
                     printf("Enter the temperature for the low alarm.\n");
                     if(settings.highTempAlarm)
                        settings.lowTemp = getFloat(-40.0,settings.highTemp);
                     else
                        settings.lowTemp = getFloat(-40.0,125.0);
                  }

                  printf("Do you want to start mission upon a temperature alarm? (T/F)\n");
                  settings.tempAlarm = getBoolean();

                  printf("Do you want the temperatures recorded in high resolution? (T/F)\n");
                  settings.highTempResolution = getBoolean();
               }

               printf("Do you want the data channel enabled? (T/F)\n");
               settings.dataEnabled = getBoolean();

               if(settings.dataEnabled)
               {
                  printf("Do you want to set a high data alarm? (T/F)\n");
                  settings.highDataAlarm = getBoolean();

                  if(settings.highDataAlarm)
                  {
                     printf("Enter the integer high alarm percentage.\n");
                     settings.highData = getFloat(0.0,100.0);
                  }

                  printf("Do you want to set a low data alarm? (T/F)\n");
                  settings.lowDataAlarm = getBoolean();

                  if(settings.lowDataAlarm)
                  {
                     printf("Enter the integer low alarm percentage.\n");
                     settings.lowData = getFloat(0.0,settings.highData);
                  }

                  printf("Do you want the data recorded at high resolution? (T/F)\n");
                  settings.highDataResolution = getBoolean();
               }

               if(!startMission(portnum,&SNum[0],settings,&config))
               {
                  OWERROR_DUMP(stdout);
                  printf("Error starting mission.\n");
               }

               break;

            case HYGRO_STOP_MISSION  :

               if(!stopMission(portnum,&SNum[0]))
               {
                  OWERROR_DUMP(stdout);
                  printf("Error stopping mission.\n");
               }

               break;

            case HYGRO_DOWNLOAD_DATA :
               if(readDevice(portnum,&SNum[0],&state[0],&config))
               {
                  config.useHumidityCalibration = FALSE;

                  if(!loadMissionResults(portnum,&SNum[0],config))
                  {
                     OWERROR_DUMP(stdout);
                     printf("error with loading data.\n");
                  }
               }
               else
               {
                  OWERROR_DUMP(stdout);
                  printf("didn't read device.\n");
               }

               break;

            case HYGRO_GET_TEMPERATURE:
               if(readDevice(portnum,&SNum[0],&state[0],&config))
               {
                  if(!doTemperatureConvert(portnum,&SNum[0],&state[0]))
                  {
                     OWERROR_DUMP(stdout);
                  }
                  else
                  {
                     printf("Use temperature calibration?\n");
                     config.useTemperatureCalibration = getBoolean();

                     if(config.useTemperatureCalibration)
                     {
                        val = decodeTemperature(&state[12],2,FALSE,config);
                        valsq = val*val;
                        error = config.tempCoeffA*valsq +
                           config.tempCoeffB*val + config.tempCoeffC;
                        val = val - error;
                        printf("The current temperature in C is %f\n",val);
                     }
                     else
                     {
                        printf("The current temperature is %f\n",
                           decodeTemperature(&state[12],2,FALSE,config));
                     }
                  }
               }
               else
               {
                  OWERROR_DUMP(stdout);
               }

               break;

            case HYGRO_GET_DATA:
               if(readDevice(portnum,&SNum[0],&state[0],&config))
               {
                  if(!doADConvert(portnum,&SNum[0],&state[0]))
                  {
                     OWERROR_DUMP(stdout);
                  }
                  else
                  {
                     if(config.hasHumidity)
                     {
                        printf("Use humidity calibration?\n");
                        config.useHumidityCalibration = getBoolean();

                        if(config.useHumidityCalibration)
                        {
                           val = decodeHumidity(&state[14],2,FALSE,config);
                           valsq = val*val;
                           error = config.humCoeffA*valsq +
                              config.humCoeffB*val + config.humCoeffC;
                           val = val - error;

                           if(val < 0.0)
                              val = 0.0;

                           printf("The current humidity is %f\n",val);
                        }
                        else
                        {
                           val = decodeHumidity(&state[14],2,FALSE,config);

                           if(val < 0.0)
                              val = 0.0;

                           printf("The current humidity is %f\n",val);
                        }
                     }

                     else
                     {
                        printf("The current AD voltage reading is %f\n",
                           getADVoltage(&state[14],2,FALSE,config));
                     }
                  }
               }
               else
               {
                  OWERROR_DUMP(stdout);
               }

               break;

            case HYGRO_BM_READ_PASS  :
               printf("Enter the 8 byte read only password if less 0x00 will be filled in.");

               printf("\n");
               printf("Data Entry Mode\n");
               printf("(0) Text (single line)\n");
               printf("(1) Hex (XX XX XX XX ...)\n");
               length = 1;

               length = getData(&passw[0],8,getNumber(0,length));

               if(length != 8)
               {
                  for(i=length;i<8;i++)
                     psw[i] = 0x00;
               }

               setBMPassword(&passw[0]);

               break;

            case HYGRO_BM_RW_PASS    :
               printf("Enter the 8 byte read/write password if less 0x00 will be filled in.");

               printf("\n");
               printf("Data Entry Mode\n");
               printf("(0) Text (single line)\n");
               printf("(1) Hex (XX XX XX XX ...)\n");
               length = 1;

               length = getData(&passw[0],8,getNumber(0,length));

               if(length != 8)
               {
                  for(i=length;i<8;i++)
                     psw[i] = 0x00;
               }

               setBMPassword(&passw[0]);

               break;

            case HYGRO_READ_PASS     :
               printf("Enter the 8 byte read only password if less 0x00 will be filled in.");

               printf("\n");
               printf("Data Entry Mode\n");
               printf("(0) Text (single line)\n");
               printf("(1) Hex (XX XX XX XX ...)\n");
               length = 1;

               length = getData(&passw[0],8,getNumber(0,length));

               if(length != 8)
               {
                  for(i=length;i<8;i++)
                     psw[i] = 0x00;
               }

               if(!setPassword(portnum,&SNum[0],&passw[0],READ_PSW))
                  OWERROR_DUMP(stderr);

               break;

            case HYGRO_RW_PASS       :
               printf("Enter the 8 byte read/write password if less 0x00 will be filled in.");

               printf("\n");
               printf("Data Entry Mode\n");
               printf("(0) Text (single line)\n");
               printf("(1) Hex (XX XX XX XX ...)\n");
               length = 1;

               length = getData(&passw[0],8,getNumber(0,length));

               if(length != 8)
               {
                  for(i=length;i<8;i++)
                     psw[i] = 0x00;
               }

               if(!setPassword(portnum,&SNum[0],&passw[0],READ_WRITE_PSW))
                  OWERROR_DUMP(stderr);

               break;

            case HYGRO_ENABLE_PASS   :
               if(!setPasswordMode(portnum,&SNum[0],ENABLE_PSW))
                  OWERROR_DUMP(stderr);
               break;

            case HYGRO_DISABLE_PASS  :
               if(!setPasswordMode(portnum,&SNum[0],DISABLE_PSW))
                  OWERROR_DUMP(stderr);
               break;

            case QUIT :
               done = TRUE;

            default:
               break;
         }
      }
      while(!done);

   }

   return 0;
}

/**
 * Create a menu from the provided OneWireContainer
 * Vector and allow the user to select a device.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number of the hygrochrom
 */
void selectDevice(int portnum, uchar *SNum)
{

   int i,j;
   int numDevices = 0;
   int num;
   uchar AllDevices[10][8];

   // get list of 1-Wire Humalog devices
   numDevices = FindDevices(portnum, &AllDevices[0], 0x41, 10);

   if(numDevices == 0)
   {
      for(i=0;i<8;i++)
         AllDevices[0][i] = 0x00;
      numDevices++;
   }

   printf("Device Selection\n");

   for (i=0; i<numDevices; i++)
   {
      printf("(%d) ",i);
      for(j=7;j>=0;j--)
         printf("%02X ",AllDevices[i][j]);
      printf("\n");
   }

   num = getNumber(0,numDevices-1);

   for(i=0;i<8;i++)
      SNum[i] = AllDevices[num][i];
}

/**
 * Display menu and ask for a selection.
 *
 * menu  the menu selections.
 *
 * @return numberic value entered from the console.
 */
int menuSelect()
{
   int length = 0;

   printf("\n");
   printf("Bank Operation Menu\n");
   printf("(0)  Start new mission.\n");
   printf("(1)  Stop mission.\n");
   printf("(2)  Download mission data.\n");
   printf("(3)  Read the temperature.\n");
   printf("(4)  Read the humidity or voltage.\n");
   printf("(5)  Set Bus Master read only password.\n");
   printf("(6)  Set Bus Master read/write password.\n");
   printf("(7)  Set device read only password.\n");
   printf("(8)  Set device read/write password.\n");
   printf("(9)  Enable the password.\n");
   printf("(10) Disable the password.\n");
   printf("(11) Quit.\n");
   length = 11;

   printf("\nPlease enter value: ");

   return getNumber(0, length);
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
      {
         done = TRUE;
      }
   }
   while(!done);

   return value;
}

/**
 * Retrieve user input from the console.
 *
 * @return boolean value entered from the console.
 */
SMALLINT getBoolean ()
{
   char ch;
   int done = FALSE;
   SMALLINT ret = FALSE;

   do
   {
      ch = (char) getchar();

      if(!isspace(ch))
      {
         if( (ch == 'T') || (ch == 't'))
         {
            ret = TRUE;
            done = TRUE;
         }
         else if( (ch == 'F') || (ch == 'f'))
         {
            ret = FALSE;
            done = TRUE;
         }
         else
         {
            printf("Enter either T or F.\n");
         }
      }

   }
   while(!done);

   return ret;
}

/**
 * Retrieve user input from the console.
 *
 * @return boolean value entered from the console.
 */
float getFloat (float min, float max)
{
   int cnt;
   int done = FALSE;
   float value;

   do
   {
      cnt = scanf("%f",&value);

      if((cnt>0) && (value>max || value<min))
      {
         printf("Value (%f) is outside of the limits (%f,%f)\n",value,min,max);
         printf("Try again:\n");
      }
      else
      {
         done = TRUE;
      }
   }
   while(!done);

   if(((int) (value*10)) % 5 == 0)
   {
      value = (float) (((int)(value*10))/10.0);
   }
   else
   {
      if( (((int) (value*10)) % 10) < 2)
      {
         value = (float) ((int) value);
      }
      else if( (((int) (value*10)) % 10) < 8)
      {
         value = (float) (((float) ((int) value)) + 0.5);
      }
      else
      {
         value = (float) (((float) ((int) value)) + 1.0);
      }
   }


   return value;
}

/*---------------------------------------------------------------------------
 * Copyright (C) 2002 Dallas Semiconductor Corporation, All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Dallas Semiconductor
 * shall not be used except as stated in the Dallas Semiconductor
 * Branding Policy.
 *---------------------------------------------------------------------------
 */




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
//  thermoms.c - This utility mission a DS1921
//               Thermochron iButton.
//
//  Version: 2.00
//
//    History:
//           1.03 -> 2.00  Reorganization of Public Domain Kit. Provide
//                         defaults on mission selection.
//

#define THERMOCHRON

#include <stdio.h>
#include <stdlib.h>
#include "ownet.h"
#include "thermo21.h"
#include "findtype.h"

// defines
#define MAXDEVICES   20

// local function prototypes
int InputMissionType(ThermoStateType *,int);

//----------------------------------------------------------------------
//  This is the Main routine for thermoms.
//
int main(int argc, char **argv)
{
   int Fahrenheit=FALSE,num,i,j;
   char str[800];
   ThermoStateType ThermoState;
   uchar ThermoSN[MAXDEVICES][8]; //the serial numbers for the devices
   int portnum=0;

   // check arguments to see if request instruction with '?' or too many
   if ((argc < 2) || (argc > 3) || ((argc > 1) && (argv[1][0] == '?' || argv[1][1] == '?')))
      ExitProg("\nusage: thermoms 1wire_net_name </Fahrenheit>\n"
              "  - Thermochron configuration on the 1-Wire Net port\n"
              "  - 1wire_net_port required port name\n"
              "    example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" \n"
              "    (Linux DS2480),\"1\" (Win32 TMEX)\n"
              "  - </Fahrenheit> optional Fahrenheit mode (default Celsius)\n"
              "  - version 2.00\n",1);

   // attempt to acquire the 1-Wire Net
   if((portnum = owAcquireEx(argv[1])) < 0)
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   // success
   printf("Port opened: %s\n",argv[1]);

   //----------------------------------------
   // Introduction
   printf("\n/---------------------------------------------\n");
   printf("  Find and mission DS1921 Thermochron iButton(s)\n"
          "  Version 2.00\n\n");

   // check arguments
   Fahrenheit = FALSE;
   if (argc >= 3)
   {
      if ((argv[2][0] == '/') &&
           ((argv[2][1] == 'F') || (argv[2][1] == 'f')))
         Fahrenheit = TRUE;
   }

   // get list of Thermochron's
	num = FindDevices(portnum, &ThermoSN[0],THERMO_FAM, MAXDEVICES);

   // check if not present or more then 1 present
   if (num == 0)
      ExitProg("Thermochron not present on 1-Wire\n",1);

   // loop to mission each Thermochron
   for (i = 0; i < num; i++)
   {
      // set the serial number portion in the thermo state
      printf("\nRead status of Thermochron: ");
      for (j = 7; j >= 0; j--)
      {
         ThermoState.MissStat.serial_num[j] = ThermoSN[i][j];
         printf("%02X",ThermoSN[i][j]);
      }
      printf("\n");

      // read Thermochron state
      if (ReadThermoStatus(portnum,&ThermoSN[i][0],&ThermoState,stdout))
      {
         // display mission status
         InterpretStatus(&ThermoState.MissStat);
         MissionStatusToString(&ThermoState.MissStat, Fahrenheit, &str[0]);
         printf("\n%s\n",str);

         // ask user mission questions
         if (!InputMissionType(&ThermoState,Fahrenheit))
         {
            printf("Input abort\n");
            continue;
         }

         // run the script to display the thermochron
         if (MissionThermo(portnum,&ThermoSN[i][0],&ThermoState,stdout))
         {
            // read Thermochron state
            if (ReadThermoStatus(portnum,&ThermoSN[i][0],&ThermoState,stdout))
            {
               // display the new mission status
               InterpretStatus(&ThermoState.MissStat);
               MissionStatusToString(&ThermoState.MissStat, Fahrenheit, &str[0]);
               printf("\n%s\n",str);
            }
            else
               printf("ERROR reading Thermochon state\n");
         }
         else
            printf("ERROR, Thermochon missioning not complete\n");
      }
      else
         printf("ERROR reading Thermochon state\n");
   }

   // release the 1-Wire Net
   owRelease(portnum);
   printf("\nClosing port %s.\n", argv[1]);
   printf("End program normally\n");
   exit(0);

   return 0;
}

//--------------------------------------------------------------------------
//  Prints a message, and the current driver versions.
//
int InputMissionType(ThermoStateType *ThermoState, int ConvertToF)
{
   long num;
   float temp;

   // prompt to erase current mission
   num = 1;
   if (!EnterNum("Erase current mission\n  (0) yes\n  (1) no\nAnswer:",1, &num, 0, 1))
      return FALSE;

   // check for no erase
   if (num == 1)
      return FALSE;

   // prompt for start delay
   num = 0;
   if (!EnterNum("\nEnter start delay in minutes\n"
                 "Answer (0 to 65535):",5, &num, 0, 65535))
      return FALSE;

   // set in mission status structure
   ThermoState->MissStat.start_delay = (ushort)num;

   // prompt for sample rate
   num = 5;
   if (!EnterNum("\nEnter sample rate in minutes\n"
                 "Answer (1 to 255):",3, &num, 1, 255))
      return FALSE;

   // set in mission status structure
   ThermoState->MissStat.sample_rate = (uchar)num;

   // prompt to erase current mission
   num = 0;
   if (!EnterNum("Enable roll-over\n  (0) yes\n  (1) no\nAnswer:",1, &num, 0, 1))
      return FALSE;

   // rollover enabled?
   ThermoState->MissStat.rollover_enable = (num == 0);

   // prompt for high trip
   if (ConvertToF)
   {
      num = 80;
      if (!EnterNum("\nEnter high temperature threshold in Fahrenheit\n"
                    "Answer (-40 to 158):",3, &num, -40, 158))
         return FALSE;
      temp = (float)((num - 32.0) * 5.0 / 9.0);
   }
   else
   {
      num = 35;
      if (!EnterNum("\nEnter high temperature threshold in Celsius\n"
                    "Answer (-40 to 70):",3, &num, -40, 70))
         return FALSE;
      temp = (float)num;
   }

   // set in mission status structure
   ThermoState->MissStat.high_threshold = (uchar)(2 * (temp + 40));

   // prompt for low trip
   if (ConvertToF)
   {
      num = 32;
      if (!EnterNum("\nEnter low temperature threshold in Fahrenheit\n"
                    "Answer (-40 to 158):",3, &num, -40, 158))
         return FALSE;
      temp = (float)((num - 32.0) * 5.0 / 9.0);
   }
   else
   {
      num = 0;
      if (!EnterNum("\nEnter low temperature threshold in Celsius\n"
                    "Answer (-40 to 70):",3, &num, -40, 70))
         return FALSE;
      temp = (float)num;
   }

   // set in mission status structure
   ThermoState->MissStat.low_threshold = (uchar)(2 * (temp + 40));

   return TRUE;
}


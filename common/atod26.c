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
//  atod26.c - Reads the voltage on the 1-Wire of the DS2438. 
//  version 1.00
//

// Include Files
#include <stdio.h>
#include "ownet.h"
#include "atod26.h"

/**
 * Sets the DS2438 to read Vad or Vdd
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * vdd      flag indicating weather to read the Vdd or the Vad
 * SNum     the serial number for the part that the read is
 *          to be done on.
 *
 * @return 'true' if the read was complete
 */
int SetupAtoD(int portnum, int vdd, uchar *SNum)
{
   uchar send_block[50];
   uchar test;
   int send_cnt=0;
   int i;
   ushort lastcrc8;
   int busybyte; 

   owSerialNum(portnum,SNum,FALSE);
   // Recall the Status/Configuration page
   // Recall command
   send_block[send_cnt++] = 0xB8;

   // Page to Recall
   send_block[send_cnt++] = 0x00;

   if(!owBlock(portnum,FALSE,send_block,send_cnt))
      return FALSE;

   send_cnt = 0;

   if(owAccess(portnum))
   {
      // Read the Status/Configuration byte
      // Read scratchpad command
      send_block[send_cnt++] = 0xBE;

      // Page for the Status/Configuration byte
      send_block[send_cnt++] = 0x00;

      for(i=0;i<9;i++)
         send_block[send_cnt++] = 0xFF;

      if(owBlock(portnum,FALSE,send_block,send_cnt))
      {
         setcrc8(portnum,0);

         for(i=2;i<send_cnt;i++)
            lastcrc8 = docrc8(portnum,send_block[i]);

         if(lastcrc8 != 0x00)
            return FALSE;
      }//Block
      else
         return FALSE;

      test = send_block[2] & 0x08;
      if(((test == 0x08) && vdd) || ((test == 0x00) && !(vdd)))
         return TRUE;
   }//Access

   if(owAccess(portnum))
   {
      send_cnt = 0;
      // Write the Status/Configuration byte
      // Write scratchpad command
      send_block[send_cnt++] = 0x4E;

      // Write page
      send_block[send_cnt++] = 0x00;

      if(vdd)
         send_block[send_cnt++] = send_block[2] | 0x08;
      else
         send_block[send_cnt++] = send_block[2] & 0xF7;

      for(i=0;i<7;i++)
         send_block[send_cnt++] = send_block[i+4];

      if(owBlock(portnum,FALSE,send_block,send_cnt))
      {
         send_cnt = 0;

         if(owAccess(portnum))
         {
            // Copy the Status/Configuration byte
            // Copy scratchpad command
            send_block[send_cnt++] = 0x48;

            // Copy page
            send_block[send_cnt++] = 0x00;

            if(owBlock(portnum,FALSE,send_block,send_cnt))
            {
               busybyte = owReadByte(portnum);
         
               while(busybyte == 0)
                  busybyte = owReadByte(portnum);

               return TRUE;
            }//Block
         }//Access
      }//Block

   }//Access

   return FALSE;
}
      
/**
 * Read the Vdd or the Vad from the DS2438
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * vdd      flag indicating weather to read the Vdd or the Vad
 * SNum     the serial number for the part that the read is
 *          to be done on.
 *
 * @return the floating point value of Vad or Vdd
 */
float ReadAtoD(int portnum, int vdd, uchar *SNum)
{
   uchar send_block[50];
   int send_cnt=0;
   int i;
   int busybyte; 
   ushort lastcrc8;
   ushort volts;
   float ret= (float) -1.0;
   int done = TRUE;

   do
   {
      if(SetupAtoD(portnum,vdd,SNum))
      {

         if(owAccess(portnum))
         {
            if(!owWriteByte(portnum,0xB4))
            {
               OWERROR(OWERROR_WRITE_BYTE_FAILED);
               return ret;
            }

            busybyte = owReadByte(portnum);
         
            while(busybyte == 0)
               busybyte = owReadByte(portnum);
         }

         if(owAccess(portnum))
         {
            // Recall the Status/Configuration page
            // Recall command
            send_block[send_cnt++] = 0xB8;

            // Page to Recall
            send_block[send_cnt++] = 0x00;

            if(!owBlock(portnum,FALSE,send_block,send_cnt))
               return ret;
         }

         send_cnt = 0;

         if(owAccess(portnum))
         {
            // Read the Status/Configuration byte
            // Read scratchpad command
            send_block[send_cnt++] = 0xBE;

            // Page for the Status/Configuration byte
            send_block[send_cnt++] = 0x00;

            for(i=0;i<9;i++)
               send_block[send_cnt++] = 0xFF;

            if(owBlock(portnum,FALSE,send_block,send_cnt))
            {
               setcrc8(portnum,0);

               for(i=2;i<send_cnt;i++)
                  lastcrc8 = docrc8(portnum,send_block[i]);

               if(lastcrc8 != 0x00)
                  return ret;

            }
            else
               return ret;    
      
            if((!vdd) && ((send_block[2] & 0x08) == 0x08))
               done = FALSE;
            else
               done = TRUE;

            volts = (send_block[6] << 8) | send_block[5];
            ret = (float) volts/100;
         }//Access
      }
   } while(!done);

   return ret;

}

/**
 * Reads the temperature from the DS2438.
 *
 * portnum  the port number of the port being used for the
 *          1-Wire Network.
 * SNum     the serial number for the part that the read is
 *          to be done on.
 *
 * @return the temperature in returned in C.
 */
double Get_Temperature(int portnum,uchar *SNum)
{
   double ret=-1.0;
   uchar send_block[50];
   int send_cnt=0;
   int i;
   ushort lastcrc8;

   owSerialNum(portnum,SNum,FALSE);

   if(owAccess(portnum))
   {
      // Convert Temperature command
      if(!owWriteByte(portnum,0x44))
      {
         OWERROR(OWERROR_WRITE_BYTE_FAILED);
         return FALSE;
      }
   }

   msDelay(10);

   if(owAccess(portnum))
   {
      // Recall the Status/Configuration page
      // Recall command
      send_block[send_cnt++] = 0xB8;

      // Page to Recall
      send_block[send_cnt++] = 0x00;

      if(!owBlock(portnum,FALSE,send_block,send_cnt))
         return FALSE;

      send_cnt = 0;
   }

   if(owAccess(portnum))
   {
      // Read the Status/Configuration byte
      // Read scratchpad command
      send_block[send_cnt++] = 0xBE;

      // Page for the Status/Configuration byte
      send_block[send_cnt++] = 0x00;

      for(i=0;i<9;i++)
         send_block[send_cnt++] = 0xFF;

      if(owBlock(portnum,FALSE,send_block,send_cnt))
      {
         setcrc8(portnum,0);

         for(i=2;i<send_cnt;i++)
            lastcrc8 = docrc8(portnum,send_block[i]);

         if(lastcrc8 != 0x00)
            return ret;

      }
      else
         return ret;    
      
      ret = (((send_block[4] << 8) | send_block[3]) >> 3) * 0.03125;
   }//Access

   return ret;
}


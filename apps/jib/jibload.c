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
//  jibload.c - Load a JiBLet to the Java-powered iButton(s) found on
//              the 1-Wire
//
//  version 3.00 Beta
//

#include "jib96.h"
#include <string.h>

#define MAXDEVICES 10

// external functions
extern SMALLINT  owAcquire(int,char *);
extern void      owRelease(int);
extern SMALLINT  FindDevices(int,uchar FamilySN[][8],SMALLINT,int);
extern void      PrintSerialNum(uchar*);
extern SMALLINT  owOverdriveAccess(int);
extern void      owSerialNum(int,uchar *,SMALLINT);

// local prototypes
void LoadJiB(int portnum, uchar *dev, char *filename,
             int MasterEraseFirst, int ClearLoadPINMode);
int ProcessCommandLine(int p_ArgC, char **p_lpArgV,
                       uchar *MasterEraseFirst, uchar *ClearLoadPINMode);
ulong GetFileToLoad(char *p_FileName, uchar *p_FileBuffer, ulong p_Len);


//--------------------------------------------------------------------------
// Main to load a JiBlet into a JiB
//
int main(int argc, char *argv[])
{
   char   l_DoTest    = 0;
   uchar  MasterEraseFirst  = FALSE;
   uchar  ClearLoadPINMode    = FALSE;
   int    i, portnum=0,NumDevices;
   uchar  FamilySN[MAXDEVICES][8];

   printf("\nLoadJiB version 3.00B\n\n");

   // see if another port is desired
   if ((argc < 3) ||
       (!ProcessCommandLine(argc,argv,&MasterEraseFirst,&ClearLoadPINMode)))
   {
      printf("\nUsage: LoadJiB port filename [-e] [-l] [-h]\n");
      printf("       port: required port name\n");
      printf("       filename: filename of JiBlet to load\n");
      printf("       -e: Master erase first\n");
      printf("       -l: Clear Load PIN mode\n");
      printf("       -h: Display usage\n\n");
      exit(0);
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

   if (NumDevices > 0)
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
        LoadJiB(portnum, FamilySN[i], argv[2],
                MasterEraseFirst, ClearLoadPINMode);
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
// Check command parameters for flags
//
int ProcessCommandLine(int p_ArgC, char **p_lpArgV,
                       uchar *MasterEraseFirst, uchar *ClearLoadPINMode)
{
   int l_i;

   // start looking at parameters after the two required ones
   for(l_i = 3;  l_i < p_ArgC; l_i++)
   {
      // check for flag designator
      if((p_lpArgV[l_i][0] == '/')||(p_lpArgV[l_i][0] == '-'))
      {
         // master erase first?
         if ((p_lpArgV[l_i][1] == 'e') || (p_lpArgV[l_i][1] == 'E'))
            *MasterEraseFirst = TRUE;

         // clear load pin to use signatures
         if ((p_lpArgV[l_i][1] == 'l') || (p_lpArgV[l_i][1] == 'L'))
            *ClearLoadPINMode = TRUE;

         // usage request
         if ((p_lpArgV[l_i][1] == 'h') || (p_lpArgV[l_i][1] == 'H'))
            return FALSE;
      }
   }

   return TRUE;
}

//--------------------------------------------------------------------------
// Read the file into the supplied buffer.
//
ulong GetFileToLoad(char *p_FileName, uchar *p_FileBuffer, ulong p_Len)
{
  FILE  *l_InFile;
  ulong l_BytesRead = 0,
        l_ReadSize = 0;

  if(!(l_InFile = fopen(p_FileName,"r+b")))
  {
    perror("Couldn't open file");
    return FALSE;
  }


  while((l_ReadSize = fread(p_FileBuffer+l_BytesRead, 1, p_Len-l_BytesRead, l_InFile)) > 0)
  {
    l_BytesRead += l_ReadSize;
  }

  fclose(l_InFile);

  return l_BytesRead;
}

//--------------------------------------------------------------------------
// Load the jib files into the iButton
//
void LoadJiB(int portnum, uchar *dev, char *filename,
             int MasterEraseFirst, int ClearLoadPINMode)
{
   int            l_FailureCount = 0,
                  l_TestCount    = 0;
   LPRESPONSEAPDU l_lpResponseAPDU;
   uchar          l_FileName[260],
                  l_FileBuff[4096];
   AID            l_CurrentAID;
   ulong          filelen;

   // select the device and put in overdrive
   printf("Device: ");
   PrintSerialNum(dev);
   printf("\n");

   owSerialNum(portnum,dev,0);
   if (!owOverdriveAccess(portnum))
      printf("ERROR, could not get device in overdrive!\n");

   // make sure there is time for hashing and a signature verification
   SetDefaultExecTime(800);

   if (MasterEraseFirst)
   {
      printf("Master Erasing Button: ");
      l_lpResponseAPDU = MasterErase(portnum);

      if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
         printf("\nMaster Erase Failed with SW = 0x%04hX\n",l_lpResponseAPDU->SW);
      else
         printf("OK\n");
   }

   if (ClearLoadPINMode)
   {
      printf("Clearing Load By PIN: ");
      l_lpResponseAPDU = SetLoadPINMode(portnum,0);

      if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
         printf("\nClear Load PIN Mode Failed with SW = 0x%04hX\n",l_lpResponseAPDU->SW);
      else
         printf("OK\n");
   }

   printf("Loading %s: ",filename);

   // read the file and load if found
   filelen = GetFileToLoad(filename,l_FileBuff,sizeof(l_FileBuff));
   if (filelen > 0)
   {
      // assume the filename less extension is the AID
      l_CurrentAID.Len = strlen(filename) - strlen(".jib");
      // unless, of course, that's too large
      if(l_CurrentAID.Len>MAX_AID_SIZE)
         l_CurrentAID.Len = MAX_AID_SIZE;
      memcpy(l_CurrentAID.Data, filename, l_CurrentAID.Len);

      l_lpResponseAPDU = LoadApplet(portnum, l_FileBuff, filelen ,&l_CurrentAID);

      if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
      {
         strncpy(l_FileName, filename, l_CurrentAID.Len);
         l_FileName[l_CurrentAID.Len] = 0;
         printf("\nLoad of %s Failed with SW = 0x%04hX\n",l_FileName,l_lpResponseAPDU->SW);
      }
      else
         printf("OK\n");
   }
   else
      printf("\nFile %s failed to open.\n",filename);
}

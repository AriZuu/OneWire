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
// shainitcopr.c - initializes a 1963S as a co-processor.
//
// Version: 2.10
//

#include "ownet.h"
#include "shaib.h"

extern SMALLINT owFirst(int,SMALLINT,SMALLINT);
extern SMALLINT owAcquire(int,char *);
extern void owRelease(int);
extern long msGettick();

static void GetBytes(char* msg, uchar* buffer, int len, uchar defValue,
                     SMALLINT printHex);
static int GetSecret(char* name, uchar** secret);

//---------------------------------------------------------------------------
// Finds a DS1963S and initializes it as a coprocessor for SHA eCash systems.
//
// Note: The algorithm used for signing certificates and verifying device
// authenticity is the SHA-1 algorithm as specified in the datasheet for the
// DS1961S/DS2432 and the DS2963S, where the last step of the official
// FIPS-180 SHA routine is omitted (which only involves the addition of
// constant values).
//
int main(int argc, char** argv)
{
   long lvalue = 0;
   SHACopr copr;
   int namelen, auxlen;
   uchar coprFile[255];
   uchar *authSecret, *signSecret;
   unsigned int authlen, signlen;
   char test[2] = {'y',0};
#ifdef COPRVM
   FILE* fp;
   int randomNum, i;
   unsigned int len;
#else
   FileEntry fe;
   int maxwrite = 0;
   short handle = 0;
#endif

   copr.portnum = 0;

   puts("\nStarting SHA initcopr Application\n");

#ifndef COPRVM
   // check for required port name
   if (argc != 2)
   {
      printf("1-Wire Net name required on command line!\n"
             " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
             "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      exit(1);
   }

   if((copr.portnum = owAcquireEx(argv[1])) < 0)
   {
      printf("Failed to acquire port.\n");
      exit(1);
   }

   puts("\nPlease place coprocessor token on the 1-Wire bus.\n");

   while(!FindNewSHA(copr.portnum, copr.devAN, FALSE))
   {
      if(owHasErrors())
         msDelay(10);
   }

   printf("Found device: ");
   PrintSerialNum(copr.devAN);
   puts("\n");

   GetBytes("Enter the name of the coprocessor file (4 letters): ",
                 fe.Name, 4, 0x20, FALSE);

   lvalue = 0;
   EnterNum("Enter the file extension of the coprocessor file",
                 3, &lvalue, 0, 255);
   fe.Ext =  (uchar)lvalue;
   printf("\n");
#endif

   GetBytes("Enter the name of the account service file (4 letters): ",
                 copr.serviceFilename, 4, 0x20, FALSE);

   lvalue = 102;
   EnterNum("Enter the file extension of the account service file",
                 3, &lvalue, 0, 255);
   copr.serviceFilename[4] =  (uchar)lvalue;
   printf("\n");

   lvalue = 7;
   EnterNum("Enter the authentication page number",
                 2, &lvalue, 1, 7);
   copr.authPageNumber =  (uchar)lvalue;
   printf("\n");

   lvalue = 9;
   EnterNum("Enter the workspace page number",
                 2, &lvalue, 9, 15);
   copr.wspcPageNumber =  (uchar)lvalue;
   printf("\n");

   lvalue = 1;
   EnterNum("Enter the application version number",
                 3, &lvalue, 0, 255);
   copr.versionNumber =  (uchar)lvalue;
   printf("\n");

   GetBytes("Enter the binding data in text (32 bytes): ",
                 copr.bindData, 32, 0x00, TRUE);

   //GetBytes("Enter the binding code in text (7 bytes): ",
   //              copr.bindCode, 7, 0x00, TRUE);
   memset(copr.bindCode, 0xFF, 7);

   memset(copr.initSignature, 0x00, 20);
   printf("Enter the initial signature in Hex "
          "(20 bytes, all 0x00 by default): \n");
   getData(copr.initSignature, 20, TRUE);
   PrintHex(copr.initSignature,20);
   printf("\n");

   memset(coprFile, 0x00, 255);
   namelen = EnterString("Enter the name of the service provider",
                         (char*)coprFile, 1, 50);
   copr.providerName = malloc(namelen+1);
   memcpy(copr.providerName, coprFile, namelen);
   copr.providerName[namelen] = 0;
   printf("\n\n");

   memset(coprFile, 0x00, 255);
   auxlen = EnterString("Enter any auxilliary info",
                        (char*)coprFile, 1, 100);
   copr.auxilliaryData = malloc(auxlen+1);
   memcpy(copr.auxilliaryData , coprFile, auxlen);
   copr.auxilliaryData [auxlen] = 0;
   printf("\n\n");

   lvalue = 1;
   EnterNum("Enter an encryption code", 3, &lvalue, 0, 255);
   copr.encCode =  (uchar)lvalue;
   printf("\n");

   // Set the signing challenge to all zeroes
   memset(copr.signChlg,0x00,3);

   signlen = GetSecret("System Signing Secret", &signSecret);

   authlen = GetSecret("System Authentication Secret", &authSecret);

   EnterString("Reformat the secret for DS1961S compatibility", test, 1, 1);
   if(test[0] == 'y')
   {
      ReformatSecretFor1961S(authSecret, authlen);
      PrintHex(authSecret, authlen);
      printf("\n");
      copr.ds1961Scompatible = 0x55;
   }

#ifndef COPRVM
   // now the fun begins
   copr.signPageNumber = 8;

   printf("Installing signing secret on page %d, secret %d, length %d\n",
          copr.signPageNumber, copr.signPageNumber&7, signlen);
   PrintHex(signSecret, signlen);

   if(!InstallSystemSecret18(copr.portnum,
                             copr.signPageNumber,
                             copr.signPageNumber&7,
                             signSecret, signlen, FALSE))
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   printf("Installing authentication secret on page %d, secret %d, length %d\n",
          copr.authPageNumber, copr.authPageNumber&7, authlen);
   PrintHex(authSecret, authlen);

   if(!InstallSystemSecret18(copr.portnum,
                             copr.authPageNumber,
                             copr.authPageNumber&7,
                             authSecret, authlen, TRUE))
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

#endif

   memcpy(coprFile, copr.serviceFilename, 5);
   coprFile[5] = copr.signPageNumber;
   coprFile[6] = copr.authPageNumber;
   coprFile[7] = copr.wspcPageNumber;
   coprFile[8] = copr.versionNumber;
   memcpy(&coprFile[13], copr.bindData, 32);
   memcpy(&coprFile[45], copr.bindCode, 7);
   memcpy(&coprFile[52], copr.signChlg, 3);
   coprFile[55] = namelen;
   coprFile[56] = 20;
   coprFile[57] = auxlen;
   memcpy(&coprFile[58], copr.providerName, namelen );
   memcpy(&coprFile[58+namelen], copr.initSignature, 20 );
   memcpy(&coprFile[78+namelen], copr.auxilliaryData, auxlen );
   coprFile[78+namelen+auxlen] = copr.encCode;
   coprFile[79+namelen+auxlen] = copr.ds1961Scompatible;

#ifdef COPRVM
   // open up a file, maybe "shaVM.cnf"
   fp = fopen(SHACoprFilename, "wb");
   if(fp==NULL)
   {
      printf("Error opening file: %s", SHACoprFilename);
      exit(1);
   }
   len = 80+namelen+auxlen;
   if(fwrite(coprFile, 1, len, fp)!=len ||
      fwrite((uchar*)&signlen, 1, 1, fp)!=1 ||
      fwrite(signSecret, 1, signlen, fp)!=signlen ||
      fwrite((uchar*)&authlen, 1, 1, fp)!=1 ||
      fwrite(authSecret, 1, authlen, fp)!=authlen)
   {
      printf("Could not write coprocessor config file");
      exit(1);
   }
#else
   //delete any old files
   if((!owFormat(copr.portnum, copr.devAN)) ||
      (!owCreateFile(copr.portnum, copr.devAN, &maxwrite, &handle, &fe)))
   {
      OWERROR_DUMP(stdout);
      exit(1);
   }

   printf("File length %d\n", 80+namelen+auxlen);
   PrintHex(coprFile, 80+namelen+auxlen);

   //check for enough space
   if(maxwrite>80+namelen+auxlen)
   {
      //write the file to the device
      if(!owWriteFile(copr.portnum, copr.devAN, handle, coprFile, 80+namelen+auxlen))
      {
         OWERROR_DUMP(stdout);
         exit(1);
      }
   }
   else
   {
      printf("\n\nNot enough room on device!\n");
   }

   // and we're done
   owRelease(copr.portnum);
#endif

   // program is about to exit, but we may as well free these
   // up anyways...
   free(copr.auxilliaryData);
   free(copr.providerName);
   free(signSecret);
   free(authSecret);

   printf("Successfully set up coprocessor!\n");

   return 0;
}

static int GetSecret(char* name, uchar** secret)
{
   uchar inputBuffer[255];
   long lvalue=1, length;

   printf("How would you like to enter the %s?\n", name);
   EnterNum("\n   1) Hex\n   2) Text\n", 1, &lvalue, 1, 2);

   lvalue = getData(inputBuffer, 255, (lvalue==1));

   if(lvalue%47!=0)
      length = ((lvalue/47) + 1)*47;
   else
      length = lvalue;
   *secret = malloc(length);
   memset(*secret, 0x00, length);
   memcpy(*secret, inputBuffer, lvalue);

   printf("length=%ld\n",length);
   PrintHex(*secret, length);
   printf("\n");

   return length;
}

static void GetBytes(char* msg,
                     uchar* buffer, int len,
                     uchar defValue,
                     SMALLINT printHex)
{
   memset(buffer,defValue,len);
   printf("%s", msg);
   getData(buffer, len, FALSE);
   if(printHex)
   {
      PrintHex(buffer,len);
   }
   else
   {
      printf("  You entered: '");
      PrintChars(buffer, len);
      printf("'\n");
   }
   printf("\n");
}


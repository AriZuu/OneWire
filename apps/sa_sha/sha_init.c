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
// sha_init.c - main function for initializing software authentication
//            application.
//
#include "ownet.h"
#include "shaib.h"

// file where the hard code data is for authentication
#define SA_Filename  "sha_data.cnf"

// local functions
SMALLINT UpdateServiceDataSA(SHACopr* copr, SHAUser* user, uchar* data);
SMALLINT InstallServiceDataSA(SHACopr* copr, SHAUser* user,
                              uchar* secret, int secret_length,
                              uchar* data);
int getNumber (int min, int max);


static uchar sign_secret[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};

// main body of application
int main(int argc, char** argv)
{
   int i = 0;
   SHACopr copr;
   SHAUser user;
   int   len, data_type;
   FileEntry fe = {"COPR",0};
   uchar authSecret[47];
   int authlen = 21;
   uchar signSecret[11] = {(uchar)'s',(uchar)'i',(uchar)'g',(uchar)'n',
                           (uchar)' ',(uchar)'s',(uchar)'e',(uchar)'c',
                           (uchar)'r',(uchar)'e',(uchar)'t'};
   int signlen = 11;
   uchar data[7];
   char test[2] = {'y',0};

   copr.portnum = 0;
   user.portnum = 0;

   puts("\nStarting Software Authentication setup Application\n");

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

   user.portnum = copr.portnum;

   // MANUALLY SETTING DATA FOR COPROCESSOR
   memcpy(copr.serviceFilename, "DLSM", 4);
   copr.serviceFilename[4] = (uchar) 102;
   copr.signPageNumber     = (uchar) 8;
   copr.authPageNumber     = (uchar) 7;
   copr.wspcPageNumber     = (uchar) 9;
   copr.versionNumber      = (uchar) 1;
   memcpy(copr.bindCode, "bindcde", 7);
   for(i=0;i<8;i++)
   {
      memcpy(&copr.bindData[i*4], "bind", 4);
   }
   copr.encCode           = 0x01;
   copr.ds1961Scompatible = 0x01;

   // Prompt for Password and User data
   printf("Enter up to 47 bytes of the Authentication Secret.\n");
   printf("  Data Entry Mode\n");
   printf("  (0) Text (single line)\n");
   printf("  (1) Hex (XX XX XX XX ...)\n");
   data_type = getNumber(0,1);
   len = getData(authSecret,47,data_type);
   // padd the data with spaces or 0's depending on type
   if(len < 47)
   {
      for(i=len;i<47;i++)
            authSecret[i] = 0x00;
   }
   PrintHex(authSecret, 47);
   printf("\n");

   printf("Enter up to 7 bytes of user data.\n");
   printf("  Data Entry Mode\n");
   printf("  (0) Text (single line)\n");
   printf("  (1) Hex (XX XX XX XX ...)\n");
   data_type = getNumber(0,1);
   len = getData(data,7,data_type);
   // padd the data with spaces or 0's depending on type
   if(len < 7)
   {
      for(i=len;i<7;i++)
      {
         if (data_type == 1)
            data[i] = 0x00;
         else
            data[i] = 0x20;
      }
   }
   PrintHex(data, 7);
   printf("\n");

   ReformatSecretFor1961S(authSecret, authlen);
   //Debug: PrintHex(authSecret, authlen);
   printf("\n");
   copr.ds1961Scompatible = 0x55;
   InstallAuthSecretVM(&copr, authSecret, authlen);
   InstallSignSecretVM(&copr, signSecret, signlen);

   // Find the token to initialize
   puts("\nPlease place user token on the 1-Wire bus.\n");
   do
   {
      while(!FindNewSHA(user.portnum, user.devAN, (i==0)))
      {
         if(owHasErrors())
         {
            msDelay(10);
         }
      }
      i++;
   }
   while(user.devAN[7]==copr.devAN[7]);
   // just check the crc of the two devices

   // Initialize the token
   printf("Installing Service Data on device: ");
   PrintSerialNum(user.devAN);
   puts("\n");
   if(InstallServiceDataSA(&copr, &user, authSecret, authlen, data))
   {
      puts("User token successfully set up");
   }
   else
   {
      puts("User token setup failed");
      OWERROR_DUMP(stdout);
   }

   // and we're done
   owRelease(copr.portnum);

   return 0;
}

//-------------------------------------------------------------------------
// Installs new service data on a user token.
//
// 'copr'      - Structure for holding coprocessor information.
// 'user'      - Structure for holding user token information.
// 'secret'    - the authentication secret to install on user token.
//
// Return: If TRUE, new service installation succeeded.
//         If FALSE, an error occurred.
//
SMALLINT InstallServiceDataSA(SHACopr* copr, SHAUser* user,
                              uchar* secret, int secret_length,
                              uchar* data)
{
   short handle;
   int maxwrite;
   FileEntry fe;
   uchar fullBindCode[15];

   //make sure user has a file directory structure
   memcpy(fe.Name, copr->serviceFilename, 4);
   fe.Ext = copr->serviceFilename[4];

   // install master authentication secret
   if(user->devAN[0]==0x18)
   {
      //need to format the device
      if(!owFormat(user->portnum, user->devAN))
         return FALSE;

      //and create an empty stub for his account information
      if(!owCreateFile(user->portnum, user->devAN,
                       &maxwrite, &handle, &fe))
         return FALSE;

      //need to know what page the stub is on
      user->accountPageNumber = fe.Spage;

      // set the serial number to that of the user
      owSerialNum(user->portnum, user->devAN, FALSE);
      OWASSERT( InstallSystemSecret18(user->portnum,
                                      user->accountPageNumber,
                                      user->accountPageNumber&7,
                                      secret, secret_length, FALSE),
                OWERROR_INSTALL_SECRET_FAILED, FALSE );
   }
   else if((user->devAN[0]==0x33) || (user->devAN[0]==0xB3))
   {
      // set the serial number to that of the user
      owSerialNum(user->portnum, user->devAN, FALSE);
      //because of copy-authorization, we need to install the
      //secret first on the DS1961S and _then_ format the system
      OWASSERT( InstallSystemSecret33(user->portnum,
                                      0,
                                      0,
                                      secret, secret_length, FALSE),
                OWERROR_INSTALL_SECRET_FAILED, FALSE );

      //need to format the device
      if(!owFormat(user->portnum, user->devAN))
         return FALSE;

      //and create an empty stub for his account information
      if(!owCreateFile(user->portnum, user->devAN,
                       &maxwrite, &handle, &fe))
         return FALSE;

      //need to know what page the stub is on
      user->accountPageNumber = fe.Spage;
   }
   else
   {
      return FALSE;
   }

   // format the bind code properly
   // first four bytes of bind code
   memcpy(fullBindCode, copr->bindCode, 4);
   // followed by the pagenumber
   fullBindCode[4] = (uchar)user->accountPageNumber;
   // and 7 bytes of the address of current device
   memcpy(&fullBindCode[5], user->devAN, 7);
   // followed by the last 3 bytes of bind code
   memcpy(&fullBindCode[12], &(copr->bindCode[4]), 3);

   // create a unique secret for iButton
   if(user->devAN[0]==0x18)
   {
      OWASSERT( BindSecretToiButton18(user->portnum,
                                      user->accountPageNumber,
                                      user->accountPageNumber&7,
                                      copr->bindData, fullBindCode, TRUE),
                OWERROR_BIND_SECRET_FAILED, FALSE );

      // do a read just to get value of writecycle counter
      user->writeCycleCounter = ReadAuthPageSHA18(user->portnum,
                                                  user->accountPageNumber,
                                                  user->accountFile,
                                                  NULL,
                                                  TRUE);

   }
   else if((user->devAN[0]==0x33) || (user->devAN[0]==0xB3))
   {
      OWASSERT( BindSecretToiButton33(user->portnum,
                                      user->accountPageNumber,
                                      user->accountPageNumber&7,
                                      copr->bindData, fullBindCode, TRUE),
                OWERROR_BIND_SECRET_FAILED, FALSE );

      // Call VerifyUser just to get the user's secret in wspc
      if(!VerifyUser(copr, user, TRUE))
         return FALSE;
   }

   // set transaction ID
   user->accountFile[27] = 0;
   user->accountFile[28] = 0;
   user->accountFile[29] = 0;

   //sign the data with coprocessor and write it out
   return UpdateServiceDataSA(copr, user, data);
}

//-------------------------------------------------------------------------
// Updates service data on a user token.  This includes signing the
// data if the part is a DS1963S.
//
// 'copr'      - Structure for holding coprocessor information.
// 'user'      - Structure for holding user token information.
//
// Return: If TRUE, update succeeded.
//         If FALSE, an error occurred.
//
SMALLINT UpdateServiceDataSA(SHACopr* copr, SHAUser* user, uchar* data)
{
   ushort crc16, i;
   uchar scratchpad[32];

   // make sure length is right.
   user->accountFile[0] = 29;

   // clear out the old signature and CRC
   memcpy(&user->accountFile[2], copr->initSignature, 20);
   memset(&user->accountFile[30], 0x00, 2);

   // reset data type code
   user->accountFile[1] = 0;

   for(i=0;i<7;i++)
      user->accountFile[i+22] = data[i];

   if((user->devAN[0]==0x33) || (user->devAN[0]==0xB3))
   {
      // ---  Set up the scratchpad for signing
      memset(scratchpad, 0x00, 32);
      // the write cycle counter +1 (since we are about to write this to it)
      if(user->writeCycleCounter>0)
         IntToBytes(&scratchpad[8], 4, user->writeCycleCounter+1);
      else
         // user doesn't have write cycle counter (DS1961S)
         memset(&scratchpad[8], 0x0FF, 4);
      // the pagenumber
      scratchpad[12] = (uchar)user->accountPageNumber;
      // and 7 bytes of the address of current device
      memcpy(&scratchpad[13], user->devAN, 7);
      // the coprocessor's signing challenge
      memcpy(&scratchpad[20], copr->signChlg, 3);

      OWASSERT( CreateDataSignatureVM(copr, sign_secret,
                                      user->accountFile,
                                      scratchpad,
                                      &user->accountFile[2],
                                      TRUE),
                OWERROR_SIGN_SERVICE_DATA_FAILED, FALSE );
   }

   //add the crc at the end of the data.
   setcrc16(user->portnum, user->accountPageNumber);
   for (i = 0; i < 30; i++)
      crc16 = docrc16(user->portnum,user->accountFile[i]);
   crc16 = ~crc16;
   user->accountFile[30] = (uchar)crc16;
   user->accountFile[31] = (uchar)(crc16>>8);

   // set the serial number to that of the user
   owSerialNum(user->portnum, user->devAN, FALSE);

   if(user->devAN[0]==0x18)
   {
      //DS1963S - not too tough
      OWASSERT( WriteDataPageSHA18(user->portnum,
                                   user->accountPageNumber,
                                   user->accountFile, FALSE),
                OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );
   }
   else if((user->devAN[0]==0x33) || (user->devAN[0]==0xB3))
   {
      OWASSERT( WriteDataPageSHA33(copr, user),
                OWERROR_WRITE_DATA_PAGE_FAILED, FALSE);
   }
   else
      return FALSE;

   return TRUE;
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
         done = TRUE;

   }
   while(!done);

   return value;
}

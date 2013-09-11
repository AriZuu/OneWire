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
// sha_chk.c - main function for checking software authentication.
//

#include "ownet.h"
#include "shaib.h"

// file where the hard code data is for authentication
#define SA_Filename  "sa_data.cnf"

// local function
SMALLINT FindUserSA(SHAUser* user, FileEntry* fe,
                     SMALLINT doBlocking);
int getNumber (int min, int max);


int main(int argc, char** argv)
{
   FileEntry fe;
   int len;
   int i;
   uchar authSecret[47];
   int authlen = 21;
   uchar signSecret[11] = {(uchar)'s',(uchar)'i',(uchar)'g',(uchar)'n',
                           (uchar)' ',(uchar)'s',(uchar)'e',(uchar)'c',
                           (uchar)'r',(uchar)'e',(uchar)'t'};
   int signlen = 11;
   int ischars;


   SHACopr copr;
   SHAUser user;

   //could be different ports.
   copr.portnum = 0;
   user.portnum = 0;

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

   // Prompt for password
   printf("Enter up to 47 bytes of the Authentication Secret.\n");
   printf("  Data Entry Mode\n");
   printf("  (0) Text (single line)\n");
   printf("  (1) Hex (XX XX XX XX ...)\n");
   len = getData(authSecret,47,getNumber(0,1));
   // padd the data with spaces or 0's depending on type
   if(len < 47)
   {
      for(i = len; i < 47; i++)
         authSecret[i] = 0x00;
   }

   ReformatSecretFor1961S(authSecret, authlen);
   copr.ds1961Scompatible = 0x55;
   InstallAuthSecretVM(&copr, authSecret, authlen);
   InstallSignSecretVM(&copr, signSecret, signlen);

   puts("\nStarting SHA Software Authentication\n");
   puts("\nPlease place token on the 1-Wire bus.\n");

   memcpy(fe.Name, "DLSM", 4);
   fe.Ext = 102;

   for(;;)
   {
      if(FindUserSA(&user,&fe,FALSE))
      {
         if(VerifyUser(&copr, &user, TRUE))
         {
            PrintSerialNum(&user.devAN[0]);
            printf(", user data = ");
            // print the user data
            ischars = TRUE;
            for(i=0;i<7;i++)
            {
               if ((user.accountFile[i+22] < 0x20) ||
                   (user.accountFile[i+22] > 0x7E))
                   ischars = FALSE;
            }
            for(i=0;i<7;i++)
            {
               if (ischars)
                  printf("%c", user.accountFile[i+22]);
               else
                  printf("%02X ", user.accountFile[i+22]);
            }
            printf(", VALID\n");
            FindNewSHA(user.portnum, &user.devAN[0], TRUE);
         }
         else
         {
            PrintSerialNum(&user.devAN[0]);
            printf(", invalid\n");
            FindNewSHA(user.portnum, &user.devAN[0], TRUE);
         }
      }
      else
      {
         printf("NO DEVICE, invalid\n");
         FindNewSHA(user.portnum, &user.devAN[0], TRUE);
      }
   }

   owRelease(copr.portnum);

   return TRUE;
}

//---------------------------------------------------------------------
// Uses File I/O API to find the user token with a specific
// service file name.  Usually 'DSLM.102'.
//
// 'user'       - Structure for holding user token information
// 'fe'         - pointer to file entry structure, with proper
//                service filename.
// 'doBlocking' - if TRUE, method blocks until a user token is found.
//
// Returns: TRUE, found a valid user token
//          FALSE, no user token is present
//
SMALLINT FindUserSA(SHAUser* user, FileEntry* fe,
                     SMALLINT doBlocking)
{
   SMALLINT FoundUser = FALSE;

   if(FindNewSHA(user->portnum, user->devAN, TRUE))
   {
      short handle;

      if(owOpenFile(user->portnum, user->devAN, fe, &handle))
      {
         user->accountPageNumber = fe->Spage;
         FoundUser = TRUE;
         owCloseFile(user->portnum, user->devAN, handle);
      }
      else
      {
         PrintSerialNum(user->devAN);
         printf(" is not a SHA User.");
      }
   }

   for(;;)
   {
      // now get all the SHA iButton parts until we find
      // one that has the right file on it.
      while(!FoundUser && FindNewSHA(user->portnum, user->devAN, FALSE))
      {
         short handle;

         if(owOpenFile(user->portnum, user->devAN, fe, &handle))
         {
            user->accountPageNumber = fe->Spage;
            FoundUser = TRUE;
            owCloseFile(user->portnum, user->devAN, handle);
         }
         else
         {
            PrintSerialNum(user->devAN);
            printf(" is not a SHA User.");
         }
      }

      if(FoundUser)
         return TRUE;
      else if(!doBlocking)
         return FALSE;
   }
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

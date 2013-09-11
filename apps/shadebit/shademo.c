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
// shadebitdemo.c - main function for debit application.
//
// Version: 2.10
//

#include "ownet.h"
#include "shaib.h"
#include <fcntl.h>

//------------------------------------------------------------------------------
// Main routine for SHA debit demo application.  Finds coprocessor first.  Then
// waits for user tokens.  When a user token arrives, it is authenticated and
// debited.
//
// Note: The algorithm used for signing certificates and verifying device
// authenticity is the SHA-1 algorithm as specified in the datasheet for the
// DS1961S/DS2432 and the DS2963S, where the last step of the official
// FIPS-180 SHA routine is omitted (which only involves the addition of
// constant values).
//
int main(int argc, char** argv)
{
   int oflags = 0;
   FileEntry fe = {"COPR",0};
   //timing variables
   long a,b,c,d,e;
   //hold's user's old balance and new balance from acct info
   int oldBalance, newBalance;
   char inputChar = 0;
   SMALLINT foundUser = FALSE;

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

   if((user.portnum = copr.portnum = owAcquireEx(argv[1])) < 0)
   {
      printf("Failed to acquire port.\n");
      exit(1);
   }

#ifdef COPRVM
   // virtual coprocessor
   if(!GetCoprVM(&copr, &fe))
      exit(1);
   printf("Found coprocessor:  EMULATED");
   printf("\n   Provider: %s\n        Aux: %s \n",
      copr.providerName, copr.auxilliaryData);
#else
   // setup file entry struct with the file we are looking for
   //memcpy(fe.Name, "COPR", 4);
   //fe.Ext = 0;

   if(FindCoprSHA(&copr, &fe))
   {
      printf("Found coprocessor:  ");
      PrintSerialNum(copr.devAN);
      printf("\n   Provider: %s\n        Aux: %s \n",
         copr.providerName, copr.auxilliaryData);
   }
   else
   {
      printf("Could not find coprocessor!\n");
      exit(1);
   }
#endif


   puts("\nStarting SHA Debit Application");
   puts("  Press 'q' followed by enter or ctrl-c to quit.\n");
   puts("\nPlease place user token on the 1-Wire bus.\n");
   puts("If user token is currently on the bus, take it off");
   puts("the bus now and, after a second delay, put it back on.\n");

   //get user account data filename
   memcpy(fe.Name, copr.serviceFilename, 4);
   fe.Ext = copr.serviceFilename[4];

#ifdef O_NONBLOCK // needed for systems where ctrl-c
                  // doesn't work, harmless on others
   // turn off blocking I/O
   if ((oflags = fcntl(0, F_GETFL, 0)) < 0 ||
       fcntl(0, F_SETFL, oflags | O_NONBLOCK) < 0)
   {
      printf("std I/O Failure!\n");
      return -1;
   }
#endif

   for(;inputChar!='q' && !key_abort();)
   {
      do
      {
         if(owHasErrors())
         {
            fflush(stdout);
            msDelay(100);
         }

      #ifdef O_NONBLOCK // needed for systems where ctrl-c
                        // doesn't work, harmless on others
         if(fread(&inputChar, 1, 1, stdin)<=0)
            inputChar = '0';
      #endif

         a = msGettick();
         foundUser = FindUserSHA(&user, &fe, FALSE);
      }
      while(!foundUser && inputChar!='q' && !key_abort());

      if(!foundUser)
         continue;

      b = msGettick();
      if(VerifyUser(&copr, &user, TRUE))
      {
         c = msGettick();
         if(VerifyData(&copr, &user))
         {
            d = msGettick();
            oldBalance = GetBalance(&user);
            if(ExecuteTransaction(&copr, &user, 50, TRUE))
            {
               e = msGettick();
               newBalance = GetBalance(&user);
               puts("\n--------------------------------\n");
               puts("Complete Success");
               printf("User token: ");
               PrintSerialNum(user.devAN);
               printf("\noldBalance: $%.2f\n",((double)oldBalance/100));
               printf("newBalance: $%.2f\n",((double)newBalance/100));
               printf(" find time: %ld ms\n",b-a);
               printf(" auth time: %ld ms\n",c-b);
               printf(" verf time: %ld ms\n",d-c);
               printf(" exec time: %ld ms\n",e-d);
               printf("total time: %ld ms\n",e-a);
               puts("\n--------------------------------\n");
            }
            else
            {
               printf("execute transaction failed\n");
               OWERROR_DUMP(stdout);
            }
         }
         else
         {
            printf("verify data failed\n");
            OWERROR_DUMP(stdout);
         }
      }
      else
      {
         printf("verify user failed\n");
         OWERROR_DUMP(stdout);
      }
   }

   owRelease(copr.portnum);

   return TRUE;
}

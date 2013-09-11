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
// shadebitvm.c - Transaction-level functions for SHA Debits without a
//                hardware coprocessor.  Also, this file contains some
//                Protocol-level functions to replace their 'hardware-only'
//                counterparts..
//
// Version: 2.10

#include "ownet.h"
#include "shaib.h"

#define MAX_RETRY_CNT 255

// static functions
static int GetSecretVM(char* name, uchar** secret);

// static global vars
static uchar sign_secret[8];
static uchar auth_secret[8];
static uchar wspc_secret[8];


//-------------------------------------------------------------------------
// Returns the user's current balance as an int
//
// 'user'      - Structure for holding user token information.
//
// Return: Current value of the user's balance.
//
int GetBalance(SHAUser* user)
{
   int balance = -1;

   if(user->devAN[0]==0x18)
      balance = BytesToInt(((DebitFile*)user->accountFile)->balanceBytes, 3);
   else if((user->devAN[0]&0x7F)==0x33)
   {
      DebitFile33* acctFile = (DebitFile33*)user->accountFile;
      if(acctFile->fileLength==RECORD_A_LENGTH)
         balance = BytesToInt(acctFile->balanceBytes_A, 3);
      else if(acctFile->fileLength==RECORD_B_LENGTH)
         balance = BytesToInt(acctFile->balanceBytes_B, 3);
      else
         OWERROR(OWERROR_BAD_SERVICE_DATA);
   }
   else
      OWERROR(OWERROR_BAD_SERVICE_DATA);

   return balance;
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
SMALLINT InstallServiceData(SHACopr* copr, SHAUser* user,
                            uchar* secret, int secret_length,
                            int initialBalance)
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
   else if((user->devAN[0]&0x7F)==0x33)
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
	  DebitFile* accountFile = (DebitFile*)user->accountFile;

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

      //setup user account file with initial balance
      IntToBytes(accountFile->balanceBytes,3,initialBalance);

      // set transaction ID
      accountFile->transID[0] = 0;
      accountFile->transID[0] = 0;

   }
   else if((user->devAN[0]&0x7F)==0x33)
   {
      DebitFile33* accountFile33 = (DebitFile33*)user->accountFile;

      OWASSERT( BindSecretToiButton33(user->portnum,
                                      user->accountPageNumber,
                                      0,
                                      copr->bindData, fullBindCode, TRUE),
                OWERROR_BIND_SECRET_FAILED, FALSE );

      // Call VerifyUser just to get the user's secret in wspc
      if(!VerifyUser(copr, user, TRUE))
         return FALSE;

      //Record A
      //setup user account file with initial balance
      IntToBytes(accountFile33->balanceBytes_A,3,initialBalance);

      // set transaction ID
      accountFile33->transID_A[0] = 0;
      accountFile33->transID_A[0] = 0;

      //Record B
	   //setup user account file with initial balance
      IntToBytes(accountFile33->balanceBytes_B,3,initialBalance);

      // set transaction ID
      accountFile33->transID_B[0] = 0;
      accountFile33->transID_B[0] = 0;
   }

   //sign the data with coprocessor and write it out
   return UpdateServiceData(copr, user);
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
SMALLINT UpdateServiceData(SHACopr* copr, SHAUser* user)
{
   ushort crc16, i;
   uchar scratchpad[32];

   if(user->devAN[0] == 0x18)
   {
      DebitFile* accountFile = (DebitFile*)user->accountFile;

      // make sure length is right.
      accountFile->fileLength = 29;

      // update transaction ID
      // doesn't matter what it is, just needs to change
      accountFile->transID[0] += 1;
      if(accountFile->transID[0]==0)
        accountFile->transID[1] += 1;

      // conversion factor - 2 data bytes
      accountFile->convFactor[0] = (uchar)0x8B;
      accountFile->convFactor[1] = (uchar)0x48;

      // clear out the old signature and CRC
      memcpy(accountFile->signature, copr->initSignature, 20);
      memset(accountFile->crc16, 0x00, 2);

      // reset data type code
      accountFile->dataTypeCode = 1;

      //file doesn't continue on another page
      accountFile->contPtr = 0;

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
                                      accountFile->signature,
                                      TRUE),
                OWERROR_SIGN_SERVICE_DATA_FAILED, FALSE );

      //add the crc at the end of the data.
      setcrc16(user->portnum, user->accountPageNumber);
      for (i = 0; i < 30; i++)
         crc16 = docrc16(user->portnum,user->accountFile[i]);
      crc16 = ~crc16;
      accountFile->crc16[0] = (uchar)crc16;
      accountFile->crc16[1] = (uchar)(crc16>>8);
   }
   else if((user->devAN[0]&0x7F)==0x33)
   {
      DebitFile33* accountFile33 = (DebitFile33*)user->accountFile;

      // conversion factor - 2 data bytes
      accountFile33->convFactor[0] = (uchar)0x8B;
      accountFile33->convFactor[1] = (uchar)0x48;

      // reset data type code
      accountFile33->dataTypeCode = 1;

      if(accountFile33->fileLength==RECORD_A_LENGTH)
      {
         // Switch to Record B
         accountFile33->fileLength = RECORD_B_LENGTH;

         // update transaction ID
         // doesn't matter what it is, just needs to change
         accountFile33->transID_B[0] += 1;
         if(accountFile33->transID_B[0]==0)
           accountFile33->transID_B[1] += 1;

         //file doesn't continue on another page
         accountFile33->contPtr_B = 0;

         // clear out the old CRC
         memset(accountFile33->crc16_B, 0x00, 2);

         //add the crc at the end of the data.
         setcrc16(user->portnum, user->accountPageNumber);
         for (i = 0; i < RECORD_B_LENGTH+1; i++)
            crc16 = docrc16(user->portnum,user->accountFile[i]);
         crc16 = ~crc16;
         accountFile33->crc16_B[0] = (uchar)crc16;
         accountFile33->crc16_B[1] = (uchar)(crc16>>8);
      }
      else
      {
         // Switch to Record B
         accountFile33->fileLength = RECORD_A_LENGTH;

         // update transaction ID
         // doesn't matter what it is, just needs to change
         accountFile33->transID_A[0] += 1;
         if(accountFile33->transID_A[0]==0)
           accountFile33->transID_A[1] += 1;

         //file doesn't continue on another page
         accountFile33->contPtr_A = 0;

         // clear out the old CRC
         memset(accountFile33->crc16_A, 0x00, 2);

         //add the crc at the end of the data.
         setcrc16(user->portnum, user->accountPageNumber);
         for (i = 0; i < RECORD_A_LENGTH+1; i++)
            crc16 = docrc16(user->portnum,user->accountFile[i]);
         crc16 = ~crc16;
         accountFile33->crc16_A[0] = (uchar)crc16;
         accountFile33->crc16_A[1] = (uchar)(crc16>>8);
      }

   }

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
   else if((user->devAN[0]&0x7F)==0x33)
   {
      OWASSERT( WriteDataPageSHA33(copr, user),
                OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );
   }
   else
      return FALSE;

   return TRUE;
}

//-------------------------------------------------------------------------
// Verifies a user token as a valid member of the system.  First, a random
// challenge is generated.  Then, the user must answer that challenge.
// The user's response signature is then verified against the proper
// response..
//
// 'copr'      - Structure for holding coprocessor information.
// 'user'      - Structure for holding user token information.
// 'doBind'    - if true, the user's unique secret is recreated on the
//               coprocessor.  If this function is called multiple times,
//               it is acceptable to skip the bind for all calls after
//               the first on the same user token.
//
// Return: If TRUE, user was verified.
//         If FALSE, an error occurred or user verification failed.
//
SMALLINT VerifyUser(SHACopr* copr, SHAUser* user, SMALLINT doBind)
{
   uchar chlg[3];
   int randomNum, i;

   // Create Challenge
   randomNum = rand();
   for (i = 0; i < 3; i++)
   {
      chlg[i] = (uchar)randomNum;
      randomNum >>= 8;
   }

   OWASSERT( AnswerChallenge(user, chlg)>=0,
             OWERROR_ANSWER_CHALLENGE_FAILED, FALSE );

   if(!VerifyAuthResponseVM(copr, user, chlg, doBind))
   {
      OWERROR(OWERROR_VERIFY_AUTH_RESPONSE_FAILED);
      if((user->devAN[0]&0x7F)==0x33)
         RefreshPage33(user->portnum, user->accountPageNumber, FALSE);
      return FALSE;
   }

   return TRUE;
}

//-------------------------------------------------------------------------
// Verifies service data on a user token as a valid member of the system.
// Pre-condition: must call verify user first.
//
// 'copr'      - Structure for holding coprocessor information.
// 'user'      - Structure for holding user token information.
//
// Return: If TRUE, data was verified.
//         If FALSE, an error occurred or data verification failed.
//
SMALLINT VerifyData(SHACopr* copr, SHAUser* user)
{
   uchar scratchpad[32];
   DebitFile acctFile;
   ushort lastcrc16;
   uchar i;

   // Check the CRC of the file
   setcrc16(user->portnum, user->accountPageNumber);
   for (i = 0; i < user->accountFile[0]+3; i++)
      lastcrc16 = docrc16(user->portnum,user->accountFile[i]);

   if((user->devAN[0]&0x7F)==0x33)
   {
      // save the original file length
      uchar origPtr = user->accountFile[0];
      SMALLINT validA = FALSE, validB = FALSE, validPtr = FALSE;

      // is the file length a valid value
      if(origPtr==RECORD_A_LENGTH || origPtr==RECORD_B_LENGTH)
         validPtr = TRUE;

      // was the crc of the file correct?
      if(lastcrc16==0xB001)
      {
         // if the header points to a valid record, we're done
         if(validPtr)
         {
            // nothing more to check for DS1961S/DS2432, since
            // it carries no signed data, we're finished
            return TRUE;
         }

         // header points to neither record A nor B, but
         // crc is absolutely correct.  that can only mean
         // we're looking at something that is the wrong
         // size from what was expected, but apparently is
         // exactly what was meant to be written.  I'm done.
         OWERROR(OWERROR_BAD_SERVICE_DATA);
         return FALSE;
      }

      // lets try Record A and check the crc
      user->accountFile[0] = RECORD_A_LENGTH;
      setcrc16(user->portnum, user->accountPageNumber);
      for (i = 0; i < user->accountFile[0]+3; i++)
         lastcrc16 = docrc16(user->portnum,user->accountFile[i]);
      if(lastcrc16==0xB001)
         validA = TRUE;

      // lets try Record B and check the crc
      user->accountFile[0] = RECORD_B_LENGTH;
      setcrc16(user->portnum, user->accountPageNumber);
      for (i = 0; i < user->accountFile[0]+3; i++)
         lastcrc16 = docrc16(user->portnum,user->accountFile[i]);
      if(lastcrc16==0xB001)
         validB = TRUE;

      if(validA && validB)
      {
         // Both A & B are valid!  And we know that we can only
         // get here if the pointer or the header was not valid.
         // That means that B was the last updated one but the
         // header got hosed and the debit was not finished...
         // which means A is the last known good value, let's go with A.
         user->accountFile[0] = RECORD_A_LENGTH;
         OWASSERT( WriteDataPageSHA33(copr, user),
                   OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );
         return TRUE;
      }
      else if(validA)
      {
         // B is invalid, A is valid.  Means A is the last updated one,
         // but B is the last known good value.  The header was not updated
         // to point to A before debit was aborted.  Let's go with B
         user->accountFile[0] = RECORD_B_LENGTH;
         // must fix B's CRC
         setcrc16(user->portnum, user->accountPageNumber);
         for (i = 0; i < RECORD_B_LENGTH+1; i++)
            lastcrc16 = docrc16(user->portnum,user->accountFile[i]);
         lastcrc16 = ~lastcrc16;
         ((DebitFile33*)user->accountFile)->crc16_B[0] = (uchar)lastcrc16;
         ((DebitFile33*)user->accountFile)->crc16_B[1] = (uchar)(lastcrc16>>8);
         OWASSERT( WriteDataPageSHA33(copr, user),
                   OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );
         return TRUE;
      }
      else if(validB)
      {
         // A is invalid, B is valid. Should never ever happen.  Something
         // got completely hosed.  What should happen here?
	     user->accountFile[0] = origPtr;
         OWERROR(OWERROR_BAD_SERVICE_DATA);
         return FALSE;
      }

      // neither record contains a valid CRC.  What should happen here?
      user->accountFile[0] = origPtr;
      OWERROR(OWERROR_CRC_FAILED);
      return FALSE;
   }
   else
   {
      // verify CRC16 is correct
      OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );
   }

   memset(scratchpad, 0x00, 32);

   if(user->writeCycleCounter>0)
      IntToBytes(&scratchpad[8], 4, user->writeCycleCounter);
   else
      // user doesn't have write cycle counter (DS1961S)
      memset(&scratchpad[8], 0x0FF, 4);
   // the pagenumber
   scratchpad[12] = (uchar)user->accountPageNumber;
   // and 7 bytes of the address of current device
   memcpy(&scratchpad[13], user->devAN, 7);
   // the coprocessor's signing challenge
   memcpy(&scratchpad[20], copr->signChlg, 3);

   // make a copy of the account file
   memcpy((uchar*)&acctFile, user->accountFile, 32);

   // clear out the old signature and CRC
   memcpy(acctFile.signature, copr->initSignature, 20);
   memset(acctFile.crc16, 0x00, 2);

   OWASSERT( CreateDataSignatureVM(copr, sign_secret,
                                   (uchar*)&acctFile, scratchpad,
                                   ((DebitFile*)user->accountFile)->signature,
                                   FALSE),
             OWERROR_MATCH_SERVICE_SIGNATURE_FAILED, FALSE );
   return TRUE;
}

//-------------------------------------------------------------------------
// Performs debit of service data on a user token, re-signs the data, and
// writes it back to the user token.
// Pre-condition: must call verify user first.
//
// 'copr'          - Structure for holding coprocessor information.
// 'user'          - Structure for holding user token information.
// 'debitAmount'   - the amount of money to debit from balance in cents.
// 'verifySuccess' - Paranoid double-check of account write by re-calling
//                   VerifyUser(copr,user)
//
// Return: If TRUE, data was updated.
//         If FALSE, an error occurred or couldn't verify success.
//
SMALLINT ExecuteTransaction(SHACopr* copr, SHAUser* user,
                            int debitAmount, SMALLINT verifySuccess )
{
   SMALLINT success, dataOK = TRUE;
   uchar oldAcctData[32], newAcctData[32];
   int cnt = MAX_RETRY_CNT;
   int balance, oldBalance ;

   memcpy(oldAcctData, user->accountFile, 32);

   if(user->devAN[0]==0x18)
   {
      DebitFile* accountFile = (DebitFile*)user->accountFile;

      oldBalance = BytesToInt(accountFile->balanceBytes, 3);
      if(oldBalance<=0)
      {
         OWERROR(OWERROR_BAD_SERVICE_DATA);
         return FALSE;
      }
      balance = oldBalance - debitAmount;
      IntToBytes(accountFile->balanceBytes, 3, balance);
   }
   else if((user->devAN[0]&0x7F)==0x33)
   {
      DebitFile33* accountFile = (DebitFile33*)user->accountFile;

      if(accountFile->fileLength==RECORD_A_LENGTH)
      {
         oldBalance = BytesToInt(accountFile->balanceBytes_A, 3);
         if(oldBalance<=0)
         {
            OWERROR(OWERROR_BAD_SERVICE_DATA);
            return FALSE;
         }
         balance = oldBalance - debitAmount;
         IntToBytes(accountFile->balanceBytes_B, 3, balance);
      }
      else if(accountFile->fileLength==RECORD_B_LENGTH)
      {
         oldBalance = BytesToInt(accountFile->balanceBytes_B, 3);
         if(oldBalance<=0)
         {
            OWERROR(OWERROR_BAD_SERVICE_DATA);
            return FALSE;
         }
         balance = oldBalance - debitAmount;
         IntToBytes(accountFile->balanceBytes_A, 3, balance);
      }
      else
      {
         OWERROR(OWERROR_BAD_SERVICE_DATA);
         return FALSE;
      }
   }

   success = UpdateServiceData(copr, user);

   //if write didn't succeeded or if we need to perform
   //a verification step anyways, let's double-check what
   //the user has on the button.
   if(verifySuccess || !success)
   {
      dataOK = FALSE;
      //save what the account data is supposed to look like
      memcpy(newAcctData, user->accountFile, 32);
      do
      {
         //calling verify user re-issues a challenge-response
         //and reloads the cached account data in the user object.
         //Does not re-bind the user's unique secret
         if(VerifyUser(copr,user,FALSE))
         {
            //compare the user's account data against the working
            //copy and the backup copy.
            if( memcmp(user->accountFile, newAcctData, 32) == 0 )
            {
               //looks like it worked
               dataOK = TRUE;
            }
            else if( memcmp(user->accountFile, oldAcctData, 32) == 0 )
            {
               //if it matches the backup copy, we didn't write anything
               //and the data is still okay, but we didn't do a debit
               dataOK = TRUE;
               success = FALSE;
               OWERROR(OWERROR_SERVICE_DATA_NOT_UPDATED);
            }
            else
            {
               // retry the write
               success = UpdateServiceData(copr, user);
               //save what the account data is supposed to look like
               memcpy(newAcctData, user->accountFile, 32);
            }
         }
      }
      while(dataOK==FALSE && cnt-->0);
   }

   if(!dataOK)
   {
      //couldn't fix the data after 255 retries
      OWERROR(OWERROR_CATASTROPHIC_SERVICE_FAILURE);
      success = FALSE;
   }
   return success;
}

//-------------------------------------------------------------------------
// Verifies the authentication response of a user token without a
// hardware coprocessor.
//
// 'copr'      - Structure for holding coprocessor information.
// 'user'      - Structure for holding user token information.
// 'chlg'      - 3-byte buffer of challenge data.
// 'doBind'    - if true, the user's unique secret is recreated on the
//               coprocessor.  If this function is called multiple times,
//               it is acceptable to skip the bind for all calls after
//               the first on the same user token.
//
// Return: If TRUE, the user's authentication response matched exactly the
//            signature generated by the coprocessor.
//         If FALSE, an error occurred or the signature did not match.
//
SMALLINT VerifyAuthResponseVM(SHACopr* copr, SHAUser* user,
                              uchar* chlg, SMALLINT doBind)
{
   int wcc = user->writeCycleCounter;
   uchar scratchpad[32];
   uchar fullBindCode[15];
   uchar temp_buf[32];
   // M-X control byte
   uchar MXP = 0x00;

   memset(scratchpad, 0x00, 32);
   memset(fullBindCode, 0x0FF, 15);

   // format the bind code properly
   if(user->devAN[0]==0x18)
   {
      // Format for DS1963S
      memcpy(fullBindCode, copr->bindCode, 4);
      memcpy(&fullBindCode[12], &(copr->bindCode[4]), 3);
      // copy wcc LSB first
      IntToBytes(&scratchpad[8], 4, wcc);
   }
   else if((user->devAN[0]&0x7F)==0x33)
   {
      // Leave bindCode FF for DS1961S
      // Use AuthenticateHost command
      MXP = 0x40;
      // the user doesn't have a write cycle counter
      memset(&scratchpad[8], 0x0FF, 4);
   }
   else
   {
      OWERROR(OWERROR_WRONG_TYPE);
      return FALSE;
   }

   // the pagenumber
   fullBindCode[4] = (uchar)user->accountPageNumber;
   // and 7 bytes of the address of current device
   memcpy(&fullBindCode[5], user->devAN, 7);

   // get the user address and page num from fullBindCode
   memcpy(&scratchpad[12], &fullBindCode[4], 8);
   // set the same challenge bytes
   memcpy(&scratchpad[20], chlg, 3);

   // Fix the M-X control bits
   scratchpad[12] = (uchar)((scratchpad[12]&0x3F)|(MXP&0xC0));

   memset(temp_buf, 0x00, 32);
   memcpy(&temp_buf[8],fullBindCode,15);

   // install user's unique secret on the wspc secret
   // Just like BindSecretToiButton for standard coprocessor
   if(doBind)
   {
      OWASSERT( CreateDataSignatureVM(copr, auth_secret,
                                      copr->bindData,
                                      temp_buf,
                                      temp_buf,
                                      TRUE),
                OWERROR_BIND_SECRET_FAILED, FALSE );
      memcpy(wspc_secret, temp_buf, 8);
   }

   // recreate the signature and verify
   OWASSERT( CreateDataSignatureVM(copr, wspc_secret,
                                   user->accountFile,
                                   scratchpad,
                                   user->responseMAC,
                                   FALSE),
             OWERROR_VERIFY_SIG_FAILED, FALSE );

   return TRUE;
}

//-------------------------------------------------------------------------
// Creates a data signature for the given data buffer without a hardware
// coprocessor.
//
// 'copr'          - Structure for holding coprocessor information.
// 'data'          - data written to the data page to sign
// 'scratchpad'    - data written to the scratchpad to sign
// 'signature'     - data buffer which is either holding the signature that
//                   must match exactly what is generated on the coprocessor
//                   -or- will hold the resulting signature created by the
//                   coprocessor.
// 'readSignature' - implies whether or not the signature buffer
//                   receives the contents of the scratchpad or is used to
//                   match the contents of the scratchpad.  If true,
//                   scratchpad contents are read into the signature buffer.
//
// Return: If TRUE, the user's authentication response matched exactly the
//            signature generated by the coprocessor or the signature was
//            successfully copied into the return buffer.
//         If FALSE, an error occurred or the signature did not match.
//
SMALLINT CreateDataSignatureVM(SHACopr* copr, uchar* secret, uchar* data,
                               uchar* scratchpad, uchar* signature,
                               SMALLINT readSignature)
{
   SMALLINT i,j,offset;
   long hash[5], temp;
   uchar digestBuff[64];

   //Set up the 64 byte buffer for computing the digest.
   memcpy(digestBuff,secret,4);
   memcpy(&digestBuff[4],data,32);
   memcpy(&digestBuff[36],&scratchpad[8],12);
   memcpy(&digestBuff[48],&secret[4],4);
   memcpy(&digestBuff[52],&scratchpad[20],3);

   //digest buffer padding
   digestBuff[55] = (uchar)0x80;
   memset(&digestBuff[56], 0x00, 6);
   digestBuff[62] = (uchar)0x01;
   digestBuff[63] = (uchar)0xB8;

   ComputeSHAVM(digestBuff, hash);

   //iButtons use LSB first, so we have to turn
   //the result around a little bit.  Instead of
   //result A-B-C-D-E, our result is E-D-C-B-A,
   //where each letter represents four bytes of
   //the result.
   for(j=4; j>=0; j--)
   {
      temp = hash[j];
      offset = (4-j)*4;
      for(i=0; i<4; i++)
      {
         if(readSignature)
         {
            //reading the signature into a buffer
            signature[i+offset] = (uchar)temp;
         }
         else
         {
            //trying to match the signature;
            OWASSERT( signature[i+offset] == (uchar)temp,
                      OWERROR_MATCH_SCRATCHPAD_FAILED, FALSE );
         }
         temp >>= 8;
      }
   }

   //PrintHexLabeled("signature", signature, 20);
   return TRUE;
}

//----------------------------------------------------------------------
// Installs new system secret for VM.  input_secret must be
// divisible by 47.  Then, each block of 47 is split up with 32 bytes
// written to a data page and 15 bytes are written to the scratchpad.
// Then, Compute first secret is called (or compute next if the secret
// is divisible by 47 more than once).
//
// 'copr'          - Structure for holding coprocessor information.
// 'input_secret'  - the input secret buffer used.
// 'secret_length' - the length of the input secret buffer, divisibly
//                   by 47.
// 'dst_secret'    - the buffer where resulting secret is copied.
//
// Return: TRUE - Install successfull
//         FALSE - error occurred during secret installation.
//
SMALLINT InstallSystemSecretVM(SHACopr* copr,
                               uchar* secret, int secret_length,
                               uchar* dst_secret)
{
   int offset = 0, bytes_left = 0;
   uchar data[32],scratchpad[32];

   //PrintHexLabeled("Installing secret",secret,secret_length);
   for(offset=0; offset<secret_length; offset+=47)
   {
      // clear the buffer
      memset(data, 0x0FF, 32);
      memset(scratchpad, 0x0FF, 32);

      // Determine the amount of bytes remaining to be installed.
      bytes_left = secret_length - offset;

      // copy secret data into page buffer and scratchpad buffer
      memcpy(data, &secret[offset], (bytes_left<32?bytes_left:32));
      if(bytes_left>32)
      {
         memcpy(&scratchpad[8], &secret[offset+32],
                (bytes_left<47?bytes_left-32:15));
      }

      // fix the M-X control bits
      scratchpad[12] = scratchpad[12] & 0x3F;

      //PrintHexLabeled("  using secret",dst_secret,8);
      //PrintHexLabeled("  using data",data,32);
      //PrintHexLabeled("  using spad",scratchpad,32);

      // install user's unique secret on the wspc secret
      // Just like BindSecretToiButton for standard coprocessor
      OWASSERT( CreateDataSignatureVM(copr, dst_secret,
                                      data,
                                      scratchpad,
                                      scratchpad,
                                      TRUE),
                OWERROR_BIND_SECRET_FAILED, FALSE );
      memcpy(dst_secret, scratchpad, 8);
   }

   return TRUE;
}

//----------------------------------------------------------------------
// Installs new system signing secret for VM.  input_secret must be
// divisible by 47.  Then, each block of 47 is split up with 32 bytes
// written to a data page and 15 bytes are written to the scratchpad.
// Then, Compute first secret is called (or compute next if the secret
// is divisible by 47 more than once).
//
// 'copr'          - Structure for holding coprocessor information.
// 'secretnum'     - destination secret for computation results
// 'input_secret'  - the input secret buffer used.
// 'secret_length' - the length of the input secret buffer, divisibly
//                   by 47.
//
// Return: TRUE - Install successfull
//         FALSE - error occurred during secret installation.
//
SMALLINT InstallSignSecretVM(SHACopr* copr,
                             uchar* secret, int secret_length)
{
   int i;
   memset(sign_secret, 0x00, 8);
   i = InstallSystemSecretVM(copr, secret, secret_length, sign_secret);
   return i;
}

//----------------------------------------------------------------------
// Installs new system signing secret for VM.  input_secret must be
// divisible by 47.  Then, each block of 47 is split up with 32 bytes
// written to a data page and 15 bytes are written to the scratchpad.
// Then, Compute first secret is called (or compute next if the secret
// is divisible by 47 more than once).
// Note: as a side-effect, the random number generator is seeded in this
// function as well.
//
// 'copr'          - Structure for holding coprocessor information.
// 'secretnum'     - destination secret for computation results
// 'input_secret'  - the input secret buffer used.
// 'secret_length' - the length of the input secret buffer, divisibly
//                   by 47.
//
// Return: TRUE - Install successfull
//         FALSE - error occurred during secret installation.
//
SMALLINT InstallAuthSecretVM(SHACopr* copr,
                             uchar* secret, int secret_length)
{
   int i;
   memset(auth_secret, 0x00, 8);
   i = InstallSystemSecretVM(copr, secret, secret_length, auth_secret);

   // at bare minimum, InstallAuthSecretVM must be called before the
   // VM coprocessor can be used.  We can take advantage of this fact
   // and use this opportunity to seed the random number generator.
   srand( ((unsigned int)time(NULL)) ^
      ((secret[3]) | (secret[5]<<8) | (secret[7]<<16) | (secret[11]<<24)) );
   //is that random?  although deterministic, it's got to be at the
   //very least hard to predict.  it depends on user input which should
   //be kept top secret.

   return i;
}

//----------------------------------------------------------------------
// Loads the Coprocessor VM's config info from either a config file or
// an emulated coprocessor.
//
// 'copr'          - Structure for holding coprocessor information.
// 'fe'            - FileEntry to look for on emulated coprocessor
//
// Return: TRUE - GetCoprocessor successfull
//         FALSE - error occurred
//
SMALLINT GetCoprVM(SHACopr* copr, FileEntry* fe)
{
   FILE* fp;
   uchar *signSecret,*authSecret;
   int signlen, authlen, len, index;
   long lvalue = 1;
   char test[2] = { 'y', 0 };
   uchar buffer[255], i=0;


   printf("How would you like to initialize the VM?");
   EnterNum("\n     1) Load VM info from file\n     "
            "2) Emulate another coprocessor\n", 1, &lvalue, 1, 2);
   if(lvalue==1)
   {
      // open up a file, maybe "shaVM.cnf"
      fp = fopen(SHACoprFilename, "rb");
      if(fp==NULL)
      {
         printf("file not found: %s\n", SHACoprFilename);
         return FALSE;
      }
      len = fread(buffer, 1, 255, fp);
      if(len<=0)
      {
         printf("Could not read coprocessor config file\n");
         return FALSE;
      }

      index = GetCoprFromRawData(copr, buffer, len);

      // get the signing secret
      signlen = buffer[index];
      signSecret = malloc(signlen);
      memcpy(signSecret, &buffer[index+1], signlen);

      // get the authentication secret
      index += signlen+1;
      authlen = buffer[index];
      authSecret = malloc(authlen);
      memcpy(authSecret, &buffer[index+1], authlen);
   }
   else
   {
      if(!FindCoprSHA(copr, fe))
      {
         printf("no coprocessor found!\n");
         return FALSE;
      }

      signlen = GetSecretVM("System Signing Secret", &signSecret);

      authlen = GetSecretVM("System Authentication Secret", &authSecret);

      EnterString("Reformat the secret for DS1961S compatibility", test, 1, 1);
      if(test[0] == 'y')
      {
         ReformatSecretFor1961S(authSecret, authlen);
         PrintHex(authSecret, authlen);
         printf("\n");
         copr->ds1961Scompatible = 0x55;
      }
   }

   InstallAuthSecretVM(copr, authSecret, authlen);
   InstallSignSecretVM(copr, signSecret, signlen);

   free(authSecret);
   free(signSecret);

   return TRUE;
}


//----------------------------------------------------------------------
// Used for querying the user for secret information
//
// 'name'   - type of secret (authentication or signing)
// 'secret' - resulting secret data
//
// Returns: The length of the secret
//
static int GetSecretVM(char* name, uchar** secret)
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

   return (int) length;
}


//----------------------------------------------------------------------
// Writes the contents of the user's account file to the page specified
// by the user's account page number for DS1961.
//
// 'copr'       - Structure for holding coprocessor information.
// 'user'       - Structure for holding user token information.
//
// Return: TRUE - Write successfull
//         FALSE - error occurred during write.
//
SMALLINT WriteDataPageSHA33(SHACopr* copr, SHAUser* user)
{
   //DS1961S - a bit tougher
   //assumes wspc_secret already contains device's unique secret.
   uchar pageContents[32], data[32], scratchpad[32];
   uchar MAC[20];
   int addr = user->accountPageNumber << 5;
   int i;

   OWASSERT( ReadMemoryPageSHA33(user->portnum,
                                 user->accountPageNumber,
                                 pageContents, FALSE),
                OWERROR_READ_MEMORY_PAGE_FAILED, FALSE );

   for(i=24; i>=0; i-=8)
   {
      if(memcmp(&pageContents[i],&user->accountFile[i],8) != 0)
      {
         memcpy(data, pageContents, 28);
         memcpy(&data[28], &user->accountFile[i], 4);
         memcpy(&scratchpad[8], &user->accountFile[i+4], 4);
         scratchpad[12] = (uchar)(user->accountPageNumber&0x3F);
         memcpy(&scratchpad[13], user->devAN, 7);
         memset(&scratchpad[20], 0xFF, 3);

         OWASSERT( CreateDataSignatureVM(copr, wspc_secret, data,
                                         scratchpad, MAC, TRUE),
                   OWERROR_SIGN_SERVICE_DATA_FAILED, FALSE);

         OWASSERT( WriteScratchpadSHA33(user->portnum, addr+i,
                                        &user->accountFile[i],
                                        TRUE),
                    OWERROR_WRITE_SCRATCHPAD_FAILED, FALSE );

         OWASSERT( CopyScratchpadSHA33(user->portnum, addr+i,
                                       MAC, TRUE),
                    OWERROR_COPY_SCRATCHPAD_FAILED, FALSE );
         memcpy(&pageContents[i], &user->accountFile[i], 8);
      }
   }

   return TRUE;
}


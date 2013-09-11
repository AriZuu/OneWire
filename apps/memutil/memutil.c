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

#define DEBUG 1
#include <stdio.h>
#include <ctype.h>
#include "ownet.h"
#include "rawmem.h"
#include "mbshaee.h"
#include "mbeprom.h"
#include "mbee77.h"
#include "pw77.h"


// Defines for the menu lists
#define MAIN_MENU          0
#define BANK_MENU          1
#define ENTRY_MENU         2
// Defines for the switch statements for Main Menu
#define MAIN_SELECT_DEVICE 0
#define MAIN_QUIT          1
// Defines for the switch statements for Memory Bank
#define BANK_INFO          0
#define BANK_READ_BLOCK    1
#define BANK_READ_PAGE     2
#define BANK_READ_UDP      3
#define BANK_WRITE_BLOCK   4
#define BANK_WRITE_UDP     5
#define BANK_NEW_BANK      6
#define BANK_MAIN_MENU     7
#define BANK_BM_READ_PASS  8
#define BANK_BM_RW_PASS    9
#define BANK_READ_PASS     10
#define BANK_RW_PASS       11
#define BANK_ENABLE_PASS   12
#define BANK_DISABLE_PASS  13
// Defines for the switch statements for data entry
#define MODE_TEXT          0
#define MODE_HEX           1
// Number of devices to list
#define MAXDEVICES         10
// Max number for data entry
#define MAX_LEN            256

// Local functions
int menuSelect(int menu, uchar *SNum);
SMALLINT selectDevice(int numDevices, uchar AllDevices[][8]);
void printDeviceInfo (int portnum, uchar SNum[8]);
SMALLINT selectBank(SMALLINT bank, uchar *SNum);
void displayBankInformation (SMALLINT bank, int portnum, uchar SNum[8]);
int getNumber (int min, int max);
SMALLINT dumpBankBlock (SMALLINT bank, int portnum, uchar SNum[8],
                        int addr, int len);
SMALLINT dumpBankPage (SMALLINT bank, int portnum, uchar SNum[8], int pg);
SMALLINT dumpBankPagePacket(SMALLINT bank, int portnum, uchar SNum[8], int pg);
SMALLINT bankWriteBlock (SMALLINT bank, int portnum, uchar SNum[8], int addr,
                         uchar *data, int length);
SMALLINT bankWritePacket(SMALLINT bank, int portnum, uchar SNum[8],
                         int pg, uchar *data, int length);
SMALLINT optionSHAEE(SMALLINT bank, int portnum, uchar *SNum);


// Global necessary for screenio
int VERBOSE;
uchar AllSN[MAXDEVICES][8];


int main(int argc, char **argv)
{
   int len, addr, page, answer, i;
   int done      = FALSE;
   SMALLINT  bank = 1;
   uchar     data[552];
   int portnum = 0;
   uchar AllSN[MAXDEVICES][8];
   int NumDevices;
   int owd;
   char msg[132];


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
      // loop to do menu
      do
      {

         // Main menu
         switch (menuSelect(MAIN_MENU,&AllSN[0][0]))
         {

            case MAIN_SELECT_DEVICE :

               // find all parts
               // loop to find all of the devices up to MAXDEVICES
               NumDevices = 0;
               do
               {
                  // perform the search
                  if (!owNext(portnum,TRUE, FALSE))
                     break;

                  owSerialNum(portnum,AllSN[NumDevices], TRUE);
                  NumDevices++;
               }
               while (NumDevices < (MAXDEVICES - 1));

               /* for test devices without a serial number
               if(NumDevices == 0)
               {
                  for(i=0;i<8;i++)
                     AllSN[0][i] = 0x00;
                  NumDevices++;
               }*/

               // select a device
               owd = selectDevice(NumDevices,&AllSN[0]);

               // display device info
               printDeviceInfo(portnum,&AllSN[owd][0]);

               // select a bank
               bank = selectBank(bank, &AllSN[owd][0]);

               if((AllSN[owd][0] == 0x33) || (AllSN[owd][0] == 0xB3))
                  bank = optionSHAEE(bank,portnum,&AllSN[owd][0]);

               // display bank information
               displayBankInformation(bank,portnum,&AllSN[owd][0]);

               // loop on bank menu
               do
               {
                  switch (menuSelect(BANK_MENU,&AllSN[owd][0]))
                  {

                     case BANK_INFO :
                        // display bank information
                        displayBankInformation(bank,portnum,&AllSN[owd][0]);
                        break;

                     case BANK_READ_BLOCK :
                        // read a block
                        printf("Enter the address to start reading: ");
                        addr = getNumber(0, (owGetSize(bank,&AllSN[owd][0])-1));
                        printf("\n");

                        printf("Enter the length of data to read: ");
                        len = getNumber(0, owGetSize(bank, &AllSN[owd][0]));
                        printf("\n");

                        if(!dumpBankBlock(bank,portnum,&AllSN[owd][0],addr,len))
                           OWERROR_DUMP(stderr);
                        break;

                     case BANK_READ_PAGE :
                        printf("Enter the page number to read:  ");

                        page = getNumber(0, (owGetNumberPages(bank,&AllSN[owd][0])-1));

                        printf("\n");

                        if(!dumpBankPage(bank,portnum,&AllSN[owd][0],page))
                           OWERROR_DUMP(stderr);

                        break;

                     case BANK_READ_UDP :
                        printf("Enter the page number to read: ");

                        page = getNumber(0, (owGetNumberPages(bank,&AllSN[owd][0])-1));

                        printf("\n");

                        if(!dumpBankPagePacket(bank,portnum,&AllSN[owd][0],page))
                           OWERROR_DUMP(stderr);
                        break;

                     case BANK_WRITE_BLOCK :
                        // write a block
                        printf("Enter the address to start writing: ");

                        addr = getNumber(0, (owGetSize(bank,&AllSN[owd][0])-1));

            				if(menuSelect(ENTRY_MENU,&AllSN[owd][0]) == MODE_TEXT)
            					len = getData(data,MAX_LEN,MODE_TEXT);
            				else
            					len = getData(data,MAX_LEN,MODE_HEX);

                        if(!bankWriteBlock(bank,portnum,&AllSN[owd][0],addr,data,len))
                        {
                           OWERROR_DUMP(stderr);
                           break;
                        }

                        if(owCanRedirectPage(bank,&AllSN[owd][0]))
                        {
                           printf("Enter if you want to redirect page (0 no, 1 yes): ");

                           answer = getNumber(0,1);

                           if(answer)
                           {
                              printf("What page would you like to redirect:");

                              page = getNumber(0,255);

                              printf("Where would you like to redirect:");

                              addr = getNumber(0,255);

                              if(!redirectPage(bank,portnum,&AllSN[owd][0],page,addr))
                              {
                                 OWERROR_DUMP(stderr);
                                 break;
                              }
                           }
                        }

                        if(owCanLockPage(bank,&AllSN[owd][0]))
                        {
                           printf("Enter if you want to lock page (0 no, 1 yes):");

                           answer = getNumber(0,1);

                           if(answer)
                           {
                              printf("What page would you like to lock?");

                              page = getNumber(0,255);

                              if(!lockPage(bank,portnum,&AllSN[owd][0],page))
                              {
                                 OWERROR_DUMP(stderr);
                                 break;
                              }
                           }
                        }

                        if(owCanLockRedirectPage(bank,&AllSN[owd][0]))
                        {
                           printf("Enter if you want to lock redirected page (0 no, 1 yes):");

                           answer = getNumber(0,1);

                           if(answer)
                           {
                              printf("Which redirected page do you want to lock:");

                              page = getNumber(0,255);

                              if(!lockRedirectPage(bank,portnum,&AllSN[owd][0],page))
                              {
                                 OWERROR_DUMP(stderr);
                                 break;
                              }
                           }
                        }
                        break;

                     case BANK_WRITE_UDP :
                        printf("Enter the page number to write a UDP to: ");

                        page = getNumber(0, (owGetNumberPages(bank,&AllSN[owd][0])-1));

            				if(menuSelect(ENTRY_MENU,&AllSN[owd][0]) == MODE_TEXT)
            					len = getData(data,MAX_LEN,MODE_TEXT);
            				else
            					len = getData(data,MAX_LEN,MODE_HEX);

                        if(!bankWritePacket(bank,portnum,&AllSN[owd][0],page,data,len))
                           OWERROR_DUMP(stderr);
                        break;

                     case BANK_BM_READ_PASS:
                        printf("Enter the 8 byte read only password if less 0x00 will be filled in.");

            				if(menuSelect(ENTRY_MENU,&AllSN[owd][0]) == MODE_TEXT)
            					len = getData(data,8,MODE_TEXT);
            				else
            					len = getData(data,8,MODE_HEX);

                        if(len != 8)
                        {
                           for(i=len;i<8;i++)
                              data[i] = 0x00;
                        }

                        if(!owSetBMReadOnlyPassword(portnum,&AllSN[owd][0],data))
                           OWERROR_DUMP(stderr);

                        break;

                     case BANK_BM_RW_PASS:
                        printf("Enter the 8 byte read/write password if less 0x00 will be filled in.");

            				if(menuSelect(ENTRY_MENU,&AllSN[owd][0]) == MODE_TEXT)
            					len = getData(data,8,MODE_TEXT);
            				else
            					len = getData(data,8,MODE_HEX);

                        if(len != 8)
                        {
                           for(i=len;i<8;i++)
                              data[i] = 0x00;
                        }

                        if(!owSetBMReadWritePassword(portnum,&AllSN[owd][0],data))
                           OWERROR_DUMP(stderr);

                        break;

                     case BANK_READ_PASS:
                        printf("Enter the 8 byte read only password if less 0x00 will be filled in.");

            				if(menuSelect(ENTRY_MENU,&AllSN[owd][0]) == MODE_TEXT)
            					len = getData(data,8,MODE_TEXT);
            				else
            					len = getData(data,8,MODE_HEX);

                        if(len != 8)
                        {
                           for(i=len;i<8;i++)
                              data[i] = 0x00;
                        }

                        if(!owSetReadOnlyPassword(portnum,&AllSN[owd][0],data))
                           OWERROR_DUMP(stderr);

                        break;

                     case BANK_RW_PASS:
                        printf("Enter the 8 byte read/write password if less 0x00 will be filled in.");

            				if(menuSelect(ENTRY_MENU,&AllSN[owd][0]) == MODE_TEXT)
            					len = getData(data,8,MODE_TEXT);
            				else
            					len = getData(data,8,MODE_HEX);

                        if(len != 8)
                        {
                           for(i=len;i<8;i++)
                              data[i] = 0x00;
                        }

                        if(!owSetReadWritePassword(portnum,&AllSN[owd][0],data))
                           OWERROR_DUMP(stderr);

                        break;

                     case BANK_ENABLE_PASS:
                        if(!owSetPasswordMode(portnum,&AllSN[owd][0],ENABLE_PSW))
                           OWERROR_DUMP(stderr);
                        break;

                     case BANK_DISABLE_PASS:
                        if(!owSetPasswordMode(portnum,&AllSN[owd][0],DISABLE_PSW))
                           OWERROR_DUMP(stderr);
                        break;

                     case BANK_NEW_BANK :
                        // select a bank
                        bank = selectBank(bank,&AllSN[owd][0]);

                        if((AllSN[owd][0] == 0x33) || (AllSN[owd][0] == 0xB3))
                           bank = optionSHAEE(bank,portnum,&AllSN[owd][0]);

                        // display bank information
                        displayBankInformation(bank,portnum,&AllSN[owd][0]);
                        break;

                     case BANK_MAIN_MENU :
                        done = TRUE;
                        break;
                  }
               }
               while (!done);

               done = FALSE;
               break;

            case MAIN_QUIT :
               done = TRUE;
               break;

         }  // Main menu switch
      }
      while (!done);  // loop to do menu

      owRelease(portnum);
   }  // else for owAcquire

   return 1;
}


//--------
//-------- Read methods
//--------

/**
 * Read a block from a memory bank and print in hex
 *
 * bank    MemoryBank to read a block from
 * portnum port number
 * SNum    the serial number for the device
 * addr    address to start reading from
 * len     length of data to read
 *
 * @return 'true' if the dumping of the memory bank
 *          worked.
 */
SMALLINT dumpBankBlock (SMALLINT bank, int portnum, uchar SNum[8],
                        int addr, int len)
{
   uchar read_buf[8000];  // make larger if the data being read is larger
   int i;

   if(!owRead(bank,portnum,SNum,addr,FALSE,&read_buf[0],len))
      return FALSE;

   for(i=0;i<len;i++)
      printf("%02X ",read_buf[i]);
   printf("\n");

   return TRUE;

}

/**
 * Read a page from a memory bank and print in hex
 *
 * bank    MemoryBank to read a page from
 * portnum port number
 * SNum    The serial number of the device to read.
 * pg      page to read
 *
 * @return 'true' if the bank page was dumped.
 */
SMALLINT dumpBankPage (SMALLINT bank, int portnum, uchar SNum[8], int pg)
{
   uchar read_buf[64];
   uchar extra_buf[32];
   int i;
   int result;

   // read a page (use the most verbose and secure method)
   if (owHasPageAutoCRC(bank,SNum))
   {
      if(owHasExtraInfo(bank,SNum))
         result = owReadPageExtraCRC(bank,portnum,&SNum[0],
                     pg,&read_buf[0],&extra_buf[0]);
      else
         result = owReadPageCRC(bank,portnum,&SNum[0],pg,&read_buf[0]);
   }
   else
   {
      if(owHasExtraInfo(bank,SNum))
         result = owReadPageExtra(bank,portnum,&SNum[0],
                     pg,FALSE,&read_buf[0],&extra_buf[0]);
      else
         result = owReadPage(bank,portnum,&SNum[0],pg,FALSE,&read_buf[0]);
   }

   if(!result)
      return FALSE;

   printf("Page %d:  ",pg);

   for(i=0;i<owGetPageLength(bank,SNum);i++)
      printf("%02X ",read_buf[i]);
   printf("\n");

   if(owHasExtraInfo(bank,SNum))
   {
      printf("Extra: ");
      for(i=0;i<owGetExtraInfoLength(bank,SNum);i++)
         printf("%02X ",extra_buf[i]);
      printf("\n");
   }

   return TRUE;
}

/**
 * Read a page packet from a memory bank and print in hex
 *
 * bank  PagedMemoryBank to read a page from
 * portnum port number
 * SNum    The serial number of the device to read.
 * pg  page to read
 *
 * @return 'true' if the bank page packet was dumped.
 */
SMALLINT dumpBankPagePacket(SMALLINT bank, int portnum, uchar SNum[8], int pg)
{
   uchar  read_buf[32];
   uchar  extra_buf[32];
   int    read_rslt;
   int    i;

   // read a page packet (use the most verbose method)
   if(owHasExtraInfo(bank,SNum))
   {
      if(!owReadPagePacketExtra(bank,portnum,&SNum[0],pg,FALSE,
                                &read_buf[0],&read_rslt,&extra_buf[0]))
         return FALSE;
   }
   else
      if(!owReadPagePacket(bank,portnum,&SNum[0],pg,FALSE,&read_buf[0],&read_rslt))
         return FALSE;

   printf("Packet %d, len %d\n",pg,read_rslt);
   for(i=0;i<read_rslt;i++)
      printf("%02X ",read_buf[i]);
   printf("\n");

   if(owHasExtraInfo(bank,SNum))
   {
      printf("Extra: ");
      for(i=0;i<owGetExtraInfoLength(bank,SNum);i++)
         printf("%02X ",extra_buf[i]);
      printf("\n");
   }

   return TRUE;
}

//--------
//-------- Write methods
//--------

/**
 * Write a block of data with the provided MemoryBank.
 *
 * bank    MemoryBank to write block to
 * portnum port number for the device.
 * SNum    serial number for the device.
 * addr    address to start the write.
 * data    data to write in a byte array
 * length  length of the data to be written
 *
 * @return 'true' if the write worked.
 */
SMALLINT bankWriteBlock (SMALLINT bank, int portnum, uchar SNum[8], int addr,
                         uchar *data, int length)
{
   if(!owWrite(bank,portnum,SNum,addr,data,length))
      return FALSE;

   printf("\nWrote block length %d at addr %d\n",length,addr);

   return TRUE;
}

/**
 * Write a UDP packet to the specified page in the
 * provided PagedMemoryBank.
 *
 * bank    PagedMemoryBank to write packet to
 * portnum port number for the device
 * SNum    serial number for the device
 * pg      page number to write packet to
 * data    data to write in a byte array
 * length  length of the data to be written
 *
 * @return 'true' if the packet was written.
 */
SMALLINT bankWritePacket(SMALLINT bank, int portnum, uchar SNum[8],
                         int pg, uchar *data, int length)
{
   if(!owWritePagePacket(bank,portnum,SNum,pg,data,length))
      return FALSE;

   printf("\n");
   printf("wrote packet length %d on page %d\n",length,pg);

   return TRUE;
}

//--------
//-------- Menu methods
//--------

/**
 * Create a menu from the provided OneWireContainer
 * Vector and allow the user to select a device.
 *
 * numDevices  the number of devices on the 1-Wire.
 * AllDevices  holds all the Serial Numbers on the
 *                      1-Wire.
 *
 * @return index of the choosen device.
 */
SMALLINT selectDevice(int numDevices, uchar AllDevices[][8])
{

   int i,j;

   printf("Device Selection\n");

   for (i=0; i<numDevices; i++)
   {
      printf("(%d) ",i);
      for(j=7;j>=0;j--)
         printf("%02X ",AllDevices[i][j]);
      printf("\n");
   }

   return getNumber(0,numDevices-1);
}

/**
 * Create a menu of memory banks from the provided OneWireContainer
 * allow the user to select one.
 *
 * bank  the memory bank to be used.
 * SNum  The serial number for the chosen device.
 *
 * @return MemoryBank memory bank selected
 */
SMALLINT selectBank(SMALLINT bank, uchar *SNum)
{
   int i;
   int numberBanks = 0;

   printf("Memory Bank Selection for ");
   for(i=7;i>=0;i--)
      printf("%02X ",SNum[i]);
   printf("\n");

   numberBanks = owGetNumberBanks(SNum[0]);

   // get the banks
   for (i=0;i<numberBanks;i++)
   {
      printf("(%d) %s\n",i,owGetBankDescription((SMALLINT)i,SNum));
   }

   if((SNum[0] == 0x33) || (SNum[0] == 0xB3))
   {
      printf("(%d) Load First Secret.\n",numberBanks++);
      printf("(%d) Compute Next Secret.\n",numberBanks++);
      printf("(%d) Change Bus Master Secret.\n",numberBanks++);
      printf("(%d) Lock Secret.\n",numberBanks++);
      printf("(%d) Input new challenge for Read Authenticate.\n",numberBanks++);
      printf("(%d) Write Protect page 0-3.\n",numberBanks++);
      printf("(%d) Set Page 1 to EPROM mode.\n",numberBanks++);
      printf("(%d) Write Protect Page 0.\n",numberBanks++);
      printf("(%d) Print Current Bus Master Secret.\n",numberBanks++);
   }

   return getNumber(0, numberBanks-1);
}

//--------
//-------- Menu Methods
//--------

/**
 * Display menu and ask for a selection.
 *
 * menu  the menu selections.
 *
 * @return numberic value entered from the console.
 */
int menuSelect(int menu, uchar *SNum)
{
   int length = 0;

   switch(menu)
   {
      case MAIN_MENU:
         printf("\n");
         printf("MainMenu 1-Wire Memory Demo\n");
         printf("(0) Select Device\n");
         printf("(1) Quit\n");
         length = 1;
         break;

      case BANK_MENU:
         printf("\n");
         printf("Bank Operation Menu\n");
         printf("(0)  Get Bank information\n");
         printf("(1)  Read Block\n");
         printf("(2)  Read Page\n");
         printf("(3)  Read Page UDP packet\n");
         printf("(4)  Write Block\n");
         printf("(5)  Write UDP packet\n");
         printf("(6)  GOTO MemoryBank Menu\n");
         printf("(7)  GOTO MainMenu\n");
         if(owNeedPassword(SNum))
         {
            printf("(8)  Set Bus Master read only password.\n");
            printf("(9)  Set Bus Master read/write password.\n");
            printf("(10) Set device read only password.\n");
            printf("(11) Set device read/write password.\n");
            printf("(12) Enable the password.\n");
            printf("(13) Disable the password.\n");
            length = 13;
         }
         else
         {
            length = 7;
         }
         break;

      case ENTRY_MENU:
         printf("\n");
         printf("Data Entry Mode\n");
         printf("(0) Text (single line)\n");
         printf("(1) Hex (XX XX XX XX ...)\n");
         length = 1;
         break;
   }

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
         done = TRUE;

   }
   while(!done);

   return value;

}

/**
 * Extra options for the DS2432
 *
 * bank    the memory bank to be used.
 * portnum port number for the device
 * SNum    The serial number for the chosen device.
 *
 * @return numeric value entered from the console.
 */
SMALLINT optionSHAEE(SMALLINT bank, int portnum, uchar *SNum)
{
   int done      = FALSE;
   int try_again = FALSE;
   int addr,len;
   uchar data[64];
   int i;

   do
   {
      switch(bank)
      {
         case 0: case 1: case 2: case 3:
            done = TRUE;
            break;

         case 4:   // Load First Secret
            printf("Enter the address for loading first secret: ");
            addr = getNumber(0, 152);

            printf("Enter the Load First Secret data: ");
            if(menuSelect(ENTRY_MENU,SNum) == MODE_TEXT)
            	len = getData(data,MAX_LEN,MODE_TEXT);
            else
            	len = getData(data,MAX_LEN,MODE_HEX);

            if(!loadFirstSecret(portnum,addr,SNum,&data[0],len))
               OWERROR_DUMP(stderr);
            break;

         case 5:   // Compute Next Secret
            printf("Enter the address for computing next secret: ");
            addr = getNumber(0,128);

            if(!computeNextSecret(portnum,addr,SNum,&data[0]))
               OWERROR_DUMP(stderr);

            printf("The new secret is: ");
            for(i=0;i<8;i++)
               printf("%02X ",data[i]);
            printf("\n");
            break;

         case 6:   // Change Bus Master Secret
            printf("Enter the new Bus Master Secret: ");

            try_again = FALSE;

            do
            {
            	if(menuSelect(ENTRY_MENU,SNum) == MODE_TEXT)
            		len = getData(data,MAX_LEN,MODE_TEXT);
            	else
            		len = getData(data,MAX_LEN,MODE_HEX);

               if(len != 8)
               {
                  printf("The secret is 8 bytes long try again: ");
                  try_again = TRUE;
               }
               else
                  try_again = FALSE;
            }
            while(try_again);

            setBusMasterSecret(&data[0]);
            break;

         case 7:   // Lock Secret
            data[0] = 0xAA;

            if(!bankWriteBlock(3,portnum,SNum,8,data,1))
            {
               OWERROR_DUMP(stderr);
               break;
            }
            break;

         case 8:   // Input new challenge for Read Authenticate
            printf("Enter the New Challenge: ");

            try_again = FALSE;

            do
            {
            	if(menuSelect(ENTRY_MENU,SNum) == MODE_TEXT)
            		len = getData(data,MAX_LEN,MODE_TEXT);
            	else
            		len = getData(data,MAX_LEN,MODE_HEX);

               if(len != 8)
               {
                  printf("The challenge is 8 bytes long try again: ");
                  try_again = TRUE;
               }
               else
                  try_again = FALSE;
            }
            while(try_again);

            setChallenge(&data[0]);
            break;

         case 9:   // Write Protect page 0-3
            data[0] = 0x00;

            if(!bankWriteBlock(3,portnum,SNum,9,data,1))
            {
               OWERROR_DUMP(stderr);
               break;
            }
            break;

         case 10:  // Set Page 1 to EEPROM mode
            data[0] = 0xAA;

            for(i=5;i<8;i++)
               data[i] = 0x00;

            if(!bankWriteBlock(3,portnum,SNum,12,data,1))
            {
            OWERROR_DUMP(stderr);
            break;
            }
            break;

         case 11:  // Write Protect Page 0
            data[0] = 0xAA;

            if(!bankWriteBlock(3,portnum,SNum,13,data,1))
            {
               OWERROR_DUMP(stderr);
               break;
            }
            break;

         case 12:  // Print Current Bus Master Secret
            returnBusMasterSecret(&data[0]);

            printf("The current Bus Master Secret is:  ");
            for(i=0;i<8;i++)
               printf("%02X ",data[i]);
            printf("\n");
            break;

         default:
            done = TRUE;
            break;
      }

      if(!done)
      {
         printf("\n");
         bank = selectBank(bank, SNum);
      }
   }
   while(!done);

   return bank;

}
//--------
//-------- Display Methods
//--------

/**
 * Display information about the 1-Wire device
 *
 * portnum port number for the device
 * SNum    serial number for the device
 */
void printDeviceInfo (int portnum, uchar SNum[8])
{
   int i;

   printf("\n");
   printf("*************************************************************************\n");
   printf("* Device Name:  %s\n", owGetName(&SNum[0]));
//   printf("* Device Other Names:  %s\n", owGetAlternateNames(portnum,&SNum[0]);
   printf("* Device Address:  ");
   for(i=7;i>=0;i--)
      printf("%02X ",SNum[i]);
   printf("\n");
   printf("* iButton Description:  %s\n",owGetDescription(&SNum[0]));
   printf("*************************************************************************\n");

/*      System.out.println(
         "* Device Max speed: "
         + ((owd.getMaxSpeed() == DSPortAdapter.SPEED_OVERDRIVE)
            ? "Overdrive"
            : "Normal"));*/
}

/**
 * Display the information about the current memory back provided.
 *
 * portnum port number for the device
 * SNum    serial number for the device
 */
void displayBankInformation (SMALLINT bank, int portnum, uchar SNum[8])
{
   printf("\n");
   printf("|------------------------------------------------------------------------\n");
   printf("| Bank:  %s\n",owGetBankDescription(bank,SNum));
   printf("| Size:  %d starting at physical address %d\n",owGetSize(bank,SNum),
      owGetStartingAddress(bank,SNum));
   printf("| Features:  ");

   if(owIsReadWrite(bank,portnum,SNum))
      printf("Read/Write ");

   if(owIsWriteOnce(bank,portnum,SNum))
      printf("Write-once ");

   if(owIsReadOnly(bank,portnum,SNum))
      printf("Read-only ");

   if(owIsGeneralPurposeMemory(bank,SNum))
      printf("general-purpose ");
   else
      printf("not-general-purpose ");

   if(owIsNonVolatile(bank,SNum))
      printf("non-volatile\n");
   else
      printf("volatile\n");

   if(owNeedsProgramPulse(bank,SNum))
      printf("|\tneeds-program-pulse \n");

   if(owNeedsPowerDelivery(bank,SNum))
      printf("|\tneeds-power-delivery \n");

   printf("| Pages:  %d pages of length %d bytes ",
      owGetNumberPages(bank,SNum),owGetPageLength(bank,SNum));

   if(owIsGeneralPurposeMemory(bank,SNum))
      printf("giving %d bytes Packet data payload",owGetMaxPacketDataLength(bank,SNum));

   printf("\n| Page Features:  ");

   if(owHasPageAutoCRC(bank,SNum))
      printf("page-device-CRC ");


   if(owCanRedirectPage(bank,SNum))
      printf("pages-redirectable ");

   if(owCanLockPage(bank,SNum))
      printf("pages-lockable \n");

   if((owCanLockPage(bank,SNum)) && (owCanLockRedirectPage(bank,SNum)))
      printf("|\tredirection-lockable\n");
   else if(owCanLockRedirectPage(bank,SNum))
      printf("redirection-lockable.");

   if(!owCanLockPage(bank,SNum))
      printf("\n");

   if(owHasExtraInfo(bank,SNum))
      printf("| Extra information for each page:  %s  length %d\n",
         owGetExtraInfoDesc(bank,SNum),
         owGetExtraInfoLength(bank,SNum));

   printf("|------------------------------------------------------------------------\n\n");
}



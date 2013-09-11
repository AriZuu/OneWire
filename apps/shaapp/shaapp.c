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
//  AHAapp.c - This utility debits money from a roving SHA iButton
//               (DS1963S) using a coprocessor SHA iButton.
//
//  Version:   2.00
//  History:
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ownet.h"
#include "ibsha33.h"

// mode constants
enum { MD_FIND_ROVING=0, MD_READ_AUTH, MD_MONEY_VERF, MD_MONEY_CHANGE,
       MD_MONEY_WRITE, MD_READ_BACK_AUTH, MD_VERIFY_GONE };

// local function prototypes
int ParseData(char *,int,uchar *,int);

// globals
long time_stamp;
// verbose output mode
int VERBOSE=0;

//----------------------------------------------------------------------
//  This is the Main routine for shaapp
//
// Note: This algorithm used is the SHA-1 algorithm as specified in the
// datasheet for the DS1961S, where the last step of the official
// FIPS-180 SHA routine is omitted (which only involves the addition of
// constant values).
//
int main(short argc, char **argv)
{
   char msg[200];
   int portnum = 0;
   uchar data[8];
   ushort i;
   uchar sn[8],secret[8], memory[32];
   int rslt,n=13,done=FALSE,skip;
   int reads=0;
   uchar indata[32];
   int address;
   ushort addr;
   uchar es;
   char hexstr[32];

   // check for required port name
   if (argc != 2)
   {
      sprintf(msg,"1-Wire Net name required on command line!\n"
                  " (example: \"COM1\" (Win32 DS2480),\"/dev/cua0\" "
                  "(Linux DS2480),\"1\" (Win32 TMEX)\n");
      printf("%s",msg);
      return 0;
   }

   for(i=0; i<8; i++)
   {
      secret[i] = 0xFF;
      data[i] = 0xFF;
   }

   if((portnum = owAcquireEx(argv[1])) < 0)
      ExitProg("Did not Acquire port.\n",1);
   else
   {
      rslt = owFirst(portnum, TRUE, FALSE);
      owSerialNum(portnum,sn,TRUE);

      while(rslt && ((sn[0] != 0xB3) && (sn[0] != 0x33)))
      {
         rslt = owNext(portnum,TRUE,FALSE);
         owSerialNum(portnum,sn,TRUE);
      }

      for(i=0;i<8;i++)
         printf("%02X ",sn[i]);
      printf("\n");

      if((sn[0] == 0xB3) || (sn[0] == 0x33))
      {
         if(ReadMem(portnum,128,indata))
         {
            if(((indata[8] != 0xAA) && (indata[8] != 0x55)) &&
               ((indata[11] != 0xAA) && (indata[11] != 0x55)))
            {
               // Clear all memory to 0xFFh
               for(i=0;i<16;i++)
               {
                  if(!LoadFirSecret(portnum,(ushort) (i*8),data,8))
                     printf("MEMORY ADDRESS %d DIDN'T WRITE\n",i*8);
               }

               printf("Current Bus Master Secret Is:\n");
               for(i=0;i<8;i++)
                  printf("%02X ",secret[i]);
               printf("\n");
            }
            else if((indata[9] != 0xAA) || (indata[9] != 0x55))
            {
               printf("Please Enter the Current Secret\n");
               printf("AA AA AA AA AA AA AA AA  <- Example\n");
               scanf("%s %s %s %s %s %s %s %s",&hexstr[0],&hexstr[2],&hexstr[4],
                      &hexstr[6],&hexstr[8],&hexstr[10],&hexstr[12],&hexstr[14]);

               if(!ParseData(hexstr,strlen(hexstr),data,16))
                  printf("DIDN'T PARSE\n");
               else
               {
                  printf("The secret read was:\n");
                  for(i=0;i<8;i++)
                  {
                     secret[i] = data[i];
                     printf("%02X ",secret[i]);
                     data[i] = 0xFF;
                  }
                  printf("\n");
               }

               printf("\n");

               if((indata[13] == 0xAA) || (indata[13] == 0x55))
                  skip = 4;
               else
                  skip = 0;

               for(i=skip;i<16;i++)
               {
                  ReadMem(portnum,(ushort)(((i*8)/((ushort)32))*32),memory);

                  if(WriteScratchSHAEE(portnum,(ushort) (i*8),&data[0],8))
                     CopyScratchSHAEE(portnum,(ushort) (i*8),secret,sn,memory);
               }
            }
            else
            {
               printf("Please Enter the Current Secret\n");
               printf("AA AA AA AA AA AA AA AA  <- Example\n");
               scanf("%s %s %s %s %s %s %s %s",&hexstr[0],&hexstr[2],&hexstr[4],
                      &hexstr[6],&hexstr[8],&hexstr[10],&hexstr[12],&hexstr[14]);
               if(!ParseData(hexstr,strlen(hexstr),secret,16))
                  printf("DIDN'T PARSE\n");
               else
               {
                  printf("The secret that was read:\n");
                  for(i=0;i<8;i++)
                  {
                     printf("%02X ",secret[i]);
                  }
                  printf("\n");
               }
            }
         }

         do
         {
            printf("PICK AN OPERATION:\n\n");
            printf("(1)  Read memory\n");                   // Reads memory page data

            printf("(2)  Write scratchpad\n");              // Write to scratch pad

            printf("(3)  Read scratchpad\n");               // Read scratch pad data

            printf("(4)  Copy scratchpad\n");               // Copy scratch pad to address

            printf("(5)  Load First Secret with Address\n");// Load first secret data

            printf("(6)  Compute Next Secret\n");           // Compute next secret and
                                                            // changes Bus Master secret

            printf("(7)  Read Auth page\n");                // Read authenticate page

            printf("(8)  Change/Load Secret Only\n");       // Changes/Loads Secret and
                                                            // Bus Master Secret

            printf("(9)  Change Bus Master Secret\n");       // Only changes Bus Master S.

            printf("(10) Write Protect Secret\n");          // Write protect secret

            printf("(11) Write Protect pages 0 to 3\n");    // Write protect pages 0 to 3

            printf("(12) EPROM mode control for page 1\n"); // EPROM mode for page 1

            printf("(13) Write Protect page 0 only\n");     // Write protect page 0.

            printf("(14) Print Current Bus Master Secret.\n");

            printf("(15) QUIT\n");

            scanf("%d",&n);
            if(n == 15)
            {
               n = 0;        //used to finish off the loop
               done = TRUE;
               break;
            }

            switch(n)
            {
               case 1:  // Read Memory
                  printf("\nEnter memory address to read as integer\n");
                  scanf("%d",&reads);
                  if(ReadMem(portnum,(ushort)reads,indata))
                  {
                     printf("\n");

                     for(i=0;i<8;i++)
                        printf("%02X ",indata[i]);
                     printf("\n");

                     for(i=8;i<16;i++)
                        printf("%02X ",indata[i]);
                     printf("\n");

                     for(i=16;i<24;i++)
                        printf("%02X ",indata[i]);
                     printf("\n");

                     for(i=24;i<32;i++)
                        printf("%02X ",indata[i]);
                     printf("\n");

                     printf("Read Complete\n\n\n");
                  }
                  else
                     printf("READ DIDN'T WORK\n\n\n");
                  break;

               case 2:  // Write Scratchpad
                  printf("\nEnter the 8 bytes of data to be written.\n");
                  printf("AA AA AA AA AA AA AA AA  <- Example\n");
                  scanf("%s %s %s %s %s %s %s %s",&hexstr[0],&hexstr[2],&hexstr[4],
                        &hexstr[6],&hexstr[8],&hexstr[10],&hexstr[12],&hexstr[14]);

                  printf("\nNow enter the address as an integer.\n");
                  scanf("%d",&address);

                  if(!ParseData(hexstr,strlen(hexstr),data,16))
                  {
                     printf("DIDN'T PARSE\n");
                     break;
                  }

                  ReadMem(portnum,(ushort)((address/((ushort)32))*32),memory);

                  if(WriteScratchSHAEE(portnum,(ushort)address,data,8))
                     printf("Write complete\n\n\n");
                  else
                     printf("WRITE DIDN'T WORK\n\n\n");
                  break;

               case 3:  // Read Scratchpad
                  printf("\n");
                  if(ReadScratchSHAEE(portnum,&addr,&es,data))
                  {
                     printf("Address is:   %d\n",addr);
                     printf("E/S Data is:  %02X\n",es);
                     printf("Scratch pad data is:\n");
                     for(i=0;i<8;i++)
                        printf("%02X ",data[i]);
                     printf("\n\n\n");
                  }
                  else
                     printf("READ DIDN'T WORK\n\n\n");
                  break;

               case 4:  // Copy Scratchpad
                  printf("\nEnter address to copy to as an integer.\n");
                  scanf("%d",&address);
                  if(CopyScratchSHAEE(portnum,(ushort)address,secret,sn,memory))
                     printf("Copy Scratch Pad Complete\n\n\n");
                  else
                     printf("COPY DIDN'T WORK\n\n\n");
                  break;

               case 5:  // Load First Secret
                  printf("\nEnter the 8 bytes of data to be written.\n");
                  printf("AA AA AA AA AA AA AA AA  <- Example\n");
                  scanf("%s %s %s %s %s %s %s %s",&hexstr[0],&hexstr[2],&hexstr[4],
                        &hexstr[6],&hexstr[8],&hexstr[10],&hexstr[12],&hexstr[14]);

                  printf("\nEnter the address where you want "
                         "to load the secret as an integer.\n");
                  scanf("%d",&address);

                  if(!ParseData(hexstr,strlen(hexstr),data,16))
                  {
                     printf("DIDN'T PARSE\n");
                     break;
                  }

                  if(LoadFirSecret(portnum,(ushort)address,data,8))
                  {
                     if(address == 128)
                     {
                        for(i=0;i<8;i++)
                           secret[i] = data[i];
                     }

                     printf("Load Complete\n\n\n");
                  }
                  else
                     printf("LOAD DIDN'T WORK\n\n\n");
                  break;

               case 6:  // Compute Next Secret
                  printf("\nEnter the address for the page of data to use in computation\n");
                  scanf("%d",&address);
                  if(NextSecret(portnum,(ushort)address,secret))
                  {
                     for(i=0;i<8;i++)
                        printf("%02X ",secret[i]);
                     printf("\n");

                     printf("Next secret is complete\n\n\n");
                  }
                  else
                     printf("NEXT SECRET DIDN'T WORK\n\n\n");
                  break;

               case 7:  // Read Authenticate SHA Page
                  printf("\nEnter the 8 bytes of challenge to use.\n");
                  printf("AA AA AA AA AA AA AA AA  <- Example\n");
                  scanf("%s %s %s %s %s %s %s %s",&hexstr[0],&hexstr[2],&hexstr[4],
                        &hexstr[6],&hexstr[8],&hexstr[10],&hexstr[12],&hexstr[14]);

                  printf("\nEnter address of page to read authenticate\n");
                  scanf("%d",&address);

                  if(!ParseData(hexstr,strlen(hexstr),data,16))
                  {
                     printf("DIDN'T PARSE\n");
                     break;
                  }


                  if(ReadAuthPageSHAEE(portnum,(ushort)address,secret,sn,indata,data))
                  {
                     printf("\n");

                     for(i=0;i<8;i++)
                        printf("%02X ",indata[i]);
                     printf("\n");

                     for(i=8;i<16;i++)
                        printf("%02X ",indata[i]);
                     printf("\n");

                     for(i=16;i<24;i++)
                        printf("%02X ",indata[i]);
                     printf("\n");

                     for(i=24;i<32;i++)
                        printf("%02X ",indata[i]);
                     printf("\n\n");

                     printf("Read Authenticate Complete\n\n\n");
                  }
                  else
                     printf("READ DIDN'T WORK\n\n\n");
                  break;

               case 8:  // Change/Load new secret
                  printf("\nEnter 8 byte secret you wish to load/change\n");
                  printf("AA AA AA AA AA AA AA AA  <- Example\n");
                  scanf("%s %s %s %s %s %s %s %s",&hexstr[0],&hexstr[2],&hexstr[4],
                        &hexstr[6],&hexstr[8],&hexstr[10],&hexstr[12],&hexstr[14]);

                  if(!ParseData(hexstr,strlen(hexstr),data,16))
                  {
                     printf("DIDN'T PARSE\n");
                     break;
                  }

                  if(LoadFirSecret(portnum,128,data,8))
                  {
                     for(i=0;i<8;i++)
                        secret[i] = data[i];

                     printf("Secret Loaded\n\n\n");
                  }
                  else
                     printf("SECRET NOT LOADED\n\n\n");
                  break;

               case 9:  // Change Bus Master Secret
                  printf("\nEnter 8 byte secret you wish to load for Bus Master\n");
                  printf("AA AA AA AA AA AA AA AA  <- Example\n");
                  scanf("%s %s %s %s %s %s %s %s",&hexstr[0],&hexstr[2],&hexstr[4],
                        &hexstr[6],&hexstr[8],&hexstr[10],&hexstr[12],&hexstr[14]);

                  if(!ParseData(hexstr,strlen(hexstr),data,16))
                  {
                     printf("DIDN'T PARSE\n");
                     break;
                  }
                  else
                  {
                     for(i=0;i<8;i++)
                        secret[i] = data[i];

                     printf("Bus Master Secret Changed.\n");
                  }

                  break;

               case 10:  // Lock Secret
                  printf("\n");
                  data[0] = 0xAA;
                  for(i=1;i<8;i++)
                     data[i] = 0x00;

                  ReadMem(portnum,128,memory);

                  if(WriteScratchSHAEE(portnum,136,data,8) &&
                     CopyScratchSHAEE(portnum,136,secret,sn,memory))
                     printf("Secret Locked\n\n\n");
                  else
                     printf("SECRET NOT LOCKED\n\n\n");
                  break;

               case 11:  // Write-protect pages 0 to 3
                  printf("\n");
                  data[0] = 0x00;
                  data[1] = 0xAA;
                  for(i=2;i<8;i++)
                     data[i] = 0x00;

                  ReadMem(portnum,128,memory);

                  if(WriteScratchSHAEE(portnum,136,data,8) &&
                     CopyScratchSHAEE(portnum,136,secret,sn,memory))
                     printf("Pages 0 to 3 write protected\n\n\n");
                  else
                     printf("PAGES NOT WRITE PROTECTED\n\n\n");
                  break;

               case 12:  // Set page 1 to EPROM mode
                  printf("\n");
                  for(i=0;i<4;i++)
                     data[i] = 0x00;

                  data[4] = 0xAA;

                  for(i=5;i<8;i++)
                     data[i] = 0x00;

                  ReadMem(portnum,128,memory);

                  if(WriteScratchSHAEE(portnum,136,data,8) &&
                     CopyScratchSHAEE(portnum,136,secret,sn,memory))
                     printf("EPROM mode control activated for page 1.\n\n\n");
                  else
                     printf("EPROM PROGRAMMING DIDN'T WORK\n\n\n");
                  break;

               case 13:  // Write protect page 0
                  printf("\n");
                  for(i=0;i<5;i++)
                     data[i] = 0x00;

                  data[5] = 0xAA;

                  for(i=6;i<8;i++)
                     data[i] = 0x00;

                  ReadMem(portnum,128,memory);

                  if(WriteScratchSHAEE(portnum,136,data,8) &&
                     CopyScratchSHAEE(portnum,136,secret,sn,memory))
                     printf("Page 0 Write-protected\n\n\n");
                  else
                     printf("NOT WRITE-PROTECTED\n\n\n");
                  break;

               case 14:  // Print Current Bus Master Secret
                  printf("\nThe Current Bus Master Secret is:\n");
                  for(i=0;i<8;i++)
                     printf("%02X ",secret[i]);
                  printf("\n\n\n");
                  break;

               default:
                  break;
            }

         }while(!done);

      }
      else
         printf("DS2432 not found on One Wire Network\n");

      owRelease(portnum);
   }

   return 1;
}


//----------------------------------------------------------------------
// Parse the raw file data in to an array of uchar's.  The data must
// be hex characters possible seperated by white space
//     12 34 456789ABCD EF
//     FE DC BA 98 7654 32 10
// would be converted to an array of 16 uchars.
// return the array length.  If an invalid hex pair is found then the
// return will be 0.
//
// inbuf    the input buffer that is to be parsed
// insize   the size of the input buffer
// outbuf   the parsed data from the input buffer
// maxsize  the max size that is to be parsed.
//
// return the length of that data that was parsed
//
int ParseData(char *inbuf, int insize, uchar *outbuf, int maxsize)
{
   int ps, outlen=0, gotmnib=0;
   uchar mnib;

   // loop until end of data
   for (ps = 0; ps < insize; ps++)
   {
      // check for white space
      if (isspace(inbuf[ps]))
         continue;
      // not white space, make sure hex
      else if (isxdigit(inbuf[ps]))
      {
         // check if have first nibble yet
         if (gotmnib)
         {
            // this is the second nibble
            outbuf[outlen++] = (mnib << 4) | ToHex(inbuf[ps]);
            gotmnib = 0;
         }
         else
         {
            // this is the first nibble
            mnib = ToHex(inbuf[ps]);
            gotmnib = 1;
         }
      }
      else
         return 0;

      // if getting to the max return what we have
      if ((outlen + 1) >= maxsize)
         return outlen;
   }

   return outlen;
}


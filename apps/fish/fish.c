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
//  fish.c - Goes through the testing of the file I/O functions
//  Version 2.00
//

#include <stdio.h>
#include <ctype.h>
#include "ownet.h"
#include "owfile.h"
#include "rawmem.h"
#include "mbshaee.h"

// Defines for the menu lists
#define MAIN_MENU          0
#define VERIFY_MENU        1
#define ENTRY_MENU         2
// Defines for the switch statements for Main Menu
#define MAIN_SELECT_DEVICE 0
#define CHANGE_DIR         1
#define MAIN_LIST          2
#define MAIN_LIST_ALL      3
#define MAIN_MKDIR         4
#define MAIN_MKFILE        5
#define MAIN_WRITE_FILE    6
#define MAIN_READ_FILE     7
#define MAIN_DELETE_DIR    8
#define MAIN_DELETE_FILE   9
#define MAIN_RENAME        10
#define MAIN_WRITE_ADDFILE 11
#define MAIN_TERMINATE     12
#define MAIN_FORMAT        13
#define MAIN_QUIT          14
// Format file system on 1-Wire device.
#define VERIFY_NO          0
#define VERIFY_YES         1
// Defines for the switch statements for data entry
#define MODE_TEXT          0
#define MODE_HEX           1
// Number of devices to list
#define MAXDEVICES         10
// Max number for data entry
#define MAX_DATA           256

// External functions
extern void owClearError(void);

// Local functions
int menuSelect(int menu);
SMALLINT selectDevice(int numDevices, uchar AllDevices[][8]);
void printDeviceInfo (int portnum, uchar SNum[8]);
int getNumber (int min, int max);
int getString(char *write_buff);
void listDir(int portnum,uchar *SNum, FileEntry file, int recursive);
void file_name(DirectoryPath *, FileEntry *, int);


int main(int argc, char **argv)
{
   int len,lentwo, selection;
   int done      = FALSE;
   int first     = TRUE;
   int portnum = 0;
   int error;
   uchar AllSN[MAXDEVICES][8];
   int NumDevices;
   int owd = 0;
   char msg[132];
   char dirname[64];
   char answer[4];
   char num[4];
   DirectoryPath dirpath;
   FileEntry file;
   int i,j,cnt,digcnt,maxwrite;
   short hnd;
   uchar data[MAX_DATA];

   // check for required port name
   if (argc != 2)
   {
      sprintf(msg,"1-Wire Net name required on command line!\n"
                  " (example: \"COM1\" (Win32 DS2480D),\"/dev/cua0\" "
                  "(Linux DS2480D),\"1\" (Win32 TMEX)\n");
      printf("%s\n",msg);
      return 0;
   }

   printf("\n1-Wire File Shell for the Public Domain Kit Version 0.00\n");

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
         if(first)
         {
            selection = MAIN_SELECT_DEVICE;
            first = FALSE;
         }
         else
            selection = menuSelect(MAIN_MENU);

         // Main menu
         switch(selection)
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

               if(isJob(portnum,&AllSN[owd][0]))
               {
                  printf("Would you like to write the programmed job "
                         "before choosing another button?(Y/N)\n");
                  len = getString(&answer[0]);

                  if((answer[0] == 'Y') || (answer[0] == 'y'))
                     if(!owDoProgramJob(portnum,&AllSN[owd][0]))
                        OWERROR_DUMP(stderr);
               }

               // select a device
               owd = selectDevice(NumDevices,&AllSN[0]);

               // display device info
               printDeviceInfo(portnum,&AllSN[owd][0]);

               ChangeRom(portnum,&AllSN[owd][0]);

               break;

	    		case MAIN_FORMAT:
	      		if(menuSelect(VERIFY_MENU) == VERIFY_YES)
	      		{
                  if(!isJob(portnum,&AllSN[owd][0]))
                     owCreateProgramJob(portnum,&AllSN[owd][0]);

                  owClearError();

	         		if(!owFormat(portnum,&AllSN[owd][0]))
		   	   		OWERROR_DUMP(stderr);
	      		}

               break;

            case CHANGE_DIR:
               printf("Enter the directory you want to change to from the root "
                      "(/ for root): ");

               len = getString(&dirname[0]);

               cnt = 0;
               dirpath.NumEntries = 0;

               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               if(dirname[0] == '.')
                  dirpath.Ref = dirname[i];

               if(len>1)
                  dirpath.NumEntries++;

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;



            case MAIN_LIST:
               owClearError();
            	printf("Enter the directory to list on (/ for root):  ");

            	// get the directory and create a file on it
            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               if(dirname[0] == '.')
                  dirpath.Ref = dirname[i];

               if(len>1)
                  dirpath.NumEntries++;

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
					   // list the files without recursion
                  listDir(portnum,&AllSN[owd][0],file,FALSE);

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_LIST_ALL:
   			   // list the files with recursion
               owClearError();
               dirpath.Ref = '0';
               dirpath.NumEntries = 0;
               owChangeDirectory(portnum,&AllSN[owd][0],&dirpath);

               listDir(portnum,&AllSN[owd][0],file,TRUE);

               break;

	    		case MAIN_MKDIR:
               printf("Enter the directory to create from <root>  ");

            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
                  else
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     cnt = 0;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!isJob(portnum,&AllSN[owd][0]))
                     owCreateProgramJob(portnum,&AllSN[owd][0]);

                  owClearError();

                  if(!owCreateDir(portnum,&AllSN[owd][0],&file))
                  {
                     file_name(&dirpath,&file,TRUE);
                     OWERROR_DUMP(stderr);
                  }
               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

	    		case MAIN_MKFILE:
               printf("Enter the file name to create along with path from <root>  ");

            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]) && (digcnt < 5));
                           num[digcnt-1] = 0;

                           file.Ext = atoi(&num[0]);

                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }

                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!isJob(portnum,&AllSN[owd][0]))
                     owCreateProgramJob(portnum,&AllSN[owd][0]);

                  owClearError();

                  if(!owCreateFile(portnum,&AllSN[owd][0],&maxwrite,&hnd,&file))
                  {
                     file_name(&dirpath,&file,TRUE);
                     OWERROR_DUMP(stderr);
                  }
                  else if(!owCloseFile(portnum,&AllSN[owd][0],hnd))
                  {
                     OWERROR_DUMP(stderr);
                  }
               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_WRITE_FILE:
               printf("Enter the directory and file you would like to write to: ");

               len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;

               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!isJob(portnum,&AllSN[owd][0]))
                     owCreateProgramJob(portnum,&AllSN[owd][0]);

                  owClearError();

                  if (menuSelect(ENTRY_MENU) == MODE_TEXT)
                  {
                     len = getData(&data[0],MAX_DATA,FALSE);
                  }
                  else
                  {
                     len = getData(&data[0],MAX_DATA,TRUE);
                  }

                  owClearError();
                  if(!owOpenFile(portnum,&AllSN[owd][0],&file,&hnd))
                  {
                     error = owGetErrorNum();
                     if(error == OWERROR_FILE_NOT_FOUND)
                     {
                        if(!owCreateFile(portnum,&AllSN[owd][0],&maxwrite,&hnd,&file))
                        {
                           file_name(&dirpath,&file,TRUE);
                           OWERROR_DUMP(stderr);
                        }
                        else
                        {
                           if(!owOpenFile(portnum,&AllSN[owd][0],&file,&hnd))
                           {
                              file_name(&dirpath,&file,TRUE);
                              OWERROR_DUMP(stderr);
                           }
                           else if(!owWriteFile(portnum,&AllSN[owd][0],hnd,&data[0],len))
                           {
                              OWERROR_DUMP(stderr);
                           }
                        }
                     }
                     else
                     {
                        OWERROR(error);
                        OWERROR_DUMP(stderr);
                     }
                  }
                  else if(!owWriteFile(portnum,&AllSN[owd][0],hnd,&data[0],len))
                  {
                     OWERROR_DUMP(stderr);
                  }
               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_READ_FILE:
               printf("Enter the directory and file you would like to read from: ");

               len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;

               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!owOpenFile(portnum,&AllSN[owd][0],&file,&hnd))
                  {
                     file_name(&dirpath,&file,TRUE);
                     OWERROR_DUMP(stderr);
                  }
                  else if(!owReadFile(portnum,&AllSN[owd][0],hnd,&data[0],MAX_DATA,&len))
                  {
                     OWERROR_DUMP(stderr);
                  }
                  else
                     for(i=0;i<len;i++)
                        printf("%02X ",data[i]);

                  if(!owCloseFile(portnum,&AllSN[owd][0],hnd))
                     OWERROR_DUMP(stderr);

               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_DELETE_DIR:
               printf("Enter the directory to delete from <root>  ");

            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }

                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];
               dirpath.NumEntries++;

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!isJob(portnum,&AllSN[owd][0]))
                     owCreateProgramJob(portnum,&AllSN[owd][0]);

                  owClearError();

                  if(!owRemoveDir(portnum,&AllSN[owd][0],&file))
                     OWERROR_DUMP(stderr);
               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_DELETE_FILE:
               printf("Enter the file to delete from <root>  ");

            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));
                           num[digcnt-1] = 0;

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!isJob(portnum,&AllSN[owd][0]))
                  {
                     owCreateProgramJob(portnum,&AllSN[owd][0]);
                     owClearError();
                  }

                  if(!owDeleteFile(portnum,&AllSN[owd][0],&file))
                     OWERROR_DUMP(stderr);
               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_RENAME:
               printf("Enter the file to rename from <root>:  ");

            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));
                           num[digcnt-1] = 0;

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!isJob(portnum,&AllSN[owd][0]))
                     owCreateProgramJob(portnum,&AllSN[owd][0]);

                  owClearError();

                  if(!owOpenFile(portnum,&AllSN[owd][0],&file,&hnd))
                  {
                     file_name(&dirpath,&file,TRUE);
                     OWERROR_DUMP(stderr);
                  }
                  else
                  {
                     printf("Enter the name to rename the file: ");
                     len = getString(&dirname[0]);
                     for(i=0;i<len;i++)
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           } while(isdigit(dirname[i+digcnt]));
                           num[digcnt-1] = 0;

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;
                        }
                        else
                           file.Name[i] = dirname[i];
                     }

                     if(!owReNameFile(portnum,&AllSN[owd][0],hnd,&file))
                     {
                        file_name(&dirpath,&file,TRUE);
                        OWERROR_DUMP(stderr);
                     }

                     if(!owCloseFile(portnum,&AllSN[owd][0],hnd))
                        OWERROR_DUMP(stderr);
                  }

               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_WRITE_ADDFILE:
               if(!isJob(portnum,&AllSN[owd][0]))
               {
                  if(!owCreateProgramJob(portnum,&AllSN[owd][0]))
                     OWERROR_DUMP(stderr);
               }

               owClearError();

               printf("Enter the file to addfile to create or append to from <root>:  ");

            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));
                           num[digcnt-1] = 0;

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!owOpenFile(portnum,&AllSN[owd][0],&file,&hnd))
                  {
                     owClearError();
                     if(!owCreateFile(portnum,&AllSN[owd][0],&maxwrite,&hnd,&file))
                     {
                        file_name(&dirpath,&file,TRUE);
                        OWERROR_DUMP(stderr);
                     }
                     else
                        printf("File was created.\n");
                  }
                  else
                  {
                     if(menuSelect(ENTRY_MENU) == MODE_TEXT)
                     {
                        len = getData(&data[0],MAX_DATA,FALSE);
                     }
                     else
                     {
                        len = getData(&data[0],MAX_DATA,TRUE);
                     }

                     printf("Do you want to append this data?\n");
                     lentwo = getString(&answer[0]);
                     if((answer[0] == 'Y') || (answer[0] == 'y'))
                     {
                        if(!owWriteAddFile(portnum,&AllSN[owd][0],hnd,1,0,&data[0],&len))
                           OWERROR_DUMP(stderr);

                        if(!owCloseFile(portnum,&AllSN[owd][0],hnd))
                           OWERROR_DUMP(stderr);

                     }
                     else
                     {
                        printf("What offset would you like to start writing the data?\n");

                        len = getData(&data[0],MAX_DATA,FALSE);

                        if(!owWriteAddFile(portnum,&AllSN[owd][0],
                                           hnd,0,answer[0],&data[0],&len))
                           OWERROR_DUMP(stderr);

                        if(!owCloseFile(portnum,&AllSN[owd][0],hnd))
                           OWERROR_DUMP(stderr);

                     }
                  }

               }

               dirpath = owGetCurrentDir(portnum,&AllSN[owd][0]);
               printf("\nThe Current Directory Path is> ");
               if(dirpath.NumEntries == 0)
                  printf(" <root> ");
               else
               {
                  for(i=0;i<dirpath.NumEntries;i++)
                  {
                     for(j=0;j<4;j++)
                        printf("%c",dirpath.Entries[i][j]);
                     printf("/");
                  }
               }
               printf("\n");

               break;

            case MAIN_TERMINATE:
               printf("Enter the addfile to terminate from <root>:  ");

            	len = getString(&dirname[0]);
               cnt = 0;
               dirpath.NumEntries = 0;
               for(i=0;i<len;i++)
               {
                  if(dirname[i] != '/')
                  {
                     if((cnt < 4) && (dirname[i] != '.') && isalnum(dirname[i]))
                        dirpath.Entries[dirpath.NumEntries][cnt++] = dirname[i];
                     else
                     {
                        if(dirname[i] == '.')
                        {
                           digcnt = 1;
                           do
                           {
                              if(isdigit(dirname[i+digcnt]))
                              {
                                 num[digcnt-1] = dirname[i+digcnt];
                                 digcnt++;
                              }
                           }while(isdigit(dirname[i+digcnt]));
                           num[digcnt-1] = 0;

                           file.Ext = atoi(&num[0]);
                           i = i + digcnt;

                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';
                        }
                        else if((i+1) == len)
                        {
                           for(j=cnt;j<4;j++)
                              dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                           cnt = 0;
                        }
                     }
                  }
                  else if(cnt > 0)
                  {
                     for(j=cnt;j<4;j++)
                        dirpath.Entries[dirpath.NumEntries][cnt++] = ' ';

                     dirpath.NumEntries++;
                     cnt = 0;
                  }
                  else if(dirname[0] == '/')
                  {
                     dirpath.NumEntries = 0;
                     dirpath.Entries[0][0] = '.';
                     dirpath.Entries[0][1] = ' ';
                     break;
                  }
               }

               for(i=0;i<4;i++)
                  file.Name[i] = dirpath.Entries[dirpath.NumEntries][i];

               if(!owChangeDirectory(portnum,&AllSN[owd][0],&dirpath))
               {
                  file_name(&dirpath,&file,FALSE);
                  OWERROR_DUMP(stderr);
               }
               else
               {
                  if(!owOpenFile(portnum,&AllSN[owd][0],&file,&hnd))
                  {
                     file_name(&dirpath,&file,TRUE);
                     OWERROR_DUMP(stderr);
                  }
                  else
                  {
                     if(!isJob(portnum,&AllSN[owd][0]))
                     {
                        if(!owCreateProgramJob(portnum,&AllSN[owd][0]))
                           OWERROR_DUMP(stderr);
                     }

                     owClearError();

                     if(!owTerminateAddFile(portnum,&AllSN[owd][0],&file))
                     {
                        OWERROR_DUMP(stderr);
                     }
                     else
                        printf("File will be terminated once job is programmed.\n");
                  }
               }

               break;


            case MAIN_QUIT:
               if(isJob(portnum,&AllSN[owd][0]))
               {
                  printf("Would you like to write the programmed job "
                         "before choosing another button?(Y/N)\n");
                  printf("The job will continue and try and write to the \n");
                  printf("iButton until it is successful or you hit Ctrl-C.\n");

                  len = getString(&answer[0]);

                  if((answer[0] == 'Y') || (answer[0] == 'y'))
                  {
                     do
                     {
                        if(!owDoProgramJob(portnum,&AllSN[owd][0]))
                        {
                           OWERROR_DUMP(stderr);
                        }
                        else
                        {
                           done = TRUE;
                        }
                     }
                     while((!done) || (!key_abort()));
                  }
               }

               done = TRUE;
               break;

            default:
               break;

         }  // Main menu switch

      }
      while (!done);  // loop to do menu

      owRelease(portnum);
   }  // else for owAcquire

   return 1;
}


/**
 * listDir
 *
 * Lists the current files/directories if recursive is FALSE
 * otherwise list all files and directories
 *
 *
 */
void listDir(int portnum,uchar *SNum, FileEntry file, int recursive)
{
   int done;
   int i,j;
   DirectoryPath dirpath[10];
   DirectoryPath init;
   int dir_cnt = 0;

   init = owGetCurrentDir(portnum,SNum);

   done = FALSE;
   do
   {
      done = !owNextFile(portnum,SNum,&file);

      if(file.Name[0] == '.')
      {
         done = !owNextFile(portnum,SNum,&file);
         if(file.Name[0] == '.')
            done = !owNextFile(portnum,SNum,&file);
      }

      if(!done)
      {
         if(!(recursive && (file.Ext == 0x7F)))
         {
            if(recursive)
            {
               printf("|");
               for(i=0;i<(4*init.NumEntries)+4;i++)
                  printf("-");
            }
            for(i=0;i<4;i++)
               if(file.Name[i] != ' ')
                  printf("%c",file.Name[i]);
         }
         if(file.Ext == 0x7F && !recursive)
            printf("   <dir>\n");
         else if(file.Ext == 0x7F)
         {
            dirpath[dir_cnt] = owGetCurrentDir(portnum,SNum);
            for(j=0;j<4;j++)
               dirpath[dir_cnt].Entries[dirpath[dir_cnt].NumEntries][j] =
                  file.Name[j];
            dirpath[dir_cnt++].NumEntries++;
         }
         else
            printf(".%03d\n",file.Ext);
      }
   } while (!done);
   done = FALSE;

   for(i=0;i<dir_cnt;i++)
   {
      owChangeDirectory(portnum,SNum,&dirpath[i]);

      printf("|");
      for(j=0;j<(4*dirpath[i].NumEntries)-1;j++)
         printf("-");
      printf("|");
      for(j=0;j<4;j++)
         file.Name[j] =
            dirpath[i].Entries[dirpath[i].NumEntries-1][j];

      for(j=0;j<4;j++)
         printf("%c",file.Name[j]);
      printf("\n");
      listDir(portnum,SNum,file,recursive);
   }

   if(dirpath[dir_cnt-1].NumEntries == 1)
   {
      dirpath[0].Ref = '0';
      dirpath[0].NumEntries = 0;
      owChangeDirectory(portnum,SNum,&dirpath[0]);
   }

   if(owHasErrors())
      OWERROR_DUMP(stderr);
}

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
int menuSelect(int menu)
{
   int length = 0;

   switch(menu)
   {
      case MAIN_MENU:
         printf("\n");
         printf("----  1-Wire File Shell ----\n");
         printf("(0) Select Device\n");
         printf("(1) Change Directory\n");
         printf("(2) Directory list\n");
         printf("(3) Directory list (recursive)\n");
         printf("(4) Make Directory\n");
         printf("(5) Create empty file \n");
         printf("(6) Write data to a file\n");
         printf("(7) Read data from a file\n");
         printf("(8) Delete 1-Wire directory\n");
         printf("(9) Delete 1-Wire file\n");
         printf("(10) Rename 1-Wire file\n");
         printf("(11) Write or Add to AddFile\n");
         printf("(12) Terminate an AddFile\n");
         printf("(13) Format filesystem on 1-Wire device\n");
         printf("[14]-Quit\n");
         length = 14;
         break;

      case VERIFY_MENU:
         printf("\n");
         printf("Format file system on 1-Wire device?\n");
         printf("(0) NO\n");
         printf("(1) YES (delete all files/directories)\n");
         length = 1;
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
 * Retrieve user input from the console in the form of hex or text.
 *
 * write_buff  the buffer for the data to be written into.
 *
 * @return length of array
 */
int getString(char *write_buff)
{
   char ch;
   int  cnt = 0, temp_cnt = 0;
   int  done = FALSE;

   do
   {
      ch = (char) getchar();

      if(!isspace(ch))
         write_buff[cnt++] = ch;
      else if((cnt > 0) && (ch != ' '))
      {
      	write_buff[cnt++] = 0;
         done = TRUE;
      }
   }
   while(!done);

   return cnt;
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
   printf("* Device Address:  ");
   for(i=7;i>=0;i--)
      printf("%02X ",SNum[i]);
   printf("\n");
   printf("* iButton Description:  %s\n",owGetDescription(&SNum[0]));
   printf("*************************************************************************\n");

}

/**
 * Display the correct file structure.
 */
void file_name(DirectoryPath *dirpath, FileEntry *file, int inc_filename)
{
   int i,j;

   printf("The file format from the root should look like this:\n");
   printf("suba/subb/newf.5\n");

   if(dirpath->NumEntries > 0)
   {
      printf("This is the path you gave:\n");

      for(i=0;i<dirpath->NumEntries;i++)
      {
         for(j=0;j<4;j++)
         {
            if(dirpath->Entries[i][j] != ' ')
               printf("%c",dirpath->Entries[i][j]);
         }
         printf("/\n");
      }
   }
   else
   {
      printf("No path was read and if '\\' was the first symbol\n");
      printf("then no path was read and the file name isn't going\n");
      printf("to be valid.\n");
   }

   if(inc_filename)
   {
      printf("The file name is:\n");

      for(i=0;i<4;i++)
      {
         if(file->Name[i] != ' ')
            printf("%c",file->Name[i]);
      }
      printf("\n");
   }
}


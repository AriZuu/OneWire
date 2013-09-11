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
//  screenio.c - Utility functions for SHA iButton applications
//
//  Version:   2.00
//  History:   
//

#include "ownet.h"

// Include files for the Palm and Visor
#ifdef __MC68K__
#include <PalmOS.h>
#include <Window.h>
#endif

// defines
#define MAXLINE 16

// External functions
extern void msDelay(int);

// local function prototypes
void output_status(int, char *);
void reset_screen(void);
int available_screen(int);
void ClearScreen(int);

// globals
int VERBOSE=0;

// keep track of line number on screen
int current_line = 0;

//--------------------------------------------------------------------------
//  output status message
//
void output_status(int level, char *st)
{
   char *p;
   static char linebuf[256];
   static int cnt = 0; 

   // skip null strings
   if (st[0] == 0)
      return;

   // check if in verbose mode
   if ((level >= LV_ALWAYS) || VERBOSE)
   { 
      // look for embedded \n
      for (p = st; *p; p++)
      {
         if (*p == 0x0A || ((cnt>33) && (*p==' ')) || (*(p + 1) == 0))
         {
            // zero terminate the line
            linebuf[cnt] = 0;
            
            // print it out ???????? replace
#ifndef __MC68K__
            printf("%s",linebuf);
#else
      		WinDrawChars(linebuf,StrLen(linebuf),5,10*current_line++);
#endif

            // check if too many lines
            if (current_line == MAXLINE)
               reset_screen();
            cnt = 0;
         }
         else
            linebuf[cnt++] = *p;
      }
   }
}

//--------------------------------------------------------------------------
//  check for available space on screen.  Return 1 if available 0 otherwise
//
int available_screen(int lines)
{
  return !((current_line + lines) > MAXLINE);
}

//--------------------------------------------------------------------------
//  reset the screen 
//
void reset_screen(void)
{
   // set lines back to zero
   current_line = 0;

   // reset screen
   ClearScreen(0);
	
}


//------------------------------------------------------------------------
//  Clears the screen on the palm device
//
void ClearScreen(int row)
{
#ifdef __MC68K__
	int i;

	for(i=row;i<16;i++)
		WinDrawChars((char *) "                                                                                ",80,5,i*10);
#endif
}


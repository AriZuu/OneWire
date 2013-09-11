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

#ifndef SER550_H
#define SER550_H

#ifdef __C51__ //Keil's C51 compiler
   // for nop instruction
   #include <intrins.h>
   // sfr/sbit definitions
   #include <reg550.h>
#endif

//which serial port to use for console I/O
#ifndef STDOUT_P
   #define STDOUT_P 1
   #define stdout 1
#endif

//which serial port to use for 1-Wire I/O w/ ds2480-based adapter
#ifndef ONEWIRE_P
   #define ONEWIRE_P 0
#endif

// Port for general 1Wire communication using micro's port pin.
// WARNING: See appnote 132 to select an appropriate port pin and
//          allow for appropriate protection of the pin.
#ifndef OW_PORT
   #define OW_PORT P3_4
#endif

/* Timer reload values for baud rates:
      TH1 = 256 - ( (2^SMOD/32) * ( xtal / T1DIV * baud) ) */
/* NOTE: mileage may vary on different platforms.
         With any problems, please check these values first */
#define BAUD9600_TIMER_RELOAD_VALUE (256-((2*33000000)/(32L*4*9600)))
#define BAUD19200_TIMER_RELOAD_VALUE (256-((2*33000000)/(32L*4*19200)))
#define BAUD57600_TIMER_RELOAD_VALUE (256-((2*33000000)/(32L*4*57600)))
//not supported at 33 mhz
#define BAUD115200_TIMER_RELOAD_VALUE (256-((2*33000000)/(32L*4*115200)))

/* Should serial I/O be interrupt driven? */
#ifndef USE_SERIAL_INTERRUPTS
#define USE_SERIAL_INTERRUPTS 1
#endif

/* Should both ports run off timer 1? */
#ifndef USE_TIMER2_FOR_SERIAL0
#define USE_TIMER2_FOR_SERIAL0 0
#endif

#ifdef USE_SERIAL_INTERRUPTS
#define SERIAL_BUFFER_SIZE 256 //DONT CHANGE! Currently breaks code in ds520lnk
#endif

#endif// SER550_H
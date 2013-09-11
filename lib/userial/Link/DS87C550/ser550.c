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
//  ser550.C - Serial functions for the DS87c550.  Should be fairly portable
//             among most 8051-based micros.
//
//  Version: 1.00
//
//

#include "ownet.h"
#include "ser550.h"

extern void usDelay(int);

/* serial0 routines */
void serial0_init(uchar);
void serial0_flush(void);
/* void serial0_handler(void) */
void serial0_putchar(char);
char serial0_getchar(void);
char serial0_peek(void);

/* serial1 routines */
void serial1_init(uchar);
void serial1_flush(void);
/* void serial1_handler(void) */
void serial1_putchar(char);
char serial1_getchar(void);
char serial1_peek(void);

#if USE_SERIAL_INTERRUPTS
	// this is a ring buffer and can overflow at anytime!
	static xdata unsigned char receiveBuffer0[SERIAL_BUFFER_SIZE];
	static data volatile unsigned char receiveHead0 = 0;
	static data volatile unsigned char receiveTail0 = 0;
	static xdata unsigned char receiveBuffer1[SERIAL_BUFFER_SIZE];
	static data volatile unsigned char receiveHead1 = 0;
	static data volatile unsigned char receiveTail1 = 0;

   // not buffering on transmit
	static data volatile unsigned char transmitIsBusy0 = 0;
	static data volatile unsigned char transmitIsBusy1 = 0;
#endif

void serial0_init(uchar reload_value)
{
   ES0 = 0;
#if USE_TIMER2_FOR_SERIAL0
   TR2 = 0; // stop timer 2
#else
	TR1 = 0; // stop timer 1
#endif

	// set 8 bit uart with variable baud from timer
	// enable receiver and clear RI and TI
	SCON0 = 0x50;
	PCON |= 0x80; // baud rate double for serial0

#if USE_TIMER2_FOR_SERIAL0
	//T2CON |= 0x30;	//enable Timer2 control for serial0
	T2CON = 0x34;	//enable Timer2 control for serial0
	T2MOD = (T2MOD&0x0f) | 0x20; // timer 2 is an 8bit auto-reload counter

   TL2 = RCAP2L = reload_value;
   TH2 = RCAP2H = reload_value>>8;

   TF2 = 0; // clear timer 2 overflow flag
   TR2 = 1; // start timer 2
#else
	//T2CON &= 0xCF;	//disable Timer2 control for serial0
	TMOD = (TMOD&0x0F) | 0x20; // timer 1 is an 8bit auto-reload counter
   CKCON |= 0x10; // timer uses crystal/4
	TL1 = TH1 = reload_value;

	TF1 = 0; // clear timer 1 overflow flag
	TR1 = 1; // start timer 1
#endif

   RI = 0;
#if USE_SERIAL_INTERRUPTS
	receiveHead0 = receiveTail0 = 0;
	transmitIsBusy0 = 0;
   TI = 0;
   EA = 1; // global interrupt enable
	ES0 = 1; // enable serial channel 1 interrupt
#else
	TI = 1; // transmit buffer empty
#endif

}

void serial0_flush(void)
{
   ES0 = 0;
	RI = 0; // receive buffer empty
#if USE_SERIAL_INTERRUPTS
	receiveHead0 = receiveTail0 = 0;
	transmitIsBusy0 = 0;
   TI = 0;
	ES0 = 1;
#else
	TI = 1; // transmit buffer empty
#endif
}


void serial1_init(uchar reload_value)
{
   ES1 = 0;
	TR1 = 0; // stop timer 1

	// set 8 bit uart with variable baud from timer
	// enable receiver and clear RI and TI
	SCON1 = 0x50;
	WDCON |= 0x80; // baud rate double for serial1

	TMOD = (TMOD&0x0F) | 0x20; // timer 1 is an 8bit auto-reload counter
   CKCON |= 0x10; // timer uses crystal/4

	TL1 = TH1 = reload_value;

	TF1 = 0; // clear timer 1 overflow flag
	TR1 = 1; // start timer 1

   RI1 = 0;
#if USE_SERIAL_INTERRUPTS
	receiveHead1 = receiveTail1 = 0;
	transmitIsBusy1 = 0;
   TI1 = 0;
   EA = 1; // global interrupt enable
	ES1 = 1; // enable serial channel 1 interrupt
#else
	TI1 = 1; // transmit buffer empty
#endif
}

void serial1_flush(void)
{
   ES1 = 0;
   RI1 = 0;
#if USE_SERIAL_INTERRUPTS
	receiveHead1 = receiveTail1 = 0;
	transmitIsBusy1 = 0;
   TI1 = 0;
   ES1 = 1;
#else
	TI1 = 1; // transmit buffer empty
#endif
}

#if USE_SERIAL_INTERRUPTS

void serial0_handler (void) interrupt 13 using 2
{
	if (RI)
	{
   	//usDelay(6);
		receiveBuffer0[receiveHead0++] = SBUF0;
		if (receiveHead0==receiveTail0) /* buffer overrun, sorry :) */
			receiveTail0 += 1;
		RI = 0;
	}
	if (TI)
	{
	   //while(TI)
   		TI = 0;
		transmitIsBusy0 = 0;
	}
}

void serial0_putchar(char c)
{
	while (transmitIsBusy0)
		/*no-op*/;
	transmitIsBusy0 = 1;
	//usDelay(6);
	SBUF0 = c;
}

char serial0_getchar(void)
{
	char c;
	while (receiveHead0==receiveTail0)
	   /*no-op*/;
	c = receiveBuffer0[receiveTail0];
	ES0 = 0; // disable serial interrupts
	receiveTail0++;
	ES0 = 1; // enable serial interrupts
	return c;
}

char serial0_peek(void)
{
   if(receiveHead0==receiveTail0)
      return FALSE;
   else
      return TRUE;
}

void serial1_handler (void) interrupt 1 using 2
{
	if (RI1)
	{
   	//usDelay(6); //bug fix
		receiveBuffer1[receiveHead1++] = SBUF1;
		if (receiveHead1==receiveTail1) /* buffer overrun, sorry :) */
			receiveTail1++;
		RI1 = 0;
	}
	if (TI1)
	{
	   //while(TI1) //bug fix
   		TI1 = 0;
		transmitIsBusy1 = 0;
	}
}

void serial1_putchar(char c)
{
	while (transmitIsBusy1)
		/*no-op*/;
	transmitIsBusy1 = 1;
	//usDelay(6); //bug fix
	SBUF1 = c;
}

char serial1_getchar(void)
{
	char c;
	while (receiveHead1==receiveTail1)
	   /*no-op*/;
	c = receiveBuffer1[receiveTail1];
	ES1 = 0; // disable serial interrupts
   receiveTail1++;
	ES1 = 1; // enable serial interrupts
	return c;
}

char serial1_peek(void)
{
   if(receiveHead1==receiveTail1)
      return FALSE;
   else
      return TRUE;
}

#else //ifdef USE_SERIALINTERRUPTS

void serial0_handler (void) interrupt 13 using 2
{
	ES0 = 0; // disable serial interrupts
}

void serial0_putchar(char c)
{
	while (!TI)
		/*no-op*/;
   //while(TI) //bug fix
   	TI = 0;
   //usDelay(6); //bug fix
	SBUF0 = c;
}

char serial0_getchar(void)
{
   char c;
	while (!RI)
		/*no-op*/;
   //usDelay(6); //bug fix
	c = SBUF0;
	RI = 0;
	return c;
}

char serial0_peek(void)
{
   if(RI)
      return TRUE;
   else
      return FALSE;
}

void serial1_handler (void) interrupt 1 using 2
{
	ES1 = 0; // disable serial interrupts
}

void serial1_putchar (char c)
{
   while (!TI1)
      /*no-op*/;
   //while(TI1) //bug fix
   	TI1 = 0;
   //usDelay(6); //bug fix
   SBUF1 = c;
}

char serial1_getchar (void)
{
   char c;
	while (!RI1)
		/*no-op*/;
   //usDelay(6); //bug fix
	c = SBUF1;
	RI1 = 0;
	return c;
}

char serial1_peek(void)
{
   if(RI1)
      return TRUE;
   else
      return FALSE;
}

#endif // ifdef USE_SERIALINTERRUPTS


#if __C51__
//All that should be necessary is a macro, but not with Keil...

#if (STDOUT_P==0)
//Keil C51 v5.10 uses non-standard expression for putchar
char putchar(char c)
{
   serial0_putchar(c);
   return c;
}

char getchar(void)
{
   return serial0_getchar();
}
#else
//Keil C51 v5.10 uses non-standard expression for putchar
char putchar(char c)
{
   serial1_putchar(c);
   return c;
}

char getchar(void)
{
   return serial1_getchar();
}
#endif
#endif
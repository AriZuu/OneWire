//---------------------------------------------------------------------------
// Copyright (C) 2003 Dallas Semiconductor Corporation, All Rights Reserved.
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
//  ser400.C - Serial functions for the DS80c400.  Should be fairly portable
//             among most 8051-based micros.
//
//  Version: 1.00
//
//

#include "ownet.h"
#include "ser400.h"
#include "tini400_isr.h"
#include <reg400.h>

extern void usDelay(int);

/* serial0 routines */
void serial0_init(uchar);
void serial0_flush(void);
void serial0_handler(void);
void serial0_putchar(char);
char serial0_getchar(void);
char serial0_peek(void);
void serial0_break(uchar);
void serial0_shutdown(void);

/* serial1 routines */
void serial1_init(uchar);
void serial1_flush(void);
void serial1_handler(void);
void serial1_putchar(char);
char serial1_getchar(void);
char serial1_peek(void);
void serial1_break(uchar);
void serial1_shutdown(void);

/* serial2 routines */
void serial2_init(uchar);
void serial2_flush(void);
void serial2_handler(void);
void serial2_putchar(char);
char serial2_getchar(void);
char serial2_peek(void);
void serial2_break(uchar);
void serial2_shutdown(void);


#if USE_SERIAL_INTERRUPTS
	// this is a ring buffer and can overflow at anytime!
	static xdata unsigned char receiveBuffer0[SERIAL_BUFFER_SIZE];
	static data volatile unsigned char receiveHead0 = 0;
	static data volatile unsigned char receiveTail0 = 0;
	static xdata unsigned char receiveBuffer1[SERIAL_BUFFER_SIZE];
	static data volatile unsigned char receiveHead1 = 0;
	static data volatile unsigned char receiveTail1 = 0;
	static xdata unsigned char receiveBuffer2[SERIAL_BUFFER_SIZE];
	static data volatile unsigned char receiveHead2 = 0;
	static data volatile unsigned char receiveTail2 = 0;

   // not buffering on transmit
	static data volatile unsigned char transmitIsBusy0 = 0;
	static data volatile unsigned char transmitIsBusy1 = 0;
	static data volatile unsigned char transmitIsBusy2 = 0;
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

   /* Install serial 0 handler */
   isr_setinterruptvector(4,&serial0_handler);

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

void serial0_shutdown(void)
{
   ES0 = 0; //disable serial interrupt
   serial0_flush();
}

void serial0_break(uchar dobreak)
{
   if (dobreak == 0)
     P3 |= 0x02;   // Release break condition, P3.1 high
   else
     P3 &= 0xFD;   // Commence break condition, P3.1 low
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

   /* Install serial 1 handler */
   isr_setinterruptvector(7,&serial1_handler);

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

void serial1_shutdown(void)
{
   ES1 = 0; //disable serial interrupt
   serial1_flush();
}

void serial1_break(uchar dobreak)
{
   if (dobreak == 0)
     P1 |= 0x08;   // Release break condition, P1.3 high
   else
     P1 &= 0xF7;   // Commence break condition, P1.3 low
}

void serial2_init(uchar reload_value)
{

   ES2 = 0;      // Turn off serial 2 interrupts
   ET3 = 0;      // Turn off timer 3 interrupts

   // Timer 3 is an 8bit auto-reload counter
   // Baud rate doubler enable
   // Use crystal/4 (really whatever CD0 and CD1 are set to)
   // Clear overflow
   // Stop the timer
	T3CM = 0x32;

   // set 8 bit uart with variable baud from timer
   // enable receiver and clear RI and TI
   SCON2 = 0x50;
  
   // Set timer 3 reload value
   TL3 = TH3 = reload_value;

   T3CM |= 0x40; // start timer 3

#if USE_SERIAL_INTERRUPTS
   receiveHead2 = receiveTail2 = 0;
   transmitIsBusy2 = 0;

   SCON2 &= 0xFD;  // Make sure transmit interrupt is clear TI2=0

   /* Install serial 2 handler */
   isr_setinterruptvector(10,&serial2_handler);

   EA = 1; // global interrupt enable
   ES2 = 1; // enable serial channel 2 interrupt
#else
   SCON2 |= 0x02;  // transmit buffer empty TI2=1
#endif
}

void serial2_flush(void)
{
   ES2 = 0;   // Turn off serial 2 interrupts
   SCON2 &= 0xFE;  // Make sure receive interrupt is clear RI2=0
#if USE_SERIAL_INTERRUPTS
	receiveHead2 = receiveTail2 = 0;
	transmitIsBusy2 = 0;
   SCON2 &= 0xFD;  // Make sure transmit interrupt is clear TI2=0
   ES2 = 1;  // Turn on serial 2 interrupts
#else
   SCON2 |= 0x02;  // transmit buffer empty TI2=1
#endif
}

void serial2_shutdown(void)
{
   ES2 = 0; //disable serial interrupt
   serial2_flush();
}

void serial2_break(uchar dobreak)
{
   if (dobreak == 0)
     P6 |= 0x80;   // Release break condition, P6.7 high
   else
     P6 &= 0x7F;   // Commence break condition, P6.7 low
}

#if USE_SERIAL_INTERRUPTS

void serial0_handler (void) interrupt 4 // IVT offset 0x23
{
	if (RI)
	{
		receiveBuffer0[receiveHead0++] = SBUF0;
		if (receiveHead0==receiveTail0) /* buffer overrun, sorry :) */
			receiveTail0 += 1;
		RI = 0;
	}
	if (TI)
	{
   		TI = 0;
		transmitIsBusy0 = 0;
	}
}

void serial0_putchar(char c)
{
	while (transmitIsBusy0)
		/*no-op*/;
	transmitIsBusy0 = 1;
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

void serial1_handler (void) interrupt 7 // IVT offset 0x3B
{
	if (RI1)
	{
		receiveBuffer1[receiveHead1++] = SBUF1;
		if (receiveHead1==receiveTail1) /* buffer overrun, sorry :) */
			receiveTail1++;
		RI1 = 0;
	}
	if (TI1)
	{
   		TI1 = 0;
		transmitIsBusy1 = 0;
	}
}

void serial1_putchar(char c)
{
	while (transmitIsBusy1)
		/*no-op*/;
	transmitIsBusy1 = 1;
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

void serial2_handler (void) interrupt 10 // IVT offset 0x53
{
	if (SCON2 & 0x01)
	{
		receiveBuffer2[receiveHead2++] = SBUF2;
		if (receiveHead2==receiveTail2) /* buffer overrun, sorry :) */
			receiveTail2++;
		/* Clear RI2 */
		SCON2 &= ~(0x01);
	}
	if (SCON2 & 0x02)
	{
	   /* Clear TI2 */
	   SCON2 &= ~(0x02);
       transmitIsBusy2 = 0;
	}
}

void serial2_putchar(char c)
{
   while (transmitIsBusy2)
      /*no-op*/;
   transmitIsBusy2 = 1;
   SBUF2 = c;
}

char serial2_getchar(void)
{
   char c;
   while (receiveHead2==receiveTail2)
     /*no-op*/;
   c = receiveBuffer2[receiveTail2];
   ES2 = 0; /* disable serial interrupts */
   receiveTail2++;
   ES2 = 1; /* enable serial interrupts */
   return c;
}

char serial2_peek(void)
{
   if(receiveHead2==receiveTail2)
      return FALSE;
   else
      return TRUE;
}

#else //ifdef USE_SERIALINTERRUPTS

void serial0_handler (void) interrupt 4 // IVT offset 0x23
{
	ES0 = 0; // disable serial interrupts
}

void serial0_putchar(char c)
{
	while (!TI)
		/*no-op*/;
   	TI = 0;
	SBUF0 = c;
}

char serial0_getchar(void)
{
   char c;
	while (!RI)
		/*no-op*/;
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

void serial1_handler (void) interrupt 7 // IVT offset 0x3B
{
	ES1 = 0; // disable serial interrupts
}

void serial1_putchar (char c)
{
   while (!TI1)
      /*no-op*/;
   	TI1 = 0;
   SBUF1 = c;
}

char serial1_getchar (void)
{
   char c;
	while (!RI1)
		/*no-op*/;
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

void serial2_handler (void) interrupt 10 // IVT offset 0x53
{
   ES2 = 0; /* disable serial interrupts */
}

void serial2_putchar (char c)
{
   while (!(SCON2 & 0x02))
      /*no-op*/;
   SCON2 &= ~(0x02);   /* Clear TI2 */
   SBUF2 = c;
}

char serial2_getchar (void)
{
   char c;
   while (!(SCON2 & 0x01))
   /*no-op*/;
   c = SBUF2;
   SCON2 &= ~(0x01);   /* Clear RI2 */
   return c;
}

char serial2_peek(void)
{
   if(SCON2 & 0x01)
      return TRUE;
   else
      return FALSE;
}

#endif // ifdef USE_SERIALINTERRUPTS

/*
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
#endif*/


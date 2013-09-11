//---------Copyright (C) 1997 Dallas Semiconductor Corporation--------------
//
//  COMUT.C - This source file contains the COM specific utility funcs
//            for 16 bit COM communication.
//
//  Compiler: VC
//  Version:
//

// includes
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "dos.h"
#include "ownet.h"
#include "ds2480.h"

// local function prototypes
SMALLINT OpenCOM(int, char *);
SMALLINT SetupCOM(int, ulong);
void     FlushCOM(int);
void     CloseCOM(int);
int      WriteCOM(int, int, uchar *);
int      ReadCOM(int, int, uchar *);
void     BreakCOM(int);
void     SetBaudCOM(int, uchar);
void PowerCOM(int, short);
//IBAPI RTSCOM(short, short);
//void StdFunc WaitWriteCOM(short);
void     MarkTime(void);
SMALLINT ElapsedTime(long);
//IBAPI DSRHigh(short);
void     msDelay(int);
void     Sleep(long);
long     msGettick(void);


// serial interrupt service routines
void __interrupt __far  com_serial_int4(ushort,ushort,ushort,ushort,ushort,ushort,ushort,ushort,ushort,
                                                          ushort,ushort,ushort,ushort);
void __interrupt __far  com_serial_int3(ushort,ushort,ushort,ushort,ushort,ushort,ushort,ushort,ushort,
                                                          ushort,ushort,ushort,ushort);

// globals to be used in communication routines
#define INBUF_MASK  63    // mask to and with current pointer
#define INBUFSIZ    64    // number of chars in buffer, 64
// defines to make Microsoft C compatible
#define inportb   inp
#define outportb  outp

static ulong TimeStamp;
static ulong BigTimeStamp;
static ulong MilliSec=2386;
static HANDLE ComID[MAX_PORTNUM];
static SMALLINT PortState_init = FALSE;


// structure to reference 8250 registers
typedef struct
{
  union
  {
     int com_regs[7];
     struct
     {
          union
          {
               ushort lsb;    // least significant byte of divisor
               ushort rbr;    // receive buffer register
               ushort tbr;    // transmit buffer register
          }  a;
          union
          {
               ushort msb;    // most significant byte of divisor
               ushort ier;    // interrupt enable register
          }  b;
          ushort iir;       // interrupt id register
          ushort lcr;       // line control register
          ushort mcr;       // modem control register
          ushort lsr;       // line status register
          ushort msr;       // modem status register
     } nam;
  };
} comtype;

// structure to hold the state of the port
typedef struct
{
   ushort com_vec_n;        // vector number com jumps through
   ushort m8259_mask;       // 8259 interrupt request mask
   ushort port_adr;         // address of port.  3f8 or 2f8
   uchar  inbuf[INBUFSIZ];  // input buffer
   ushort inbuf_n;          // number of chars in buffer
   ushort inbuf_get;        // point to beginning of buffer
   ushort inbuf_put;        // next incoming char goes here
   void (_interrupt _far *save_vec)();
   comtype com;

} PortStateType;

volatile PortStateType PortState[16];


// last port written/read from
volatile short LastPrt=0;

//---------------------------------------------------------------------------
// Attempt to open a com port.  Keep the handle in ComID.
// Set the starting baud rate to 9600.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number provided will
//               be used to indicate the port number desired when calling
//               all other functions in this library.
//
//
// Returns: the port number if it was succesful otherwise -1
//
int OpenCOMEx(char *port_zstr)
{
   int portnum;

   if(!PortState_init)
   {
      int i;
      for(i=0; i<MAX_PORTNUM; i++)
         PortState[i].port_adr = 0;
      PortState_init = TRUE;
   }

   // check to find first available handle slot
   for(portnum = 0; portnum<MAX_PORTNUM; portnum++)
   {
      if(!ComID[portnum])
         break;
   }

   OWASSERT( portnum<MAX_PORTNUM, OWERROR_PORTNUM_ERROR, -1 );

   if(!OpenCOM(portnum, port_zstr))
   {
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------
//  Description:
//     Attept to open a com port.
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  port_zstr  - the string of the port to be used
//
//     Returns FALSE if unsuccessful and TRUE if successful.
//
SMALLINT OpenCOM(int portnum, char *port_zstr)
{
   ushort i,port_val;
   ushort far *ptr = (ushort far *) 0x00400000;
   ushort Prt = atoi(&port_zstr[3]);
   uchar buff[5];

   if(!PortState_init)
   {
      for(i=0; i<MAX_PORTNUM; i++)
         PortState[i].port_adr = 0;
      PortState_init = 1;
   }

   OWASSERT( portnum<MAX_PORTNUM && portnum>=0 && !PortState[portnum].port_adr,
             OWERROR_PORTNUM_ERROR, FALSE );

   // reset the state of this port
   PortState[portnum].port_adr = 0;
   PortState[portnum].inbuf_n = 0;
   PortState[portnum].inbuf_get = 0;
   PortState[portnum].inbuf_put = 0;

   // check if already setup
   if (PortState[portnum].port_adr != 0)
      return SetupCOM(portnum,9600);

   port_val = *(ptr + Prt - 1);    // get the port base

   if (port_val == 0x3f8 || port_val == 0x3e8) // COM1 or COM3
   {
       PortState[portnum].com_vec_n  = 0x0c;            // jump thru vectory C
       PortState[portnum].m8259_mask = 0x0ef;           // mask bit 4
   }
   else if (port_val == 0x2f8 || port_val == 0x2e8)  // COM2 or COM4
   {
       PortState[portnum].com_vec_n  = 0x0b;            // jump thru vectory B
       PortState[portnum].m8259_mask = 0x0f7;           // mask bit 3
   }
   else                             // must have found a non standard base
       return FALSE;

   PortState[portnum].port_adr = port_val;             // global assignments

   for (i = 0; i < 7; ++i)          // set the port values of the registers
      PortState[portnum].com.com_regs[i] = PortState[portnum].port_adr + i;

   PortState[portnum].save_vec = _dos_getvect(PortState[portnum].com_vec_n);  // save the current ISR vector

   // setup ISR depending on interrupt
   if (PortState[portnum].com_vec_n  == 0x0c)
   {
      printf("setting up com1 interupt.\n");
      _dos_setvect(PortState[portnum].com_vec_n, com_serial_int4);
   }
   else
      _dos_setvect(PortState[portnum].com_vec_n, com_serial_int3);

   _disable();         // disable interrupts while setting up ISR

   // clear dlab in LCR
   i = inportb(PortState[portnum].com.nam.lcr);
   outportb(PortState[portnum].com.nam.lcr,(uchar)(0x7F & i));

   // set rts and dtr in MCR
   outportb(PortState[portnum].com.nam.mcr,0x0B);

   // interrupt on received data available in IER
   outportb(PortState[portnum].com.nam.b.ier,0x01);

   // clear bit (3 or 4) so it interrupts unmasked
   i = inportb(0x21);
   outportb(0x21,(uchar)(PortState[portnum].m8259_mask & i));

   // read in what is there
   i = inportb(PortState[portnum].com.nam.a.rbr);

   // set the baudrate
   SetupCOM(portnum,9600);

   // clear the interrupt
   inportb(PortState[LastPrt].com.nam.iir);

   // reset the buffer counters
   PortState[portnum].inbuf_n   = 0;
   PortState[portnum].inbuf_get = 0;
   PortState[portnum].inbuf_put = 0;
   LastPrt = portnum;

   _enable();         // re-enble interrupts

   // make sure the device is powered
   outportb(PortState[portnum].com.nam.mcr,0x0B);

   // reset byte operation
   FlushCOM(portnum);
   buff[0] = (uchar) (CMD_COMM | FUNCTSEL_RESET | 0);
   if(!WriteCOM(portnum,1,buff))
      return FALSE;
   Sleep(5);
   FlushCOM(portnum);

   return TRUE;         // return success
}


//-------------------------------------------------------------------
//  Description:
//     Closes the connection to the port.
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
//     Returns FALSE if unsuccessful and TRUE if successful.
//
void CloseCOM(int portnum)
{
   // clear port address (use as flag)
   PortState[portnum].port_adr = 0;

   // restore previous interrupt
   _dos_setvect(PortState[portnum].com_vec_n, PortState[portnum].save_vec);

   _disable();                    // disable interrupts

   // set bit (3 or 4) so it interrupts unmasked
   outportb(0x21,(uchar)(~PortState[portnum].m8259_mask | inportb(0x21)));

   // clear dlab in LCR
   outportb(PortState[portnum].com.nam.lcr, (0x7f & inportb(PortState[portnum].com.nam.lcr)));

   // clear interrupt on receive in IER
   outportb(PortState[portnum].com.nam.b.ier, 0);

   // rts and out2 off. leave dtr alone in MCR
   //????????outportb(PortState[Prt].com.nam.mcr, (0x01 & inport(PortState[Prt].com.nam.mcr)));

   _enable();                     // re-enable interrupts

}


//-------------------------------------------------------------------
// Read an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set and the buffers have
// been flushed.
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  inlen      - the length of the data that was read
//  outbuf     - the input data
//
// Returns number of characters read
//
int ReadCOM(int portnum, int inlen, uchar *inbuf)
{
   short cnt=0;
   ulong far *sysclk = (ulong far *) 0x0040006c;
   ulong M;
   ulong more;

   // get a time limit
   M = *sysclk + 3;

   // remember this as the last port
   LastPrt = portnum;

   // loop to get all of the characters or timeout
   do
   {
      if (PortState[portnum].inbuf_n)
      {
         inbuf[cnt++] = PortState[portnum].inbuf[PortState[portnum].inbuf_get++];   // got it
         PortState[portnum].inbuf_get &= INBUF_MASK;   // wrap around
         --PortState[portnum].inbuf_n;                 // one less char in buffer
      }
   }
   while ((cnt < inlen) && (M > *sysclk));

   // check for more
   more = PortState[portnum].inbuf_n;

   return cnt;
}


//-------------------------------------------------------------------
// Write an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set and the buffers have
// been flushed.
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  outlen     - the length of the data to be written
//  outbuf     - the output data
//
// Returns TRUE for success and FALSE for failure
//
int WriteCOM(int portnum, int outlen, uchar *outbuf)
{
   short cnt=0;

   // remember this as the last port
   LastPrt = portnum;

   // loop to write out each byte an wait until sent
   while (cnt < outlen)
   {

      // check if output buffer empty
      if (inportb(PortState[portnum].com.nam.lsr) & 0x20)
      {
         // make sure interrupts are enabled
         _enable();

         // send out a character
         outportb(PortState[portnum].com.nam.a.tbr,outbuf[cnt++]);
      }
   }

   return outlen;
}


//------------------------------------------------------------------
//  Description:
//     This routines sets up the DCB and performs a SetCommState().
//     SetupCOM assumes OpenComm has previously been called.
//     Returns 0 if unsuccessful and 1 if successful.
//
//  Prt      - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//  baudrate - the baudrate that should be used in setting up the port.
//
//  return TRUE if the port was setup correctly
//
SMALLINT SetupCOM(int portnum, ulong baudrate)
{
   ushort reload;

   // set dlab to load reload of 8250 in LCR
   outportb(PortState[portnum].com.nam.lcr, 0x80);

   // calculat the reload
   switch (baudrate)
   {
      case 9600:   reload = 12;
         break;
      case 19200:  reload = 6;
         break;
      case 57600:  reload = 2;
         break;
      case 115200: reload = 1;
         break;
      default: reload = 12;
         break;
   };

   // load msb of Reload
   outportb(PortState[portnum].com.nam.b.msb, reload / 256);

   // load lsb of Reload
   outportb(PortState[portnum].com.nam.a.lsb, reload & 0xFF);

   // parity off, stop=1, word=8 in LCR
   outportb(PortState[portnum].com.nam.lcr, 3);
   //?????outportb(PortState[Prt].com.nam.lcr, 0x07); // 2 stop bits for test

   // enable 16550 hokie1

   outportb(PortState[portnum].com.nam.iir,0xC7);

   // disable 16550 fifos
   // commented out because you can do the hardware enable
   // or this option chooses software fifo.
//   outportb(PortState[portnum].com.nam.iir,0x00);

   return TRUE;
}


//-------------------------------------------------------------------
//  Description:
//     flush the rx and tx buffers
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
void FlushCOM(int portnum)
{
   // loop to flush the send buffers
   while ((inportb(PortState[portnum].com.nam.lsr) & 0x60) != 0x60)
      ;

   // read any bytes in input buffer
   while (inportb(PortState[LastPrt].com.nam.lsr) & 0x01)
      inportb(PortState[LastPrt].com.nam.a.rbr);

   // clear the interrupt
   inportb(PortState[LastPrt].com.nam.iir);

   // non-specific return from interrupt
   outportb(0x20,0x20);

   _disable();         // disable interrupts

   // reset the buffer counters
   PortState[portnum].inbuf_n   = 0;
   PortState[portnum].inbuf_get = 0;
   PortState[portnum].inbuf_put = 0;

   _enable();         // re-enble interrupts
}


//--------------------------------------------------------------------------
//  ISR for com port receive.
//
void __interrupt __far  com_serial_int4(ushort _es, ushort _ds, ushort _di, ushort _si,
                                   ushort _bp, ushort _sp, ushort _bx, ushort _dx,
                                   ushort _cx, ushort _ax, ushort _ip, ushort _cs,
                                   ushort _flags)
{
   _disable();                    // disable interrupts

   // get uart status
   if (inportb(PortState[LastPrt].com.nam.lsr) & 0x01)
   {
      // get the character
      PortState[LastPrt].inbuf[PortState[LastPrt].inbuf_put++] = inportb(PortState[LastPrt].com.nam.a.rbr);
      // wrap buffer
      PortState[LastPrt].inbuf_put &= INBUF_MASK;
      // count the new character
      PortState[LastPrt].inbuf_n++;
   }

   // clear the interrupt
   inportb(PortState[LastPrt].com.nam.iir);

   // non-specific return from interrupt
   outportb(0x20,0x20);

   _enable();                     // re-enable interrupts
}

//--------------------------------------------------------------------------
//  ISR for com port receive.
//
void __interrupt __far  com_serial_int3(ushort _es, ushort _ds, ushort _di, ushort _si,
                                   ushort _bp, ushort _sp, ushort _bx, ushort _dx,
                                   ushort _cx, ushort _ax, ushort _ip, ushort _cs,
                                   ushort _flags)
{
   _disable();                    // disable interrupts

   // get uart status
   if (inportb(PortState[LastPrt].com.nam.lsr) & 0x01)
   {
      // get the character
      PortState[LastPrt].inbuf[PortState[LastPrt].inbuf_put++] = inportb(PortState[LastPrt].com.nam.a.rbr);
      // wrap buffer
      PortState[LastPrt].inbuf_put &= INBUF_MASK;
      // count the new character
      PortState[LastPrt].inbuf_n++;
   }

   // clear the interrupt
   inportb(PortState[LastPrt].com.nam.iir);

   // non-specific return from interrupt
   outportb(0x20,0x20);

   _enable();                     // re-enable interrupts
}

//-------------------------------------------------------------------
//  Description:
//     Send a break on the com port for "len" msec
//
//  portnum    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
void BreakCOM(int portnum)
{
   // start the break
   outportb(PortState[portnum].com.nam.lcr, 0x43);

   Sleep(2);

   // clear the break
   outportb(PortState[portnum].com.nam.lcr, 0x03);
}

//--------------------------------------------------------------------------
// Set the baud rate on the com port.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number was provided to
//               OpenCOM to indicate the port number.
// 'new_baud'  - new baud rate defined as
//                PARMSET_9600     0x00
//                PARMSET_19200    0x02
//                PARMSET_57600    0x04
//                PARMSET_115200   0x06
//
void SetBaudCOM(int portnum, uchar new_baud)
{
   ushort reload;

   // set dlab to load reload of 8250 in LCR
   outportb(PortState[portnum].com.nam.lcr, 0x80);

   // calculate the reload
   // does not support PARMSET_57600
   // and PARMSET_115200
   switch(new_baud)
   {
      case PARMSET_9600:   reload = 12;
         break;
      case PARMSET_19200:  reload = 6;
         break;
      case PARMSET_57600:  reload = 2;
         break;
      case PARMSET_115200: reload = 1;
         break;
      default: reload = 12;
         break;
   };

   // load msb of Reload
   outportb(PortState[portnum].com.nam.b.msb, reload / 256);

   // load lsb of Reload
   outportb(PortState[portnum].com.nam.a.lsb, reload & 0xFF);

   // parity off, stop=1, word=8 in LCR
   outportb(PortState[portnum].com.nam.lcr, 3);
}

//--------------------------------------------------------------------------
//  Description:
//     Drop and then raise power (DTR,RTS) on the com port
//
//  len > 0 - drop power for len ms
//      = 0 - reset power drop
//      < 0 - drop power for an infinite time
//
void PowerCOM(int portnum, short len)
{
   // drop power?
   if (len != 0)
      // DTR/RTS off
      outportb(PortState[portnum].com.nam.mcr,0);

   if (len > 0)
      // sleep
      Sleep(len);

   // is this not an infinite power drop?
   if (len >= 0)
      // DTR/RTS on
      outportb(PortState[portnum].com.nam.mcr,0x0B);
}


//--------------------------------------------------------------------------
// Mark time stamp for later comparison
//
void MarkTime(void)
{
   // get the rought timestamp
   BigTimeStamp = msGettick();

   // get an accurate timestamp
   outportb(0x43,0);                // freeze value in timer
   TimeStamp = inportb(0x40) & 0xFF;        // read value in timer
   TimeStamp += (inportb(0x40) << 8) & 0xFF00;
}


//--------------------------------------------------------------------------
// Check if timelimit number of ms have elapse from the call to MarkTime
//
// timelimit  - the time limit for the elapsed time
//
// Return TRUE if the time has elapse else FALSE.
//
SMALLINT ElapsedTime(long timelimit)
{
   ulong C,N;

   // check if using rought or accurate timestamp
   if (timelimit > 5)
   {
      // check if time elapsed
      return ((BigTimeStamp + timelimit) < (ulong) msGettick());
   }
   else
   {
      outportb(0x43,0);                      // freese value in timer
      N = inportb(0x40) & 0xFF;              // read value in timer
      N += (inportb(0x40) << 8) & 0xFF00;

      if (TimeStamp < N)
         C = 65536 - (N - TimeStamp);
      else
         C = TimeStamp - N;

      // check if time elapsed
      return ((timelimit * MilliSec) < C);
   }
}


//--------------------------------------------------------------------------
// Delays for the given amout of time
//
// len  - the number of milliseconds to sleep
//
void msDelay(int len)
{
   Sleep(len);
}

//--------------------------------------------------------------------------
//  Wait duration in ms
//
void Sleep(long len)
{
   // get a time stamp
   MarkTime();
   // wait for len ms
   while (!ElapsedTime(len));
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   ulong *sysclk = (ulong *) 0x0040006c;

   return *sysclk * 55;
}
//---------------------------------------------------------------------------
// Copyright (C) 2001 Dallas Semiconductor Corporation, All Rights Reserved.
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
//     Module Name: JibComm
//
//     Description: Low-level Java-Powered iButton Communication
//
//        Filename: jib96o.c
//
// Dependant Files: jib96.h
//
//         Version: 1.00 (based on cibcomm.c Ver 2.206)
//
//

#include "ownet.h"
#include "jib96.h"

// defines
#define STREAM_OVERHEAD 4
#define ME_RUNTIME      12
#define SLEEP_PAD       6

// external low-level 1-Wire functions
extern SMALLINT owTouchBit(int,SMALLINT);
extern SMALLINT owTouchByte(int,SMALLINT);
extern SMALLINT owWriteByte(int,SMALLINT);
extern SMALLINT owReadBitPower(int, SMALLINT);
extern SMALLINT owAccess(int);
extern SMALLINT owBlock(int,SMALLINT,uchar *,SMALLINT);
extern void     msDelay(int);
extern SMALLINT owLevel(int,SMALLINT);

// exportable functions
//ushort SendCiBMessage(int, uchar *, ushort, uchar);
//ushort RecvCiBMessage(int, uchar *, ushort *);
//void SetMinRunTime(ushort);

// local functions
static uchar SendSegment(int, uchar *, uchar *, ushort, ushort *);
static uchar SendData(int, uchar *, uchar, ushort *);
static uchar SendHeader(int, uchar, uchar *, ushort, uchar, ushort *);
static uchar GetStatus(int, uchar *, uchar *, uchar *, uchar *);
static uchar SendStatus(int, uchar);
static uchar SendRun(int);
static uchar SendReset(int);
static uchar SendInterrupt(int);
static uchar RecvHeader(int, uchar *, ushort *, ushort *, uchar *, ushort *);
static uchar RecvData(int, uchar *, uchar, ushort, ushort *, uchar);
static uchar ReleaseSequence(int, ushort);
static uchar CheckCRC(int, ushort);
static uchar CheckStreamCRC(uchar *, int, ushort);
static void TimeDelay(int, uchar);
static ushort CRC16TGen(uchar, ushort);
static void GenCRC16Table();
static ushort CRC16(uchar, ushort);
static ushort CRC16Buf(uchar *, ushort, ushort);
static void GenCRC16CorrectionTable();
static ushort GetCRC16Correction(uchar);

// globals
static ushort MinRun = 96;
static int StreamLen = 0;
static uchar StreamBuff[256+STREAM_OVERHEAD];
static uchar LastSegment = FALSE;
static ushort CRCTable[256];
static ushort CRCCorrectionTable[140];
static uchar PowerOn = FALSE;
static uchar didsetup = FALSE;

//--------------------------------------------------------------------------
//
// Top-Level function to send a message to the CiB
//
//      portnum   = port number the device is on
//      mp        = Pointer to message string
//      mlen      = Length of message (bytes)
//      ExecTime  = Estimated execution time (0-15, where 0 = minimum)
//
//      return    = Error code, or zero if all went well
//
ushort SendCiBMessage(int portnum,
                        uchar *mp,
                        ushort mlen,
                        uchar ExecTime)
{
  uchar ETime;            // Temp value for ExecTime
  uchar Retry;            // Retry counter
  uchar seg;              // Current segment number
  uchar InFree;           // Inbound FIFO bytes available
  uchar OutUsed;          // Outbound FIFO bytes used
  uchar owms;             // micro status
  uchar cpst;             // Co-Processor status
  ushort checksum = 0;  // checksum accumulator
  ushort sum = 0;       // Temporary checksum store
  uchar error;

  // check for setup
  if (!didsetup)
  {
     didsetup = TRUE;
     GenCRC16Table();
     GenCRC16CorrectionTable();
  }

  seg = 127;          // Start with no segment number done

  ExecTime &= 0xFF;
  ETime = (uchar) ExecTime;

  // Set-up a Retry loop
  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      // Get the status from the device
      if((error = GetStatus(portnum, &InFree, &OutUsed, &owms, &cpst)) != 0)
        break;

      // See if a POR has occurred and treat it as an error if so
      if(owms & CE_BPOR)
      {
        error = ER_POR;
        break;
      }

      // See if the co-processor was running when we stopped
      if(cpst & 1)
      {
        // Send the time value to the CIB
        if((error = SendStatus(portnum, ETime)) != 0)
          break;

        // Execute the co-processor and allow it to finish up
        if((error = SendRun(portnum)) != 0)
          break;

        TimeDelay(portnum, (uchar) (ETime+SLEEP_PAD));
      }
      else
      {
        // If the co-processor is not running, then

        // Work around for 0 owms
        if(!(owms & 0x7f) && !OutUsed)
          owms = 0x3F;

        // See if the previous command has not yet been completed
        if((owms & 0x20) ||
           ((owms & 0x1F) == CE_FIRSTBIRTH) ||
           (owms == 0x1F))
        {

           // If Operation does not finish double previous time until 8
           // then increment by one until max time (15 = 3.75 s +64 ms) 9/25 CIM

            if (ETime == 0)
               ETime++;
            else if (ETime < 8)
               ETime <<=1;
            else if ((ETime >= 8) && (ETime < 15))
               ETime++;
            else
              ETime = 15;

          // if we don't allow a First Birthday (FB) to complete
          // we can end up in an infinite POR loop - FBs
          // currently take on the order of 1.5 Seconds

          if((owms & 0x1F) == CE_FIRSTBIRTH)
          {
            // Send the time value to the CIB
            if((error = SendStatus(portnum, ME_RUNTIME)) != 0)
              break;
          }
          else
          {
            // Send the time value to the CIB
            if((error = SendStatus(portnum, ETime)) != 0)
              break;
          }

          // Execute the micro and allow it to finish up
          if((error = SendRun(portnum)) != 0)
            break;

          if((owms & 0x1F) == CE_FIRSTBIRTH)
          {
            TimeDelay(portnum, ME_RUNTIME);
            TimeDelay(portnum, 1);
          }
          else
          {
            // Wait for whatever we did above to finish up
            // don't forget the extra pad for the coprocessor alarm
            TimeDelay(portnum, (uchar)(ETime+SLEEP_PAD));
          }

        }
        else
        {
          // See if the device has room for new command. Error if not
          if(InFree < HEADERSIZE)
          {
            error = ER_NOROOM;
            break;
          }

          // If the segment we're about to send is not the FIRST one,
          // check the status to see if we had any comm errors on the last
          // (Errors left over from prior uses don't concern us!)

          if(seg != 127)
          {
            if(((owms & 31) > 1) & ((owms & 31) < 8))
            {
              error = (uchar)((owms & 31) | ER_COMMERROR);
              break;
            }
          }

          // If the segment number MS bit = 1, then we're done
          if(seg & 128)
          {
            // Successful exit
            return 0;
          }

          // When we have finished a segment, go to the next segment
          seg++;
          seg = (uchar)(seg & 127);

          //If about to send First segment, clear checksum accumulator
          checksum = seg ? checksum + sum : 0;

          // For each new segment that we attempt to send out, we start a
          // temporary checksum in 'sum' with the current checksum value.
          // If the segment transfer fails, we will not have added
          // anything to the actual checksum so we can try again

          sum = checksum;

          // Send out the next segment
          if((error = SendSegment(portnum, &seg, mp, mlen, &sum)) != 0)
            break;

          // Now we have loaded the data segment into IPR register and
          // the header for it into the FIFO. Next set the execution time
          // if it's the last message segment, and the execute the message

          if(seg & 128)
          {
            ETime = (uchar) ExecTime;

            if((error = SendStatus(portnum, ETime)) != 0)
                break;
          }
          else
            ETime = 0;

          // Send the run-interrupt command to the device
          if((error = SendInterrupt(portnum)) != 0)
             break;
        }

        TimeDelay(portnum, ETime);
      }

    } // End of forever loop

    // Save the error cause
    ETime = error;

    // If we had an error and broke out of the loop, reset the micro
    if((error = SendReset(portnum)) != 0)
      break;

    // Send the time value to the CIB
    if((error = SendStatus(portnum, (uchar)(Retry? (Retry<<1): 1))) != 0)
      break;

    // Then allow it to run and report a completion status
    if((error = SendRun(portnum)) != 0)
      break;

    // See if a little run time will cure the problem that caused the reset
    TimeDelay(portnum, (uchar)((Retry? (Retry<<1): 1)+SLEEP_PAD));

    // We always begin again at the start (segment 0) after an error
    seg = 127;

    // Restore the original error cause code
    error = ETime;

  } //End of Retry loop

  // Error-return if we failed all the retries
  return error;
}

//--------------------------------------------------------------------------
//
// Send out one (specific) segment of the message to the device
//
//      portnum = port number the device is on
//      pseg    = Pointer to current segment number (0 = First)
//      mp      = Pointer to message string
//      msglen  = Length of message string (bytes)
//      psum    = Pointer to checksum accumulator
//
//      return  =   Error code, or zero if all went well
//
uchar SendSegment(int portnum, uchar* pseg, uchar* mp, ushort msglen, ushort* psum)
{
  uchar     seglen;      // Length of segment we're sending
  ushort    indexx;       // Offset of segment we're sending
  uchar     remain;      // Bytes remaining before this seg
  uchar*    segstart;    // Start of segment in message
  uchar     error;

  // Offset of next segment to send
  indexx = (ushort)(*pseg * SEGSIZE);

  // Computer how much data remains unsent to the device
  remain = (uchar)(msglen - indexx);      // Length of segment remaining send

  // Limit the segment size to that which is allowed in this device
  if(remain > SEGSIZE)
  {
    // If remain is bigger than max, send max
    seglen = SEGSIZE;
  }
  else
  {
    // If remain will fit, wrap it up
    seglen = remain;
    // Flag this as the last segment
    *pseg |= 128;
  }

  // Define the point in the message where this segment begins
  segstart = mp + indexx;

  // Send out the segment data to the device
  if((error = SendData(portnum, segstart, seglen, psum)) != 0)
    return error;

  // Send out the header to the device
  if((error = SendHeader(portnum, *pseg, segstart, remain, seglen, psum)) != 0)
    return error;

  return 0;
}

//--------------------------------------------------------------------------
//
// Send out the data portion of a message segment to the device
//
//      portnum   =   port number the device is on
//      mp        =   Pointer to segment in the message string
//      seglen    =   length of this segment
//      psum      =   Pointer to checksum accumulator
//
//      return    =   Error code, or zero if all went well
//
uchar SendData(int portnum, uchar* mp, uchar seglen, ushort* psum)
{
  uchar  Retry,
         error;
  uchar  i;
  ushort crc;

  // Now try (and Retry) to output the data without error
  for (Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      // Send out the data segment to the device and compute the crc
      // and checksum as it goes out
      StreamLen = 0;

      StreamBuff[StreamLen++] = OW_IPRWRITE;
      StreamBuff[StreamLen++] = seglen;

      for(i = 0; i < seglen; i++)
        StreamBuff[StreamLen++] = mp[i];

      // set up the crc bytes

      // CHECK FOR BLIVET

      StreamBuff[StreamLen++] = 0xff;
      StreamBuff[StreamLen++] = 0xff;

      if(!owAccess(portnum))
      {
         error = ER_ACCESS;
         break;
      }

      if(!owBlock(portnum, FALSE, StreamBuff, StreamLen))
      {
        error = ER_ACCESS;
        break;
      }

      crc = CRC16Buf(StreamBuff, 0, (ushort)(StreamLen-2));

      // Check the returning CRC from the device
      if((error = CheckStreamCRC(StreamBuff,StreamLen-2,crc)) != 0)
      {
        if((error = CheckStreamCRC(StreamBuff,StreamLen-2, (ushort)(crc ^ GetCRC16Correction(sizeof(crc))))) != 0)
          break;

        StreamBuff[2] &= 0x0FE;
      }

      for(i = 0; i < seglen; i++)
        mp[i] = StreamBuff[i+2];

      for(i = 0; i < seglen; i++)
        *psum += mp[i];

      // If all has gone well, return with AOK status
      return 0;
    }
  }

  // If we've retried and failed, return with failure status
  return (uchar)(error | ER_SENDDATA);
}

//--------------------------------------------------------------------------
//
// Send out the header portion of a message segment to the device
//
//      portnum  =   port number the device is on
//      seg      =   Segment number to send out (0-127)
//      mp       =   Pointer to segment in message string
//      remain   =   Number of bytes remaining, including this segment
//      psum     =   Pointer to checksum accumulator
//
//      return   =   Error code, or zero if all went well
//
uchar SendHeader(int portnum, uchar seg, uchar* mp, ushort remain, uchar seglen, ushort* psum)
{
  ushort    crc,
            segcrc;
  uchar     Retry,
            error;
  FIFORDWR  FB;
  ushort    i;

  // We must compute the data segment's CRC for the header
  // This includes the segment length value

  segcrc = CRC16(seglen, 0);
  segcrc = CRC16Buf(mp, segcrc, seglen);

  // Now try (and Retry) sending the header to the device
  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      if(!owAccess(portnum))
      {
        error = ER_ACCESS;
        break;
      }

      FB.Cmd = OW_FIFOWRITE;
      FB.Len = FIFO_LEN;

      FB.FIFOBuf[0] = seg;
      FB.FIFOBuf[1] = seglen;
      FB.FIFOBuf[2] = LSB(remain);
      FB.FIFOBuf[3] = MSB(remain);
      FB.FIFOBuf[4] = LSB(segcrc);
      FB.FIFOBuf[5] = MSB(segcrc);

      for(i = 0; i < 6; i++)
        *psum += FB.FIFOBuf[i];

      FB.FIFOBuf[6] = LSB(*psum);
      FB.FIFOBuf[7] = MSB(*psum);

      if(!owBlock(portnum, FALSE, &FB.Cmd, 10))
      {
         error = ER_ACCESS;
         break;
      }

      crc = CRC16Buf(&FB.Cmd, 0, sizeof(FB));

      //
      // Add the checksum bytes to the checksum for subsequent segments
      // (Uses segcrc because it's handy!)

      segcrc = *psum;
      *psum += LSB(segcrc);
      *psum += MSB(segcrc);

      // Check the returning CRC from the device
      if((error = CheckCRC(portnum, crc)) != 0)
        break;

      // Send the release sequence and check the ACK bit
      if((error = ReleaseSequence(portnum, RS_FIFOWRITE)) != 0)
        break;

      // If all went well, return with OK status
      return 0;
    }
  }

  // If we retried and failed, return with failure status
  return (uchar)(error | ER_SENDHDR);
}

//--------------------------------------------------------------------------
//
// Return the device status values -
//
//    portnum   =   port number the device is on
//    InFree    =   Pointer to status = Number of free bytes in FIFO
//    OutUsed   =   Pointer to status = Number of bytes waiting in FIFO
//    owms      =   Pointer to status = One-Wire Micro Status byte
//    cpst      =   Pointer to status = Co-Processor Status Byte
//
//    return    =   Error code, or zero if all went well
//
uchar GetStatus(int portnum, uchar* InFree, uchar* OutUsed, uchar* owms, uchar* cpst)
{
  uchar    IOBuf[5];
  uchar    Retry,
           error;
  ushort crc;
  int      i;

  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      StreamLen = 0;

      StreamBuff[StreamLen++] = OW_STATUSREAD;

      // CHECK FOR BLIVET

      for(i = 0; i < 6; i++) // 2 for CRC read
        StreamBuff[StreamLen++] = 0xff;

      if(!owAccess(portnum))
      {
         error = ER_ACCESS;
         break;
      }

      if(!owBlock(portnum, FALSE, StreamBuff, StreamLen))
      {
        error = ER_ACCESS;
        break;
      }

      crc = CRC16Buf(StreamBuff, 0, (ushort)(StreamLen-2));

      if((error = CheckStreamCRC(StreamBuff,StreamLen-2,crc)) != 0)
      {
        if(!(StreamBuff[1] & 1))
          break;

        StreamBuff[1] &= 0xFE;

        crc = CRC16Buf(StreamBuff, 0, (ushort)(StreamLen-2));

        if((error = CheckStreamCRC(StreamBuff,StreamLen-2,crc)) != 0)
          break;
      }

      for(i = 0; i < 5; i++)
        IOBuf[i] = StreamBuff[i];

      *InFree  = IOBuf[1];
      *OutUsed = IOBuf[2];
      *owms    = IOBuf[3];
      *cpst    = IOBuf[4];

      // If success, return an AOK status
      return 0;
    }
  }

  // If we've re-tried and failed, return a failure status
  return (uchar)(error | ER_GETSTATUS);
}

//--------------------------------------------------------------------------
//
//   Send the owus byte (timing value) to the device -
//
//      portnum     =   port number the device is on
//      owus        =   One-Wire Uart Status byte
//
//      return      =   Error code, or zero if all went well
//
uchar SendStatus(int portnum, uchar owus)
{
  uchar  Retry,
         error;
  ushort crc;

  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      if(!owAccess(portnum))
      {
        error = ER_ACCESS;
        break;
      }

      crc = 0;
      crc = CRC16((uchar)owTouchByte(portnum, OW_STATUSWRITE), crc);
      crc = CRC16((uchar)owTouchByte(portnum, owus), crc);


      if((error = CheckCRC(portnum, crc)) != 0)
        break;

      if((error = ReleaseSequence(portnum, RS_STATUSWRITE)) != 0)
        break;

      // If all is well, exit with OK status
      return 0;
    }
  }

  // If retries failed, return with failure status
  return (uchar)(error | ER_SENDSTATUS);
}

//--------------------------------------------------------------------------
//
// The SendRun function sends the micro-run command to the device
//
//      portnum   =   port number the device is on
//      return    =   Error code, or zero if all went well
//
uchar SendRun(int portnum)
{
  uchar  Retry,
         error;

  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      if(!owAccess(portnum))
      {
        error = ER_ACCESS;
        break;
      }

      if(!owWriteByte(portnum, OW_MICRORUN))
      {
         error = ER_ACCESS;
         break;
      }

      // Issue the release sequence code
      if((error = ReleaseSequence(portnum, RS_MICRORUN)) != 0)
        break;

      // If all went well, return with AOK status
      return 0;
    }
  }

  // If errors occurred return error status
  return (uchar)(error | ER_SENDRUN);
}

//--------------------------------------------------------------------------
//
// The SendReset function sends the micro-reset command to the device
//
//      portnum   =   port number the device is on
//      return    = Error code, or zero if all went well
//
uchar SendReset(int portnum)
{
  uchar Retry,
       error;

  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      if(!owAccess(portnum))
      {
        error = ER_ACCESS;
        break;
      }

      if(!owWriteByte(portnum, OW_MICRORESET))
      {
         error = ER_ACCESS;
         break;
      }

      if((error = ReleaseSequence(portnum, RS_MICRORESET)) != 0)
        break;

      // If all went well, return w/o error
      return 0;
    }
  }

  return (uchar)(error | ER_SENDRESET);
}

//--------------------------------------------------------------------------
//
// The SendInterrupt function sends the micro-interrupt command to the device
//
//      portnum   =   port number the device is on
//      return    =   Error code, or zero if all went well
//
uchar SendInterrupt(int portnum)
{
  uchar Retry,
        error;

  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      if(!owAccess(portnum))
      {
        error = ER_ACCESS;
        break;
      }

      if(!owWriteByte(portnum, OW_MICROINTERRUPT))
      {
         error = ER_ACCESS;
         break;
      }

      if((error = ReleaseSequence(portnum, RS_MICROINTERRUPT)) != 0)
        break;

      return 0;
    }
  }

  return (uchar)(error | ER_SENDINT);
}

//--------------------------------------------------------------------------
//
// Return Message Function -
//
//    portnum   =   port number the device is on
//    mp        =   Current Pointer to message buffer string
//    bufsize   =   Number of bytes in provided buffer space
//
//    return    =   Error code, or zero if all went well
//
ushort RecvCiBMessage(int portnum, uchar *mp, ushort *bufsize)
{
  uchar  ETime = 0;      // Temporary time value
  uchar  *mbuf;          // Start of message buffer
  uchar  InFree;         // Inbound FIFO bytes available
  uchar  OutUsed;        // Outbound FIFO bytes used
  uchar  owms;           // micro status
  uchar  cpst;           // Co-Processor status
  uchar  seg;            // Current segment number
  ushort hdrcrc;         // Crc from header
  ushort hdrsum;         // Checksum from header
  ushort checksum;       // Running checksum of the message
  ushort sum;            // Temporary checksum store
  uchar  size;           // Size of current segment
  ushort msglen;         // Length of message received so far
  uchar  oldseg;         // Prior segment number for checking order
  uchar  Retry,
         error;
  uchar  JavaiButton = 0;

  checksum = msglen = 0;
  // Remember where the buffer starts
  mbuf = mp;

  // Set up a loop to try (and Retry) getting a message from the device
  for(Retry = 0; Retry < 6; Retry++)
  {
    // When we Retry, Retry from the top
    oldseg = 0xFF;

    while(1)
    {
      //Get the device status and see if there is a segment there to read
      if((error = GetStatus(portnum, &InFree, &OutUsed, &owms, &cpst)) != 0)
        break;

      // See if a POR has occurred and treat it as an error if so
      if(owms & CE_BPOR)
      {
        error = ER_POR;
        break;
      }

      // See if the co-processor was running when we stopped
      if((cpst & 1) != 0)
      {
        // Send the time value to the CIB
        if((error = SendStatus(portnum, ETime)) != 0)
          break;

        // Execute the co-processor and allow it to finish up
        if((error = SendRun(portnum)) != 0)
          break;

        // Compute a delay time to complete
        ETime = (uchar)(cpst >> 4);

        // Wait for whatever we did above to finish up
        TimeDelay(portnum, (uchar)(ETime+SLEEP_PAD));
      }
      else
      {
        // If the co-processor is not running, then

        // See if the previous command has not yet been completed
        if(owms & 0x20)
        {
          //
          // Decide on a time value to complete the prior operation -
          // For now, we'll send back 1/2 of the status value as a time -

          ETime = (uchar)((owms & 0x1F) >> 1);

          if(ETime == 0)
            ETime++;

          //
          // if we don't allow a First Birthday (FB) to complete
          // we can end up in an infinite POR loop - FBs
          // currently take on the order of 1.5 Seconds

          if((owms & 0x1F) == CE_FIRSTBIRTH)
          {
            // Send the time value to the CIB
            if((error = SendStatus(portnum, ME_RUNTIME)) != 0)
              break;
          }
          else
          {
            // Send the time value to the CIB
            if((error = SendStatus(portnum, ETime)) != 0)
              break;
          }

          // Execute the micro and allow it to finish up
          if((error = SendRun(portnum)) != 0)
            break;

          //
          // wait an extra 1.5 seconds to compensate for first birthday.

          if((owms & 0x1F) == CE_FIRSTBIRTH)
          {
            TimeDelay(portnum, ME_RUNTIME);
            TimeDelay(portnum, 1);
          }
          else
          {
            // Wait for whatever we did above to finish up
            TimeDelay(portnum, ETime);
          }
        }
      }

      // See if there's a segment header waiting for us
      if(OutUsed == 0)
      {
        error = ER_NOHEADER;
        break;
      }

      // See if the header is valid (ie: the correct length)
      if(OutUsed != HEADERSIZE)
      {
        error = ER_BADHEADER;
        break;
      }

      // Read the header in from the device
      if((error = RecvHeader(portnum, &seg, &hdrcrc, &hdrsum, &size, &sum)) != 0)
        break;

      // See if this is the first segment of a new message
      if(((seg & 127) == 0) && (!JavaiButton))
      {
        // When we get the first segment of a new message we must
        // initialize everything

        msglen = checksum = 0;
        oldseg = 0;
        mp     = mbuf;
      }
      else
      {
        if(JavaiButton)
        {
          checksum = 0;
          seg = oldseg = 0;
        }
        // See if the segments are in proper order
        else if (++oldseg != (seg & 127))
        {
          error = CE_BADSEQ;
          break;
        }
      }

      // See if the new message data would overrun the buffer
      if((msglen + size) > *bufsize)
      {
        error = ER_OVERRUN;
        break;
      }

      // Make sum equal the whole progressive checksum including header
      checksum += sum;

      // Read in the segment data
      if((error = RecvData(portnum, mp, size, hdrcrc, &sum, JavaiButton)) != 0)
        break;

      // If the message data came in OK then update values
      checksum += sum;        // Keep the end-to-end checksum current

      if(!JavaiButton)
      {
        msglen   += size;       // Keep track of the size of the message
        mp       += size;       // Advance the message pointer
      }
      else
      {
        msglen   += (ushort)(size - 3);       // Keep track of the size of the message
        mp       += size - 3;       // Advance the message pointer
      }

      // Verify the progressive checksum value
      if(hdrsum != checksum)
      {
        error = ER_CHECKSUM;
        break;
      }

      // Keep the running sum correct
      checksum += LSB(hdrsum);
      checksum += MSB(hdrsum);

      //
      // See if this was the last message segment. If it was, then we
      // return with AOK status, and rcvd message size replaces bufsize.

      if(LastSegment && (owms != 14))
      {
        *bufsize = msglen;
        return 0;
      }
      else if(owms == 14)
      {
        JavaiButton = 1;

        if ((error = SendStatus(portnum, 1)) != 0)
	        break;
        if (SendRun(portnum) != 0)
          break;

        TimeDelay(portnum, 2);
      }
      else
      {
        // Send the time value to the CIB
        if((error = SendStatus(portnum, 2)) != 0)
          break;

        // Allow the micro to run and bring up our next segment
        if(SendRun(portnum) != 0)
          break;

        TimeDelay(portnum, 2+SLEEP_PAD);
      }
    }

    // Send the time value to the CIB
    if((error = SendStatus(portnum, (uchar)(Retry? (Retry<<1): 1))) != 0)
      break;

    // If we have any failure, we must alow the micro to run or we
    // may not recover from it
    if(SendRun(portnum) == 0)
    {
      TimeDelay(portnum, (uchar)((Retry? (Retry<<1): 1)+SLEEP_PAD));
    }
  }

  // Return with error status if retries exhausted
  return error;
}

//--------------------------------------------------------------------------
//
// Read a header (FIFO) from the device -
//
//    portnum = port number the device is on
//    seg     = Pointer to segment number from the device
//    hdrcrc  = CRC-16 of the segment data area from the header
//    hdrsum  = Running checksum result from the header
//    size    = Segment data area size from the header
//    sum     = Checksum of the header bytes
//
//    return = Error code, or zero if all went well
//
uchar RecvHeader(int portnum,
                 uchar* seg,
                 ushort* hdrcrc,
                 ushort* hdrsum,
                 uchar* size,
                 ushort* sum)
{
  ushort   owcrc;
  ushort   i;
  FIFORDWR FB;
  uchar    Retry,
           error;

  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      FB.Cmd = OW_FIFOREAD;
      FB.Len = FIFO_LEN;

      // CHECK FOR BLIVET
      // write 0xff's to stream buff directly
      memset(FB.FIFOBuf, 0xFF, 8);

      StreamLen = 0;

      memcpy(StreamBuff,&FB,sizeof(FB));
      StreamLen += 10;

      // set up crc
      StreamBuff[StreamLen++] = 0xff;
      StreamBuff[StreamLen++] = 0xff;

      if(!owAccess(portnum))
      {
         error = ER_ACCESS;
         break;
      }

      if(!owBlock(portnum, FALSE, StreamBuff, StreamLen))
      {
        error = ER_ACCESS;
        break;
      }

      memcpy(&FB,StreamBuff,sizeof(FB));
      owcrc = CRC16Buf(&FB.Cmd, 0, sizeof(FB));

      if((error = CheckStreamCRC(StreamBuff,StreamLen-2,owcrc)) != 0)
      {
        if((error = CheckStreamCRC(StreamBuff,StreamLen-2, (ushort)(owcrc ^ GetCRC16Correction(10)))) != 0)
          break;

        FB.FIFOBuf[0] &= 0x0FE;
      }

      *sum = 0;

      for(i = 0; i < 6; i++)
        *sum += FB.FIFOBuf[i];

      *seg    = FB.FIFOBuf[0];
      *size   = FB.FIFOBuf[1];
      *hdrcrc = (ushort)((FB.FIFOBuf[5] << 8) | FB.FIFOBuf[4]);
      *hdrsum = (ushort)(FB.FIFOBuf[7] << 8) | FB.FIFOBuf[6];

      if(((*seg) & 128) == 128)
        LastSegment = TRUE;
      else
        LastSegment = FALSE;

      if((error = ReleaseSequence(portnum, RS_FIFOREAD)) != 0)
        break;

      // If all went well, return OK status
      return 0;
    }
  }

  // If retries exhautsed, return error status
  return (uchar)(error | ER_RECVHDR);
}

//--------------------------------------------------------------------------
//
// Return the segment data from the device
//
//    portnum = port number the device is on
//    mp      = Pointer to message buffer next available position
//    size    = Size of message segment (from header)
//    hdrcrc  = CRC-16 of the data segment from header
//    sum     = Checksum accumulator for segment data
//
//    return  = Error code, or zero if all went well
//
uchar RecvData(int portnum, uchar* mp, uchar size, ushort hdrcrc, ushort* sum, uchar JavaiButton)
{
  uchar* mstart;
  ushort crc,
         owcrc,
         cksum,
         n;
  int    i;
  uchar  Retry,
         error;
  uchar* TempArray;

  mstart = mp;

  for(Retry = 0; Retry < 3; Retry++)
  {
    while(1)
    {
      // Get the current buffer pointer
      mp = mstart;

      crc = CRC16(size,0);

      // Initialize the local checksum accumulator
      cksum = 0;

      if(!JavaiButton)
      {
        StreamLen = 0;

        StreamBuff[StreamLen++] = OW_IPRREAD;
        StreamBuff[StreamLen++] = size;

        // CHECK FOR BLIVET

        for(i = 0; i < size+2; i++)// 2 for crc read
          StreamBuff[StreamLen++] = 0xff;

        if(!owAccess(portnum))
        {
           error = ER_ACCESS;
           break;
        }

        if(!owBlock(portnum, FALSE, StreamBuff, StreamLen))
        {
          error = ER_ACCESS;
          break;
        }

        owcrc = CRC16Buf(StreamBuff,0,(ushort)(size+2));

        // Check the returning CRC values against what we received
        if((error = CheckStreamCRC(StreamBuff,StreamLen-2,owcrc)) != 0)
        {
          // fix for bit blivet
          if((error = CheckStreamCRC(StreamBuff,StreamLen-2,(ushort)(owcrc ^ GetCRC16Correction((uchar)(size+2))))) != 0)
            break;

          StreamBuff[2] &= 0xFE;
        }

        for(i = 0; i < size; i++)
          mp[i] = StreamBuff[i+2];

        crc   = CRC16Buf(StreamBuff+1,0,(ushort)(size+1));

        for(n = 0; n < size; n++)
          cksum += mp[n];

        // See if the CRC of the data matches what we expected as-per header
        if(hdrcrc != crc)
        {
          error = ER_HDRCRC;
          break;
        }

        // Since we've been successful, update the checksum and return AOK
        *sum = cksum;

        return 0;
      } // if(!JavaiButton)
      else
      {
        if(!(TempArray = malloc((size)*2)))
        {
          error = ER_OVERRUN;
          break;
        }

        StreamLen = 0;

        StreamBuff[StreamLen++] = OW_IPRREAD;
        StreamBuff[StreamLen++] = size;

         // CHECK FOR BLIVET

        for(i = 0; i < size+2; i++)// 2 for crc read
          StreamBuff[StreamLen++] = 0xff;

        if(!owAccess(portnum))
        {
           error = ER_ACCESS;
           break;
        }

        if(!owBlock(portnum, FALSE, StreamBuff, StreamLen))
        {
          error = ER_ACCESS;
          free(TempArray);
          break;
        }

        owcrc = CRC16Buf(StreamBuff,0,(ushort)(size+2));

        // Check the returning CRC values against what we received
        if((error = CheckStreamCRC(StreamBuff,StreamLen-2,owcrc)) != 0)
        {
          if((error = CheckStreamCRC(StreamBuff,StreamLen-2,(ushort)(owcrc ^ GetCRC16Correction((uchar)(size+2))))) != 0)
            break;

          StreamBuff[2] &= 0x0FE;
        }

        for(i = 0; i < size; i++)
          TempArray[i] = StreamBuff[i+2];

        crc   = CRC16Buf(StreamBuff+1,0,(ushort)(size+1));

        for(n = 0; n < size; n++)
          cksum += TempArray[n];

        // See if the CRC of the data matches what we expected as-per header
        if(hdrcrc != crc)
        {
          error = ER_HDRCRC;
          free(TempArray);
          TempArray = NULL;
          break;
        }

        // Since we've been successful, update the checksum and return AOK
        *sum = cksum;
        if(size>2)
           memcpy(mp, TempArray+3,size-3);
        free(TempArray);
        return 0;
      }// else
    } // for
  }// for Retry

  // If retries failed, return the error status
  return (uchar)(error | ER_RECVDATA);
}

//--------------------------------------------------------------------------
//
// Issue the release sequence to the device and check the ACK response
//
//    portnum = port number the device is on
//    rscode  = Release sequence code (command specific)
//
//    return = Error code, or zero if all went well
//
uchar ReleaseSequence(int portnum, ushort rscode)
{
  uchar IOBuf[20];
  ushort rslt;

  IOBuf[0] = LSB(rscode);
  IOBuf[1] = MSB(rscode);

  if(!owBlock(portnum, FALSE, IOBuf, 2))
  {
     return ER_ACCESS;
  }

  if((rscode == RS_MICRORUN) || (rscode == RS_MICROINTERRUPT))
  {
    PowerOn = TRUE;
    rslt = !owReadBitPower(portnum, 0);
  }
  else
    rslt = owTouchBit(portnum, 1);

  // ACK bit from device (should be zero)
  if (!rslt)
    return 0;
  else
    return ER_ACK;
}

//--------------------------------------------------------------------------
//
// Check the returning crc bytes against the crc provided (expected)
//
//      portnum =   port number the device is on
//      crc     =   CRC16 result of received bytes from device
//
//      return  =   Error code, or zero if all went well
//
uchar CheckCRC(int portnum, ushort crc)
{
  uchar IOBuf[2];

  // CHECK FOR BLIVET

  IOBuf[0] = IOBuf[1] = 0xFF;

  if(!owBlock(portnum, FALSE, IOBuf, 2))
  {
     return ER_ACCESS;
  }

  // Verify the CRC LS byte response
  if(IOBuf[0] != (uchar) ~LSB(crc))
    if((IOBuf[0] & 0x0FE) != (uchar) ~LSB(crc))
      return ER_CRC;

  // Verify the CRC MS byte response
  if(IOBuf[1] != (uchar) ~MSB(crc))
    return ER_CRC;

  return 0;
}

//--------------------------------------------------------------------------
//
// Check the returning crc bytes in stream buffer bp against the crc provided
// (expected) -
//
//      data   =   pointer to stream buffer
//      offset =   offset to first SRS byte
//      crc    =   CRC16 result of received bytes from device
//
//      return =   Error code, or zero if all went well
//
uchar CheckStreamCRC(uchar* data, int offset, ushort crc)
{

  if(data[offset] != (LSB(crc)^0xFF))
  {
    return ER_CRC;
  }

  if(data[offset+1] != (MSB(crc)^0xFF))
  {
    return ER_CRC;
  }

  return 0;
}

//--------------------------------------------------------------------------
//
// Produce a time delay as-per et -
//
//      return      =   None
//
void TimeDelay(int portnum, uchar et)
{
   SMALLINT tempLevel = MODE_NORMAL;

   msDelay(et*250 + MinRun);

   if(PowerOn)
   {
      PowerOn = FALSE;
      if(MODE_NORMAL != owLevel(portnum,MODE_NORMAL))
      {
         OWERROR(OWERROR_LEVEL_FAILED);
      }
   }
}

//--------------------------------------------------------------------------
//
//   CRC16 generation. Used here only for table generation.
//
ushort CRC16TGen(uchar x, ushort crc)
{
  uchar bit;
  uchar n;

  for(n = 0; n < 8; n++)
  {
    bit = (uchar)((crc ^ x) & 1);
    crc >>= 1;

    if(bit)
      crc ^= 0xA001;

    x >>= 1;
  }

  return crc;
}


//--------------------------------------------------------------------------
//
//    Generate 256 word table for CRC16 lookup.
//
void GenCRC16Table()
{
  ushort i;

  for(i = 0; i < 256; ++i)
    CRCTable[i] = CRC16TGen((uchar) i, 0);

  GenCRC16CorrectionTable();
}

//--------------------------------------------------------------------------
//
//    Fast CRC16 generation.
//
ushort CRC16(uchar x, ushort crc)
{
  return CRCTable[(crc ^ x) & 0xFF] ^ crc >> 8;
}

//--------------------------------------------------------------------------
//
//    Compute CRC16 of buffer pointed to by bp.
//
ushort CRC16Buf(uchar* bp, ushort crc, ushort Count)
{
  ushort i;

  for(i = 0; i < Count; i++)
    crc = CRCTable[(crc ^ bp[i]) & 0xFF] ^ crc >> 8;

  return crc;
}

//--------------------------------------------------------------------------
//
//    Compute CRC16 correctionTable
//
void GenCRC16CorrectionTable()
{
  int   l_i,
        l_Size  = sizeof(CRCCorrectionTable)/sizeof(ushort);
  ushort  l_CurrentCRC  = 0;

  memset(CRCCorrectionTable,  0,  sizeof(CRCCorrectionTable));
  CRCCorrectionTable[2] = CRC16(1, l_CurrentCRC);

  for(l_i = 3; l_i < l_Size; l_i++)
  {
    CRCCorrectionTable[l_i] = CRC16(0, CRCCorrectionTable[l_i-1]);
  }
}

//--------------------------------------------------------------------------
//
//    Get CRC16 correction value
//
ushort GetCRC16Correction(uchar p_Length)
{
  if(p_Length > sizeof(CRCCorrectionTable)/sizeof(ushort))
    return 0;

  if(p_Length < 1)
    return 0;

  return CRCCorrectionTable[p_Length-1];
}

//--------------------------------------------------------------------------
//
void SetMinRunTime(ushort MR)
{
   MinRun = MR;
}

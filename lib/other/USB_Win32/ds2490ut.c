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
//  ds2490ut.c - DS2490 utility functions.
//             (Requires DS2490.SYS)
//
//  Version: 
//

#include "ownet.h"
#include "ds2490.h"

// handles for the USB ports
extern HANDLE usbhnd[MAX_PORTNUM];

// global DS2490 state
SMALLINT USBLevel[MAX_PORTNUM]; 
SMALLINT USBSpeed[MAX_PORTNUM]; 
SMALLINT USBVersion[MAX_PORTNUM]; 
SMALLINT USBVpp[MAX_PORTNUM]; 

//---------------------------------------------------------------------------
// Attempt to resyc and detect a DS2490
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                OpenCOM to indicate the port number.
//
// Returns:  TRUE  - DS2490 detected successfully
//           FALSE - Could not detect DS2490
//
SMALLINT DS2490Detect(HANDLE hDevice)
{
   SMALLINT present,vpp;
   SETUP_PACKET setup;
   ULONG nOutput = 0;

   // reset the DS2490
   DS2490Reset(hDevice);

   // set the strong pullup duration to infinite
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_SET_DURATION | COMM_IM;
   setup.Index = 0x0000;
   setup.Length = 0;
   setup.DataOut = FALSE;
   // call the driver
   DeviceIoControl(hDevice,
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL);

   // set the 12V pullup duration to 512us
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = COMM_CMD;
   setup.Value = COMM_SET_DURATION | COMM_IM | COMM_TYPE;
   setup.Index = 0x0040;
   setup.Length = 0;
   setup.DataOut = FALSE;
   // call the driver
   DeviceIoControl(hDevice,
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL);

   // disable strong pullup, but leave program pulse enabled (faster)
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = MODE_CMD;
   setup.Value = MOD_PULSE_EN;
   setup.Index = ENABLEPULSE_PRGE; 
   setup.Length = 0x00;
   setup.DataOut = FALSE;
   // call the driver
   DeviceIoControl(hDevice,
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL);

   // return result of short check
   return DS2490ShortCheck(hDevice,&present,&vpp);
}


//---------------------------------------------------------------------------
// Check to see if there is a short on the 1-Wire bus. Used to stop 
// communication with the DS2490 while the short is in effect to not
// overrun the buffers.
//
// '*present' - flag set (1) if device presence detected
// '*vpp' - flag set (1) if Vpp programming voltage detected
//
// Returns:  TRUE  - DS2490 1-Wire is NOT shorted
//           FALSE - Could not detect DS2490 or 1-Wire shorted
//
SMALLINT DS2490ShortCheck(HANDLE hDevice, SMALLINT *present, SMALLINT *vpp)
{
   STATUS_PACKET status;
   BYTE nResultRegisters;
   BYTE i;

   // get the result registers (if any)
   if (!DS2490GetStatus(hDevice, &status, &nResultRegisters))
      return FALSE;

   // get vpp present flag
   *vpp = ((status.StatusFlags & STATUSFLAGS_12VP) != 0);

   //	Check for short
   if(status.CommBufferStatus != 0)
   {	
      return FALSE;
   }
   else
   {  
      // check for short
	   for (i = 0; i < nResultRegisters; i++)
	   {	
         // check for SH bit (0x02), ignore 0xA5
         if (status.CommResultCodes[i] & COMMCMDERRORRESULT_SH)
         {	
            // short detected
            return FALSE; 
         }
      }
   }

   // check for No 1-Wire device condition
   *present = TRUE;
   // loop through result registers
   for (i = 0; i < nResultRegisters; i++)
   {   
      // only check for error conditions when the condition is not a ONEWIREDEVICEDETECT
      if (status.CommResultCodes[i] != ONEWIREDEVICEDETECT)
      {   
         // check for NRS bit (0x01)
         if (status.CommResultCodes[i] & COMMCMDERRORRESULT_NRS)
         {   
            // empty bus detected
            *present = FALSE;
         }
      }
   }

   return TRUE;
}


//---------------------------------------------------------------------------
// Stop any on-going pulses
//
// Returns:  TRUE  - pulse stopped
//           FALSE - Could not stop pulse
//
SMALLINT DS2490HaltPulse(HANDLE hDevice)
{
   STATUS_PACKET status;
   BYTE nResultRegisters;
   SETUP_PACKET setup;
   ULONG nOutput = 0;
   long limit;

   
   // set a time limit
   limit = msGettick() + 300;
   // loop until confirm pulse has ended or timeout
   do
   {
      // HalExecWhenIdle, ResumExecution to stop an infinite pulse
      
      // HalExecWhenIdle
      setup.RequestTypeReservedBits = 0x40;
      setup.Request = CONTROL_CMD;
      setup.Value = CTL_HALT_EXE_IDLE;
      setup.Index = 0x00;
      setup.Length = 0x00;
      setup.DataOut = FALSE;
      // call the driver
      if (!DeviceIoControl(hDevice,
					       DS2490_IOCTL_VENDOR,
					       &setup,
					       sizeof(SETUP_PACKET),
					       NULL,
					       0,
					       &nOutput,
					       NULL))
      {
         // failure
         break;
      }

      // ResumExecution   
      setup.RequestTypeReservedBits = 0x40;
      setup.Request = CONTROL_CMD;
      setup.Value = CTL_RESUME_EXE;
      setup.Index = 0x00;
      setup.Length = 0x00;
      setup.DataOut = FALSE;

      // call the driver
      if (!DeviceIoControl(hDevice,
					       DS2490_IOCTL_VENDOR,
					       &setup,
					       sizeof(SETUP_PACKET),
					       NULL,
					       0,
					       &nOutput,
					       NULL))
      {
         // failure
         break;
      }

      // read the status to see if the pulse has been stopped
      if (!DS2490GetStatus(hDevice, &status, &nResultRegisters))
      {
         // failure
         break;
      }
      else
      {
         // check the SPU flag
         if ((status.StatusFlags & STATUSFLAGS_SPUA) == 0)
         {
            // success
            // disable both pulse types
            setup.RequestTypeReservedBits = 0x40;
            setup.Request = MODE_CMD;
            setup.Value = MOD_PULSE_EN;
            setup.Index = 0;
            setup.Length = 0x00;
            setup.DataOut = FALSE;
            // call the driver
            DeviceIoControl(hDevice,
					             DS2490_IOCTL_VENDOR,
					             &setup,
					             sizeof(SETUP_PACKET),
					             NULL,
					             0,
					             &nOutput,
					             NULL);

            return TRUE;
         }
      }
   }
   while (limit > msGettick());

   return FALSE;
}

//---------------------------------------------------------------------------
// Description: Gets the status of the DS2490 device
// Input:       hDevice - the handle to the DS2490 device
//              pStatus - the Status Packet to be filled with data
//              pResultSize - the number of result register codes returned
//                can be NULL
// Returns:     FALSE on failure, TRUE on success
// Error Codes: DS2490COMM_ERR_USBDEVICE
SMALLINT DS2490GetStatus(HANDLE hDevice, STATUS_PACKET *status, BYTE *pResultSize)
{
   BYTE *bufOutput = (BYTE *)status;
   ULONG nOutput = sizeof(STATUS_PACKET);

   // Call device IO Control interface (DS2490_IOCTL_STATUS) in driver
   if (!DeviceIoControl(hDevice,
						 DS2490_IOCTL_STATUS,
						 NULL,
						 0,
						 bufOutput,
						 sizeof(STATUS_PACKET),
						 &nOutput,
						 NULL)
	   )
   {
      OWERROR(OWERROR_ADAPTER_ERROR);
      return FALSE;
   }

   // any bytes returned after the 16th byte are place in the CommResultCodes field
   if (pResultSize != NULL)
   {
      *pResultSize = (BYTE) nOutput - 16;
   }

   return TRUE;
}


//---------------------------------------------------------------------------
// Description: Perfroms a hardware reset of the DS2490 equivalent to a 
//              power-on reset
// Input:       hDevice - the handle to the DS2490 device
// Returns:     FALSE on failure, TRUE on success
// Error Codes: DS2490COMM_ERR_USBDEVICE
//
SMALLINT DS2490Reset(HANDLE hDevice)
{
   SETUP_PACKET setup;
   ULONG nOutput = 0;

   // setup for reset
   setup.RequestTypeReservedBits = 0x40;
   setup.Request = CONTROL_CMD;
   setup.Value = CTL_RESET_DEVICE;
   setup.Index = 0x00;
   setup.Length = 0x00;
   setup.DataOut = FALSE;
   // call the driver
   return (!DeviceIoControl(hDevice,
					    DS2490_IOCTL_VENDOR,
					    &setup,
					    sizeof(SETUP_PACKET),
					    NULL,
					    0,
					    &nOutput,
					    NULL));
}


//---------------------------------------------------------------------------
// Description: Reads data from EP3
// Input:       hDevice - the handle to the DS2490 device
//              buffer - the size must be >= nBytes
//              pnBytes - the number of bytes to read from the device
// Returns:     FALSE on failure, TRUE on success
//
SMALLINT DS2490Read(HANDLE hDevice, BYTE *buffer, WORD *pnBytes)
{

   // Synchronous read:
	ULONG	nBytes = *pnBytes;

	if	(!ReadFile(hDevice,  // handle
                 buffer,   // buffer to read
                 nBytes,   // number of bytes to read
                 &nBytes,  // number of bytes read
                 NULL   // overlap
						))
	{
      OWERROR(OWERROR_ADAPTER_ERROR);
		return FALSE;
	}
	else
	{
		*pnBytes = (WORD)nBytes;
		return TRUE;
	}
}

//---------------------------------------------------------------------------
// Description: Writes data to EP2
// Input:       hDevice - the handle to the DS2490 device
//              buffer - the size must be >= nBytes
//              pnBytes - the number of bytes to write to the device
// Returns:     FALSE on failure, TRUE on success
//
SMALLINT DS2490Write(HANDLE hDevice, BYTE *buffer, WORD *pnBytes)
{
   // Synchronous write:
   // assume enough room for write
   ULONG	nBytes = *pnBytes;

   if	(!WriteFile(hDevice,    // handle
                  buffer,     // buffer to read
                  *pnBytes,   // number of bytes to read
                  &nBytes,    // number of bytes read
                  NULL))      // overlap
   {
      OWERROR(OWERROR_ADAPTER_ERROR);
      return FALSE;
   }
   else
   {
      *pnBytes = (WORD)nBytes;
      return TRUE;
   }
}


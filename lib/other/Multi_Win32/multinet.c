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
//  multinet.C - Wrapper to hook all adapter types in the 1-Wire Public 
//               Domain API for network functions.
//
//  Version: 3.00
//
//

#include "mownet.h"

extern SMALLINT default_type; 

//--------------------------------------------------------------------------
// The 'owFirst' finds the first device on the 1-Wire Net  This function
// contains one parameter 'alarm_only'.  When
// 'alarm_only' is TRUE (1) the find alarm command 0xEC is
// sent instead of the normal search command 0xF0.
// Using the find alarm command 0xEC will limit the search to only
// 1-Wire devices that are in an 'alarm' state.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'do_reset'   - TRUE (1) perform reset before search, FALSE (0) do not
//                perform reset before search.
// 'alarm_only' - TRUE (1) the find alarm command 0xEC is
//                sent instead of the normal search command 0xF0
//
// Returns:   TRUE (1) : when a 1-Wire device was found and it's
//                        Serial Number placed in the global SerialNum[portnum]
//            FALSE (0): There are no devices on the 1-Wire Net.
//
SMALLINT owFirst(int portnum, SMALLINT do_reset, SMALLINT alarm_only)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owFirst_DS9490(portnum & 0xFF, do_reset, alarm_only);
      case DS1410E: return owFirst_DS1410E(portnum & 0xFF, do_reset, alarm_only);
      default: 
      case DS9097U: return owFirst_DS9097U(portnum & 0xFF, do_reset, alarm_only);
   };
}

//--------------------------------------------------------------------------
// The 'owNext' function does a general search.  This function
// continues from the previos search state. The search state
// can be reset by using the 'owFirst' function.
// This function contains one parameter 'alarm_only'.
// When 'alarm_only' is TRUE (1) the find alarm command
// 0xEC is sent instead of the normal search command 0xF0.
// Using the find alarm command 0xEC will limit the search to only
// 1-Wire devices that are in an 'alarm' state.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'do_reset'   - TRUE (1) perform reset before search, FALSE (0) do not
//                perform reset before search.
// 'alarm_only' - TRUE (1) the find alarm command 0xEC is
//                sent instead of the normal search command 0xF0
//
// Returns:   TRUE (1) : when a 1-Wire device was found and it's
//                       Serial Number placed in the global SerialNum[portnum]
//            FALSE (0): when no new device was found.  Either the
//                       last search was the last device or there
//                       are no devices on the 1-Wire Net.
//
SMALLINT owNext(int portnum, SMALLINT do_reset, SMALLINT alarm_only)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owNext_DS9490(portnum & 0xFF, do_reset, alarm_only);
      case DS1410E: return owNext_DS1410E(portnum & 0xFF, do_reset, alarm_only);
      default: 
      case DS9097U: return owNext_DS9097U(portnum & 0xFF, do_reset, alarm_only);
   };
}

//--------------------------------------------------------------------------
// The 'owSerialNum' function either reads or sets the SerialNum buffer
// that is used in the search functions 'owFirst' and 'owNext'.
// This function contains two parameters, 'serialnum_buf' is a pointer
// to a buffer provided by the caller.  'serialnum_buf' should point to
// an array of 8 unsigned chars.  The second parameter is a flag called
// 'do_read' that is TRUE (1) if the operation is to read and FALSE
// (0) if the operation is to set the internal SerialNum buffer from
// the data in the provided buffer.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'serialnum_buf' - buffer to that contains the serial number to set
//                   when do_read = FALSE (0) and buffer to get the serial
//                   number when do_read = TRUE (1).
// 'do_read'       - flag to indicate reading (1) or setting (0) the current
//                   serial number.
//
void owSerialNum(int portnum, uchar *serialnum_buf, SMALLINT do_read)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  owSerialNum_DS9490(portnum & 0xFF, serialnum_buf, do_read);
               break;
      case DS1410E: owSerialNum_DS1410E(portnum & 0xFF, serialnum_buf, do_read);
               break;
      default: 
      case DS9097U: owSerialNum_DS9097U(portnum & 0xFF, serialnum_buf, do_read);
               break;
   };
}

//--------------------------------------------------------------------------
// Setup the search algorithm to find a certain family of devices
// the next time a search function is called 'owNext'.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number was provided to
//                   OpenCOM to indicate the port number.
// 'search_family' - family code type to set the search algorithm to find
//                   next.
//
void owFamilySearchSetup(int portnum, SMALLINT search_family)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  owFamilySearchSetup_DS9490(portnum & 0xFF, search_family);
               break;
      case DS1410E: owFamilySearchSetup_DS1410E(portnum & 0xFF, search_family);
               break;
      default: 
      case DS9097U: owFamilySearchSetup_DS9097U(portnum & 0xFF, search_family);
               break;
   };
}

//--------------------------------------------------------------------------
// Set the current search state to skip the current family code.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
void owSkipFamily(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  owSkipFamily_DS9490(portnum & 0xFF);
               break;
      case DS1410E: owSkipFamily_DS1410E(portnum & 0xFF);
               break;
      default: 
      case DS9097U: owSkipFamily_DS9097U(portnum & 0xFF);
               break;
   };
}

//--------------------------------------------------------------------------
// The 'owAccess' function resets the 1-Wire and sends a MATCH Serial
// Number command followed by the current SerialNum code. After this
// function is complete the 1-Wire device is ready to accept device-specific
// commands.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:   TRUE (1) : reset indicates present and device is ready
//                       for commands.
//            FALSE (0): reset does not indicate presence or echos 'writes'
//                       are not correct.
//
SMALLINT owAccess(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owAccess_DS9490(portnum & 0xFF);
      case DS1410E: return owAccess_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owAccess_DS9097U(portnum & 0xFF);
   };
}

//----------------------------------------------------------------------
// The function 'owVerify' verifies that the current device
// is in contact with the 1-Wire Net.
// Using the find alarm command 0xEC will verify that the device
// is in contact with the 1-Wire Net and is in an 'alarm' state.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'alarm_only'  - TRUE (1) the find alarm command 0xEC
//                 is sent instead of the normal search
//                 command 0xF0.
//
// Returns:   TRUE (1) : when the 1-Wire device was verified
//                       to be on the 1-Wire Net
//                       with alarm_only == FALSE
//                       or verified to be on the 1-Wire Net
//                       AND in an alarm state when
//                       alarm_only == TRUE.
//            FALSE (0): the 1-Wire device was not on the
//                       1-Wire Net or if alarm_only
//                       == TRUE, the device may be on the
//                       1-Wire Net but in a non-alarm state.
//
SMALLINT owVerify(int portnum, SMALLINT alarm_only)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owVerify_DS9490(portnum & 0xFF, alarm_only);
      case DS1410E: return owVerify_DS1410E(portnum & 0xFF, alarm_only);
      default: 
      case DS9097U: return owVerify_DS9097U(portnum & 0xFF, alarm_only);
   };
}

//----------------------------------------------------------------------
// Perform a overdrive MATCH command to select the 1-Wire device with
// the address in the ID data register.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Returns:  TRUE: If the device is present on the 1-Wire Net and
//                 can do overdrive then the device is selected.
//           FALSE: Device is not present or not capable of overdrive.
//
//  *Note: This function could be converted to send DS2480
//         commands in one packet.
//
SMALLINT owOverdriveAccess(int portnum)
{
   switch ((default_type) ? default_type : (portnum >> 8) & 0xFF)
   {
      case DS9490:  return owOverdriveAccess_DS9490(portnum & 0xFF);
      case DS1410E: return owOverdriveAccess_DS1410E(portnum & 0xFF);
      default: 
      case DS9097U: return owOverdriveAccess_DS9097U(portnum & 0xFF);
   };
}

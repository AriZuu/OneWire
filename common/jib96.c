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
//     Module Name: Jib API
//
//     Description: API for executing methods on the iButton for Java and
//                  for executing the iButton Command Processor.
//
//        Filename: jib96.c
//
// Dependant Files: jib96.h
//
//         Version: 1.00 (based on jibapi.c V1.24)
//
//        Comments: - "iButton with Java" API.
//
//           Notes: - The format for all values passed to or from the 
//                    iButton's Command Processor are "Little Endian". The 
//                    format for values passed to/from an applet's process 
//                    method are determined by the applet's author.
//                  - Not all commands in this API version are supported in 
//                    older firmware revisions of the Java Button.
//

#include "jib96.h"

// globals to this module
static ushort       g_LastError       = ERR_ISO_NORMAL_00;
static CMDPACKET    g_CmdPacket;
static RETPACKET    g_RetPacket;
static ushort       g_ExecTimeMS      = 64;
static ushort       g_MinTimeMS       = 0;
static RESPONSEAPDU g_ResponseAPDU;
static JIBMASTERPIN g_MasterPIN       = {0,{0,0,0,0,0,0,0,0}};

//--------------------------------------------------------------------------
// Sets the default execution time.
//  
void SetDefaultExecTime(ushort p_ExecTimeMS)
{
  g_ExecTimeMS = p_ExecTimeMS;
}

//--------------------------------------------------------------------------
// Sends a generic command APDU to the iButton
//  @param portnum is the port number where the device is
//  @param p_lpCommandAPDU is configured by the caller
//  @param p_ExecTimeMS is the estimated runtime in 
// milliseconds. Valid runtimes range from 64 milliseconds to just under
// 4 seconds (4000 ms). Invalid runtimes will be adjusted up or down
// as necessary.
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SendAPDU(int portnum, 
                        LPCOMMANDAPDU  p_lpCommandAPDU, 
                        ushort         p_ExecTimeMS)
{
  ushort l_Error;
  ushort l_RecvLen;
  ushort l_Delay = 0;

  memset((uchar*)&g_ResponseAPDU,0,sizeof(g_ResponseAPDU));

  if(p_ExecTimeMS)
  {
    l_Delay = (p_ExecTimeMS + g_MinTimeMS) / 250;   

    if(l_Delay > 0x0F)
      l_Delay = 0x0F;
  }

  if(!p_lpCommandAPDU)
  {
    SetJiBError(g_ResponseAPDU.SW);
    return &g_ResponseAPDU;
  }  
  
  g_CmdPacket.CmdByte = 137;
  g_CmdPacket.GroupID = 0;
  // add 3 bytes for the packet header
  g_CmdPacket.Len = 3 + sizeof(p_lpCommandAPDU->Header); 
                                                              
  memcpy(g_CmdPacket.CmdData,
          p_lpCommandAPDU->Header,
          sizeof(p_lpCommandAPDU->Header));
  g_CmdPacket.CmdData[g_CmdPacket.Len++ - 3] = p_lpCommandAPDU->Lc;
  
  if(p_lpCommandAPDU->Lc)
  {
    if(!p_lpCommandAPDU->Data)
    {
      SetJiBError(g_ResponseAPDU.SW);
      return &g_ResponseAPDU;
    }  

    memcpy(g_CmdPacket.CmdData + g_CmdPacket.Len - 3, // CmdData is 3 bytes into structure  
            p_lpCommandAPDU->Data,
            p_lpCommandAPDU->Lc);
    
    g_CmdPacket.Len += p_lpCommandAPDU->Lc;
  }
  
  g_CmdPacket.CmdData[g_CmdPacket.Len++ - 3] = p_lpCommandAPDU->Le;          

  l_Error = SendCiBMessage(portnum, (uchar*)&(g_CmdPacket.Len),
                           (ushort) (g_CmdPacket.Len+1),
                           (uchar)l_Delay);

  if(!l_Error)
  {
    l_RecvLen = sizeof(g_RetPacket); 

    // Read the devices response  
    l_Error = RecvCiBMessage(portnum, (uchar*)&(g_RetPacket.CSB), 
                             &l_RecvLen);
 
    if(!l_Error)    
    {
      // fill in the response APDU  
      if(l_RecvLen >= 5)
        g_ResponseAPDU.Len  = l_RecvLen - 5; // subtract two for SW, three for the header  
      else
        g_ResponseAPDU.Len  = 0;

      g_ResponseAPDU.Data = g_RetPacket.CmdData;
      g_ResponseAPDU.SW   = ((ushort)(g_RetPacket.CmdData[g_ResponseAPDU.Len]) << 8) +
                            g_RetPacket.CmdData[g_ResponseAPDU.Len+1];
    }
  }
    
  if(!l_Error)
  {
    if(!g_RetPacket.CSB)
      SetJiBError(g_ResponseAPDU.SW);
    else  
      SetJiBError(g_RetPacket.CSB);
  }
  else
  {
    g_ResponseAPDU.SW = ERR_COMM_FAILURE;
    SetJiBError(ERR_COMM_FAILURE); 
  }         

  return &g_ResponseAPDU;                          
}

//--------------------------------------------------------------------------
// Sets the Master PIN variable that is sent to the iButton with commands
// that require a PIN. This command does not set the iButton's Master PIN.
//  
void SetPIN(LPJIBMASTERPIN p_lpMasterPIN)
{
   if(p_lpMasterPIN)
     if(p_lpMasterPIN->Len <= MAX_PIN_SIZE)
       memcpy(&g_MasterPIN,p_lpMasterPIN,sizeof(JIBMASTERPIN));
}

//--------------------------------------------------------------------------
// Runs the process method on the selected applet. Valid CLA, INS, P1 and P2 
// values are chosen by the applet writer. CLA, INS, P1 and P2 values used 
// by the API and/or the iButton Command Processor are reserved.
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
// 
LPRESPONSEAPDU Process(int portnum,
                       uchar   p_CLA, 
                       uchar   p_INS, 
                       uchar   p_P1, 
                       uchar   p_P2, 
                       uchar   p_Lc, 
                       uchar* p_lpData,
                       ushort   p_RunTimeMS)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = p_CLA;
  l_CommandAPDU.INS  = p_INS;
  l_CommandAPDU.P1   = p_P1;
  l_CommandAPDU.P2   = p_P2;
  l_CommandAPDU.Lc   = p_Lc;
  l_CommandAPDU.Data = p_lpData;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  return SendAPDU(portnum, &l_CommandAPDU,p_RunTimeMS);  
}

//--------------------------------------------------------------------------
// Selects an applet to be active on the iButton.
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
// NOTE: The iButton will return an error if the string is not in the
//       range 5 to 16 bytes (inclusive).
//  
LPRESPONSEAPDU Select(int portnum, LPAID p_lpAID)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_SELECT;
  l_CommandAPDU.INS  = JIB_APDU_INS_SELECT;
  l_CommandAPDU.P1   = JIB_APDU_P1_SELECT;
  l_CommandAPDU.P2   = JIB_APDU_UNUSED;
  l_CommandAPDU.Lc   = p_lpAID->Len;
  l_CommandAPDU.Data = p_lpAID->Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Issues a master erase of the iButton
//  @return A ResponseAPDU object containing the response apdu sent by the 
//          iButton. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU MasterErase(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 0;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+1);
  l_CommandAPDU.Data = (uchar*)&g_MasterPIN;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//  @return A ResponseAPDU that contains the amount of free RAM. 
//          SW = 0x9000 indicates success. 
//
LPRESPONSEAPDU GetFreeRam(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 1;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//  @return A ResponseAPDU that contains the amount of free RAM. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetFreeRamPerBank(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_STATS;
  l_CommandAPDU.P2   = 1;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//  @return A ResponseAPDU containing the Firmware Version String.
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetFirmwareVersionString(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Reads the ephemeral garbage collection mode. 
//  @return The ResponseAPDU containing the value of the ephemeral garbage
//          Collector. A value of 1 indicates the garbage collector is 
//          turned on. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetEphemeralGCMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 2;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the mode of the ephemeral garbage collector.  Setting the mode to 
// 1 turns the collector on, setting it to 0 turns the collector off.   
// If the iButton is password protected, setPIN() should be called before  
// invoking this method. 
// Parameter: p_Mode 1  - turn the collector on, 
//                   0  - turn the collector off.  
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
// 
LPRESPONSEAPDU SetEphemeralGCMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 2;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Reads the applet garbage collection mode. 
//  @return The ResponseAPDU containing the mode of the applet garbage 
//          Collector. A value of 1 indicates the garbage collector is 
//          turned on. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetAppletGCMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 3;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the mode of the applet garbage collector.  Setting the mode to 1 
// turns the collector on, setting it to 0 turns the collector off.   
// If the iButton is password protected, setPIN() should be called before  
// invoking this method. 
// Parameter: p_Mode 1  - turn the collector on, 
//                     0  - turn the collector off.  
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetAppletGCMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 3;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Returns the mode of the CommandPIN.  If a value of 1 is returned by 
// the iButton, PINs are required to perform all Admistrative commands. 
//  @return The ResponseAPDU containing the mode of the CommandPIN. 
//          A value of 1 indicates a PIN is required for all Administrative 
//          commands. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetCommandPINMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 4;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the mode of the CommandPIN.  Setting the mode to 1 indicates the 
// the iButton should require a PIN for all Administrative commands. 
// Parameter: p_Mode 1  - set the CommandPIN mode to require a PIN for 
//                            all Administrative commands. 
//                   0  - set the CommandPIN mode to not require a PIN 
//                            for Administrative commands. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetCommandPINMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 4;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Returns the mode of the LoadPIN.  If a value of 1 is returned by the 
// iButton, PINs are required to load applets. 
//  @return The ResponseAPDU containing the mode of the LoadPIN. 
//          A value of 1 indicates a PIN is required for loading applets 
//          into the iButton. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetLoadPINMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 5;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the mode of the LoadPIN.  Setting the mode to 1 indicates the
// iButton should require a PIN for all applets loads. 
// Parameter: p_Mode 1 - set the LoadPIN mode to require a PIN for applet 
//                       load. 
//                   0 - set the LoadPIN mode to not require a PIN for 
//                       applet load. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetLoadPINMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 5;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//  @return The ResponseAPDU containing the restore mode.
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetRestoreMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 6;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Setting the mode to 1 enables restoration of fields to previous values 
// if a transaction was interrupted. 
// Parameter: p_Mode 1 - Allow restoration. 
//                   0 - Do not allow restoration. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetRestoreMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 6;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Gets the Exception mode of the iButton.  If the mode is 1, then VM 
// exceptions are thrown. If the mode is 0, then VM exceptions are not 
// thrown. 
//  @return The ResponseAPDU containing the Exception mode.
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetExceptionMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 7;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the Exception mode of the iButton.  If the mode is 1, then VM 
// exceptions are thrown. If the mode is 0, then VM exceptions are not 
// thrown. 
// Parameter: p_Mode 1 - set the Exception mode to allow VM thrown 
//                       exceptions. 
//                   0 - set the Exception mode to not allow the VM to 
//                       throw exceptions. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetExceptionMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 7;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Gets the ISOException mode of the iButton.  If the mode is 1,  
// ISOExceptions are handled differently from all other exceptions. 
// Instead of returning 0x6f00 for an uncaught ISOException, the reason is
// returned in the SW. If the mode is 0, ISOExceptions are handled like
// all other exceptions.
//  @return The ResponseAPDU containing the ISOException mode.
//          SW = 0x9000 indicates success.
// NOTE: Requires Firmware Version 1.00 or greater. 
//  
LPRESPONSEAPDU GetISOExceptionMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 8;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the ISOException mode of the iButton.  If the mode is 1,  
// ISOExceptions are handled differently from all other exceptions. 
// Instead of returning 0x6f00 for an uncaught ISOException, the reason is
// returned in the SW. If the mode is 0, ISOExceptions are handled like
// all other exceptions. 
// Parameter: p_Mode 1 - set the ISOException mode. 
//                   0 - clear the ISOException mode. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
// NOTE: Requires Firmware Version 1.00 or greater. 
//  
LPRESPONSEAPDU SetISOExceptionMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 8;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Gets the Error Reporting mode of the iButton. If the mode is 1, 
// GetLastJCREError will return the error byte indicating the reason for 
// the last "Java Card Runtime Environment" thrown error.
//  @return The ResponseAPDU containing the Error Reporting mode.
//          SW = 0x9000 indicates success. 
// NOTE: Requires Firmware Version 1.00 or greater. 
//  
LPRESPONSEAPDU GetErrorReportMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 9;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the Error Report mode of the iButton. If the mode is 1, 
// GetLastJCREError will return the error byte indicating the reason for 
// the last "Java Card Runtime Environment" thrown error. 
// Parameter: p_Mode 1 - set the Error Report mode to. 
//                   0 - set the Error Report mode to. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
// NOTE: Requires Firmware Version 1.00 or greater. 
//  
LPRESPONSEAPDU SetErrorReportMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 9;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Gets the size of the iButton's Commit Buffer.   
//  @return The ResponseAPDU containing the size of the iButton's commit 
//          buffer in bytes. The ResponseAPDU's data field will contain two 
//          bytes representing the size (short). 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetCommitBufferSize(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x0A;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Gets the number of fields in the iButton's Commit Buffer.   
//  @return The ResponseAPDU containing the size of the iButton's commit 
//          buffer in number of fields. The ResponseAPDU's data field will 
//          contain two bytes representing the size (short). 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetCommitBufferFields(int portnum)
{
  LPRESPONSEAPDU  l_lpResponseAPDU;
  short           l_BufferSize;  
  
  l_lpResponseAPDU = GetCommitBufferSize(portnum);
  
  if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
    return l_lpResponseAPDU;

  l_BufferSize  = ((short)l_lpResponseAPDU->Data[0]&0xff) + 
                  ((short)l_lpResponseAPDU->Data[1]<<8);
  l_BufferSize /= COMMIT_BUFFER_BYTES_PER_FIELD;
  l_lpResponseAPDU->Data[0] = (uchar)(l_BufferSize & 0xff);
  l_lpResponseAPDU->Data[1] = (uchar)(l_BufferSize>>8);
  return l_lpResponseAPDU;
}

//--------------------------------------------------------------------------
// Sets the iButtons CommitBufferSize.  The value passed in is rounded up 
// by the iButton to the nearest multiple of COMMIT_BUFFER_ucharS_PER_FIELD. 
// Parameter: p_SizeInBytes the new size of the commit buffer in bytes. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetCommitBufferSize(int portnum, short p_SizeInBytes)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 0x0A;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+3); // PIN Length + PIN Data + size  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = (uchar)(p_SizeInBytes&0xff);
  l_Data[g_MasterPIN.Len+2] = (uchar)(p_SizeInBytes>>8);
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Calculates the number of bytes that is needed to store p_NumberOfFields
// fields in the commit buffer and resizes the buffer accordingly. 
// Parameter: p_NumberOfFields - the new size of the commit buffer using 
//                               number of fields. 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetCommitBufferFields(int portnum, short p_NumberOfFields)
{
  LPRESPONSEAPDU  l_lpResponseAPDU;
  short           l_BufferSize;

  l_BufferSize = p_NumberOfFields*COMMIT_BUFFER_BYTES_PER_FIELD;

  l_lpResponseAPDU = SetCommitBufferSize(portnum, l_BufferSize);
  return l_lpResponseAPDU;  
}

//--------------------------------------------------------------------------
// Reads the Answer to Reset from the iButton, as defined in ISO7816-5 
//  @return Response APDU that contains the iButton's ATR. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetATR(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x0B;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Reads the Real Time Clock of the iButton.   
//  @return The ResponseAPDU containing the value of the Real Time Clock. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetRealTimeClock(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x0C;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Reads random bytes from the iButton.  The maximum number of bytes that 
// can be reads is MAX_RECEIVE - APDU overhead. 
// Parameter: p_NumBytes - the number of random bytes to read from the 
//                         iButton. 
//  @return Response APDU that contains the requested number of random 
//          bytes. 
//          SW = 0x9000 indicates success. 
//
LPRESPONSEAPDU GetRandomBytes(int portnum, short p_NumBytes)
{
  uchar            l_Data[2];
  uchar            l_RecvData[MAX_RECEIVE];
  uchar*          l_Offset    = l_RecvData;
  COMMANDAPDU     l_CommandAPDU;
  LPRESPONSEAPDU  l_lpResponseAPDU;
  short           l_NumBytes  = p_NumBytes;

  // .03 Bug workaround
  if(!p_NumBytes)
  {
    g_ResponseAPDU.Len  = 0;
    g_ResponseAPDU.SW   = ERR_ISO_NORMAL_00;
    return &g_ResponseAPDU;
  }

  if(p_NumBytes > (MAX_RECEIVE - (sizeof(RESPONSEAPDU)-sizeof(uchar*))))
  {
    g_ResponseAPDU.Len  = 0;
    g_ResponseAPDU.SW   = ERR_API_INVALID_PARAMETER;
    return &g_ResponseAPDU;
  }

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x0D;
  l_CommandAPDU.Lc   = 2;
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  // can only generate 110 bytes per APDU
  while(l_NumBytes > 110)
  {
    l_Data[0] = (uchar)120;
    l_Data[1] = (uchar)0;

    l_lpResponseAPDU = SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);
    
    if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
      return l_lpResponseAPDU;
    
    memcpy(l_Offset,l_lpResponseAPDU->Data,120);
    l_Offset    += 120;
    l_NumBytes  -= 120;
  }

  l_Data[0] = (uchar)(l_NumBytes & 0xff);
  l_Data[1] = (uchar)(l_NumBytes >> 8);
  
  l_lpResponseAPDU = SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);
    
  if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
    return l_lpResponseAPDU;

  if(p_NumBytes > 120)
  {
    memcpy(l_Offset,l_lpResponseAPDU->Data,l_NumBytes);
    memcpy(l_lpResponseAPDU->Data,l_RecvData,p_NumBytes);
    l_lpResponseAPDU->Len = (ushort)p_NumBytes;
  }

  return l_lpResponseAPDU; 
}

//--------------------------------------------------------------------------
// Returns the AID of the applet with the given number.  Each applet stored 
// on the iButton is assigned a number(0 - 15).  A directory structure of 
// applets on an iButton can be built by calling this method iteratively 
// with parameter values from 0 to 15. 
// NOTE:  - AIDs are not guaranteed to be stored in contiguous locations.
//          Therefore an exaustive search through all numbers 0 - 15 is
//          required to enumerate all applets on the iButton.
//        - Version 32 and below used 2 bytes for the AID length. Later
//          versions use 1 byte.
// Parameter: p_AIDNum - the number of the applet to retrieve the AID. 
//  @return Response APDU containing the AID of the applet requested. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetAIDByNumber(int portnum, uchar p_AIDNum)
{
  COMMANDAPDU     l_CommandAPDU;
  LPRESPONSEAPDU  l_lpResponseAPDU;
  uchar           l_Buff[21];// max AID length +Java Array length + SW length  
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x0E;
  l_CommandAPDU.Lc   = 1;
  l_CommandAPDU.Data = &p_AIDNum;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  l_lpResponseAPDU = SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);
  
  if(l_lpResponseAPDU)
  {
    if(l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00)
    {
      // .03 firmware bug workaround    
      if(l_lpResponseAPDU->SW != 0x8453) // AID not found SW
      {
        if(l_lpResponseAPDU->Data)
        {
          // remove Java array length
          memcpy(l_Buff,l_lpResponseAPDU->Data,0x12);
          l_lpResponseAPDU->Data[0] = l_Buff[0];
          memcpy(l_lpResponseAPDU->Data+1,l_Buff+2,0x10);
          // now set the correct length
          l_lpResponseAPDU->Len = (ushort)l_Buff[0]+1;
          // insert the happy response
          l_lpResponseAPDU->SW = ERR_ISO_NORMAL_00;
        }
        else
        {
          // data not valid
          l_lpResponseAPDU->Len = 0;
          // insert the unhappy response
          l_lpResponseAPDU->SW = ERR_COMM_FAILURE;
        }
      }
    }
    else
      l_lpResponseAPDU->Len = (ushort)l_lpResponseAPDU->Data[0]+1;
  }

  return l_lpResponseAPDU;
}

//--------------------------------------------------------------------------
// If Error Reporting Mode is enabled, the error byte indicating the reason 
// for the last JCRE thrown error is returned. Otherwise an error is 
// returned in the SW (see: GetErrorReportMode,SetErrorReportMode).
//  @return The ResponseAPDU containing the Error Reporting mode.
//          SW = 0x9000 indicates success. 
// NOTE: Requires Firmware Version 1.00 or greater. 
//  
LPRESPONSEAPDU GetLastJCREError(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x0F;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//
//  @return . 
// NOTE: Requires Firmware Version 1.10 or greater. 
//  
LPRESPONSEAPDU GetAllAIDs(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x10;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//
//  @return . 
// NOTE: Requires Firmware Version 1.10 or greater. 
//  
LPRESPONSEAPDU GetActivationTimeRemaining(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x11;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//
//  @return . 
// NOTE: Requires Firmware Version 1.10 or greater. 
//  
LPRESPONSEAPDU GetVerifyLoadWithIdMode(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_INFO;
  l_CommandAPDU.P2   = 0x12;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;
  
  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
//
//  @return . 
// NOTE: Requires Firmware Version 1.10 or greater. 
//  
LPRESPONSEAPDU SetVerifyLoadWithIdMode(int portnum, uchar p_Mode)
{
  uchar        l_Data[32];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 0x12;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2); // PIN Length + PIN Data + Mode Byte  
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_Mode;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Gets the number of Power On Resets the device has experienced since the 
// last master erase. 
//  @return Response APDU containing the number of POR since the last 
//          master erase. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU GetPORCount(int portnum)
{
  COMMANDAPDU l_CommandAPDU;
  
  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_STATS;
  l_CommandAPDU.P2   = 0;
  l_CommandAPDU.Lc   = 0;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Sets the master PIN of the iButton. A call to SetPIN with the old PIN 
// is required before this method can be called. The PIN to be set is 
// passed in. 
// Parameter: p_lpNewMasterPin the new PIN for the iButton 
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU SetMasterPIN(int portnum, LPJIBMASTERPIN p_lpNewMasterPin)
{
  uchar            l_Data[2*(MAX_PIN_SIZE+1)];
  COMMANDAPDU     l_CommandAPDU;
  LPRESPONSEAPDU  l_lpResponseAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_ADMIN;
  l_CommandAPDU.P2   = 1;
  // PIN Length + PIN Data + PIN Length + PIN Data  
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+p_lpNewMasterPin->Len+2); 
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  // copy current PIN  
  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  // copy new PIN  
  memcpy(l_Data+g_MasterPIN.Len+1,p_lpNewMasterPin,p_lpNewMasterPin->Len+1);

  l_lpResponseAPDU = SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);

  // go ahead and set the local PIN if the command succeeded  
  if(l_lpResponseAPDU->SW == (ushort)0x9000)
    SetPIN(p_lpNewMasterPin);

  return l_lpResponseAPDU;
}

//--------------------------------------------------------------------------
// Deletes the currently selected Applet.
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU DeleteSelectedApplet(int portnum)
{
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_AID;
  l_CommandAPDU.P2   = 0;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+1); 
  l_CommandAPDU.Data = (uchar*)&g_MasterPIN;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Deletes Applet number p_AppletNumber.
// Parameter: p_AppletNumber - number of the applet to be deleted.
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU DeleteAppletByNumber(int portnum, uchar p_AppletNumber)
{
  uchar        l_Data[MAX_PIN_SIZE+2];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_AID;
  l_CommandAPDU.P2   = 1;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+2);
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  l_Data[g_MasterPIN.Len+1] = p_AppletNumber;

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Deletes the Applet with AID p_lpAID.
// Parameter: p_lpAID - AID of the applet to be deleted.
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU DeleteAppletByAID(int portnum, LPAID p_lpAID)
{
  uchar        l_Data[MAX_PIN_SIZE+MAX_AID_SIZE+2];
  COMMANDAPDU l_CommandAPDU;

  l_CommandAPDU.CLA  = JIB_APDU_CLA_COMMAND_PROC;
  l_CommandAPDU.INS  = JIB_APDU_INS_COMMAND_PROC;
  l_CommandAPDU.P1   = JIB_APDU_P1_COMMAND_AID;
  l_CommandAPDU.P2   = 2;
  l_CommandAPDU.Lc   = (uchar)(g_MasterPIN.Len+p_lpAID->Len+2); 
  l_CommandAPDU.Data = l_Data;
  l_CommandAPDU.Le   = JIB_APDU_LE_DEFAULT;

  memcpy(l_Data,(uchar*)&g_MasterPIN,g_MasterPIN.Len+1);
  memcpy(l_Data+g_MasterPIN.Len+1,p_lpAID,p_lpAID->Len+1);

  return SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);  
}

//--------------------------------------------------------------------------
// Loads the applet stored in p_lpAppletBuffer.
// Parameter: p_lpAppletBuffer  - Buffer containing the applet (ASSUMPTION:
//                                Buffer was previously loaded with the 
//                                contents of a .JiB "dot JiB" file).
//            p_AppletLen       - Length of the applet in the buffer.
//            p_lpAID           - Pointer the the applet's AID.
//  @return The ResponseAPDU containing the Status Word of the response. 
//          SW = 0x9000 indicates success. 
//  
LPRESPONSEAPDU LoadApplet(int portnum, 
                          uchar* p_lpAppletBuffer, 
                          ulong  p_AppletLen,
                          LPAID  p_lpAID)
{
  uchar *l_lpData, *l_lpOffset;
  COMMANDAPDU     l_CommandAPDU;
  LPRESPONSEAPDU  l_lpResponseAPDU;
  ulong           l_BufferSize  = p_AppletLen+sizeof(AID)+sizeof(JIBMASTERPIN);
  ulong           l_PacketSize;

  // make sure AID length fits
  if(p_lpAID->Len > MAX_AID_SIZE)
  {
    g_ResponseAPDU.Len  = 0;
    g_ResponseAPDU.SW   = ERR_API_INVALID_PARAMETER;
    return &g_ResponseAPDU;
  }
  
  if(!(l_lpOffset = (l_lpData = malloc(l_BufferSize))))
  {
    g_ResponseAPDU.Len  = 0;
    g_ResponseAPDU.SW   = ERR_API_MEMORY_ALLOCATION;
    return &g_ResponseAPDU;
  }

  memset(l_lpData,0,l_BufferSize);

  // copy the master pin first
  memcpy(l_lpOffset,(uchar*)&g_MasterPIN,sizeof(g_MasterPIN));
  l_lpOffset += sizeof(g_MasterPIN);
  l_PacketSize  = sizeof(g_MasterPIN); 

  // copy the AID
  memcpy(l_lpOffset,(uchar*)p_lpAID,p_lpAID->Len+1);
  memset(l_lpOffset+p_lpAID->Len+1,0,sizeof(AID)-(p_lpAID->Len+1));
  l_lpOffset    += sizeof(AID);
  l_PacketSize  += sizeof(AID);

  // copy the applet
  memcpy(l_lpOffset,p_lpAppletBuffer,p_AppletLen);
  l_PacketSize += p_AppletLen;
  l_lpOffset  = l_lpData;
   
  l_CommandAPDU.CLA   = JIB_APDU_CLA_LOAD_APPLET;
  l_CommandAPDU.INS   = JIB_APDU_INS_LOAD_APPLET;
  l_CommandAPDU.P1    = JIB_APDU_P1_LOAD_APPLET;
  l_CommandAPDU.P2    = JIB_APDU_UNUSED;
  l_CommandAPDU.Lc    = (uchar)((l_PacketSize > MAX_APDU_SIZE) ? MAX_APDU_SIZE:
                                                                l_PacketSize);
  l_CommandAPDU.Data  = l_lpOffset;
  l_CommandAPDU.Le    = JIB_APDU_LE_DEFAULT;

  l_lpResponseAPDU    = SendAPDU(portnum, &l_CommandAPDU, 
                                 (ushort)((l_CommandAPDU.Lc == MAX_APDU_SIZE)? 64: g_ExecTimeMS));

  if((l_lpResponseAPDU->SW == ERR_ISO_NORMAL_00)||
     (l_lpResponseAPDU->SW != ERR_NEXT_LOAD_PACKET_EXPECTED))
  {
    free(l_lpData);
    return l_lpResponseAPDU;
  }

  l_lpOffset += MAX_APDU_SIZE;
  l_PacketSize -= MAX_APDU_SIZE;
  l_CommandAPDU.P1  = JIB_APDU_P1_CONTINUE_LOAD;

  while(l_PacketSize > MAX_APDU_SIZE)
  {
    l_CommandAPDU.Data  = l_lpOffset;
    l_lpResponseAPDU    = SendAPDU(portnum, &l_CommandAPDU,
                                   (ushort)((l_lpResponseAPDU->SW == ERR_NEXT_LOAD_PACKET_EXPECTED)? 64: g_ExecTimeMS));
    
    l_lpOffset += MAX_APDU_SIZE;
    l_PacketSize -= MAX_APDU_SIZE;

    if((l_lpResponseAPDU->SW == ERR_ISO_NORMAL_00)||
       (l_lpResponseAPDU->SW != ERR_NEXT_LOAD_PACKET_EXPECTED))
      break;
  }

  if(l_PacketSize)
  {
    l_CommandAPDU.Lc    = (uchar)l_PacketSize; 
    l_CommandAPDU.Data  = l_lpOffset;
    l_lpResponseAPDU    = SendAPDU(portnum, &l_CommandAPDU,g_ExecTimeMS);
  }

  free(l_lpData);
  return l_lpResponseAPDU;  
}

//--------------------------------------------------------------------------
//
//    Set new error code.
//
void SetJiBError(ushort p_Error)
{
  g_LastError = g_ResponseAPDU.SW = p_Error;
}

//--------------------------------------------------------------------------
//
//    Get error code.
//
ushort GetJiBError()
{
  return g_LastError;
}

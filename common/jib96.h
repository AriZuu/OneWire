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

#include "ownet.h"
#include <memory.h>

#define MAX_SEND            255      // Maximum send buffer size 
#define MAX_RECEIVE         2048     // Maximum receive buffer size 
#define MAX_JIB_NUM         16       // Maximum number of CiBs on system 
#define MAX_BUF_SIZE        256 
#define MAX_PIN_SIZE        8
#define MAX_AID_SIZE        16

// maximum data bytes that can be sent in a single packet 
#define MAX_APDU_SIZE       114

#define JIB_APDU_CLA_COMMAND_PROC (uchar)0xd0
#define JIB_APDU_CLA_LOAD_APPLET  (uchar)0xd0
#define JIB_APDU_CLA_SELECT       (uchar)0x00
#define JIB_APDU_INS_COMMAND_PROC (uchar)0x95
#define JIB_APDU_INS_LOAD_APPLET  (uchar)0xa6
#define JIB_APDU_INS_SELECT       (uchar)0xa4
#define JIB_APDU_P1_LOAD_APPLET   (uchar)0x01
#define JIB_APDU_P1_CONTINUE_LOAD (uchar)0x02
#define JIB_APDU_P1_SELECT        (uchar)0x04
#define JIB_APDU_P1_COMMAND_ADMIN (uchar)0x00
#define JIB_APDU_P1_COMMAND_INFO  (uchar)0x01
#define JIB_APDU_P1_COMMAND_STATS (uchar)0x02
#define JIB_APDU_P1_COMMAND_AID   (uchar)0x03
#define JIB_APDU_LE_DEFAULT       (uchar)0x00
#define JIB_APDU_UNUSED           (uchar)0x00

// API communication error codes
#define ERR_NO_JIBS_FOUND        0xF000
#define ERR_BAD_JIB_ROM          0xF100
#define ERR_JIB_NOT_FOUND        0xF200
#define ERR_ADAPTER_NOT_FOUND    0xF300
#define ERR_COMM_FAILURE         0xF400

// General API Errors
#define ERR_API_INVALID_PARAMETER 0xFA00
#define ERR_API_MEMORY_ALLOCATION 0xFA01

// ISO Errors & JiB Errors
#define ERR_ISO_NORMAL_00             0x9000
#define ERR_NEXT_LOAD_PACKET_EXPECTED 0x6301

#define COMMIT_BUFFER_BYTES_PER_FIELD 0x09



//---------------------------------------------------------------------------
// For the current CIB implementation, the maximum outbound or inbound
// message size is 128 bytes, and the header (FIFO) data is always 8 bytes
// long for all transactions.
 
#define SEGSIZE     128    // Max seg size that we can send to the device 
#define HEADERSIZE  8      // Size of a comm layer header packet 
#define CIB_FC      0x16
#define MAX_CIB_NUM 16
#define FIFO_LEN    8

// Communications Layer OWMS Status Codes -
#define CE_RESET         0
#define CE_MSGINCOMP     1
#define CE_BADSEQ        2 
#define CE_BUFOVERRUN    3
#define CE_BADCKSUM      4
#define CE_HDRSIZE       5
#define CE_DATASIZE      6
#define CE_BADCRC        7
#define CE_DURATION      8
#define CE_FIFONOTEMPTY  9
#define CE_STANDBY       10
#define CE_RESPONSERDY   11
#define CE_RESPINCOMP    12
#define CE_NOHEADER      13
#define CE_MESSAGEOK     15
#define CE_FIRSTBIRTH    29
#define CE_CIREEXEC      30
#define CE_CMDINCOMP     31

#define CE_BPOR          0x40   // Buferred POR bit in OWMS status byte 

// Function Error Return Codes
#define ER_NOROOM    2       // No room in outbound FIFO for a message header 
#define ER_POR       3       // Constant POR status from device 
#define ER_ACCESS    4       // Error trying to access device 
#define ER_COLLIDE   5       // One-Wire Collision Error 
#define ER_ACK       6       // Failure to get device acknowledge bit 
#define ER_CRC       7       // CRC test failed 
#define ER_CHECKSUM  8       // Checksum test failed 
#define ER_NOHEADER  9       // No header found (no message to read) 
#define ER_BADHEADER 10      // Bad header form or length 
#define ER_HDRCRC    11      // Header crc does not match data segment crc 
#define ER_SEQUENCE  12      // Segments arrived out of sequence or missing 
#define ER_OVERRUN   13      // Inbound message overflowed buffer limit 
#define ER_COPRUN    14      // Co-Processor Will Not Complete 
#define ER_RERUN     15      // Prior command will not complete 
#define ER_ADAPTER   16      // Bad adapter selection  

// Error Group Codes
#define ER_SENDDATA   0x10   // Error in sending segment data 
#define ER_SENDHDR    0x20   // Error in sending a segment header 
#define ER_GETSTATUS  0X30   // Error in geting status 
#define ER_SENDSTATUS 0x40   // Error in sending status command 
#define ER_SENDRUN    0x50   // Error in sending run command 
#define ER_SENDRESET  0x60   // Error in sending reset command 
#define ER_SENDINT    0x70   // Error in sending interrupt command 
#define ER_RECVHDR    0x80   // Error in unload of segment header 
#define ER_RECVDATA   0x90   // Error in unload of segment data 
#define ER_COMMERROR  0xA0   // Comm Error response from device 

// One-Wire Command Codes -
#define OW_MICRORUN       0x87
#define OW_MICROINTERRUPT 0x77
#define OW_MICRORESET     0xDD
#define OW_STATUSREAD     0xE1
#define OW_STATUSWRITE    0xD2
#define OW_IPRREAD        0xAA
#define OW_IPRWRITE       0x0F
#define OW_FIFOREAD       0x22
#define OW_FIFOWRITE      0x2D

// CiB One-Wire Release sequence values -
#define RS_MICRORUN       0x5D73
#define RS_MICROINTERRUPT 0x6D43
#define RS_MICRORESET     0x92BC
#define RS_STATUSWRITE    0x517F
#define RS_FIFOREAD       0x624C
#define RS_FIFOWRITE      0x9DB3

#define LSB(a) (uchar) ((a) & 0xFF)
#define MSB(a) (uchar) (((a) & 0xFF00) >> 8)

//---------------------------------------------------------------------------
// Structure definitions
//
typedef struct _FIFORDWR
{
   uchar Cmd;
   uchar Len;
   uchar FIFOBuf[8];
}
FIFORDWR;

typedef struct _CMDPACKET
{
   uchar Len;
   uchar CmdByte;
   uchar GroupID;
   uchar CmdData[MAX_SEND]; 
}
CMDPACKET;

// Firmware return packet 
typedef struct _RETPACKET      
{
   uchar CSB;
   uchar GroupID;
   uchar DataLen;
   uchar CmdData[MAX_RECEIVE];
}
RETPACKET;

// Command APDU 
typedef struct _COMMANDAPDU  
{
  uchar   Header[4];
  uchar   Lc;
  uchar  *Data;
  uchar   Le;
} COMMANDAPDU, *LPCOMMANDAPDU;

// Field access defines 
#define CLA Header[0]
#define INS Header[1]
#define P1  Header[2]
#define P2  Header[3]

// Response APDU 
typedef struct _RESPONSEAPDU 
{
  ushort   Len;
  uchar   *Data;
  ushort   SW;
} RESPONSEAPDU,*LPRESPONSEAPDU;

// iButton Master PIN 
typedef struct _JIBMASTERPIN 
{
  uchar Len;
  uchar Data[MAX_PIN_SIZE];
} JIBMASTERPIN,*LPJIBMASTERPIN;

// Applet AID 
typedef struct _AID
{
  uchar Len;
  uchar Data[MAX_AID_SIZE];
} AID, *LPAID;

// API Commands 
#ifdef __cplusplus
extern "C" {
#endif

// raw communication functions
ushort SendCiBMessage(int portnum, 
                        uchar *mp, 
                        ushort mlen, 
                        uchar ExecTime);
ushort RecvCiBMessage(int portnum, 
                        uchar *mp, 
                        ushort *bufsize);
void SetMinRunTime(ushort MR);

// device specific functions
void SetDefaultExecTime(ushort p_ExecTimeMS);
LPRESPONSEAPDU SendAPDU(int portnum, 
                        LPCOMMANDAPDU  p_lpCommandAPDU, 
                        ushort         p_ExecTimeMS);
void SetPIN(LPJIBMASTERPIN p_lpMasterPIN);
LPRESPONSEAPDU Process(int portnum,
                       uchar   p_CLA, 
                       uchar   p_INS, 
                       uchar   p_P1, 
                       uchar   p_P2, 
                       uchar   p_Lc, 
                       uchar* p_lpData,
                       ushort   p_RunTimeMS);
LPRESPONSEAPDU Select(int portnum, LPAID p_lpAID);
LPRESPONSEAPDU MasterErase(int portnum);
LPRESPONSEAPDU GetFreeRam(int portnum);
LPRESPONSEAPDU GetFreeRamPerBank(int portnum);
LPRESPONSEAPDU GetFirmwareVersionString(int portnum);
LPRESPONSEAPDU GetEphemeralGCMode(int portnum);
LPRESPONSEAPDU SetEphemeralGCMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetAppletGCMode(int portnum);
LPRESPONSEAPDU SetAppletGCMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetCommandPINMode(int portnum);
LPRESPONSEAPDU SetCommandPINMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetLoadPINMode(int portnum);
LPRESPONSEAPDU SetLoadPINMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetRestoreMode(int portnum);
LPRESPONSEAPDU SetRestoreMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetExceptionMode(int portnum);
LPRESPONSEAPDU SetExceptionMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetISOExceptionMode(int portnum);
LPRESPONSEAPDU SetISOExceptionMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetErrorReportMode(int portnum);
LPRESPONSEAPDU SetErrorReportMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetCommitBufferSize(int portnum);
LPRESPONSEAPDU GetCommitBufferFields(int portnum);
LPRESPONSEAPDU SetCommitBufferSize(int portnum, short p_SizeInBytes);
LPRESPONSEAPDU SetCommitBufferFields(int portnum, short p_NumberOfFields);
LPRESPONSEAPDU GetATR(int portnum);
LPRESPONSEAPDU GetRealTimeClock(int portnum);
LPRESPONSEAPDU GetRandomBytes(int portnum, short p_NumBytes);
LPRESPONSEAPDU GetAIDByNumber(int portnum, uchar p_AIDNum);
LPRESPONSEAPDU GetLastJCREError(int portnum);
LPRESPONSEAPDU GetAllAIDs(int portnum);
LPRESPONSEAPDU GetActivationTimeRemaining(int portnum);
LPRESPONSEAPDU GetVerifyLoadWithIdMode(int portnum);
LPRESPONSEAPDU SetVerifyLoadWithIdMode(int portnum, uchar p_Mode);
LPRESPONSEAPDU GetPORCount(int portnum);
LPRESPONSEAPDU SetMasterPIN(int portnum, LPJIBMASTERPIN p_lpNewMasterPin);
LPRESPONSEAPDU DeleteSelectedApplet(int portnum);
LPRESPONSEAPDU DeleteAppletByNumber(int portnum, uchar p_AppletNumber);
LPRESPONSEAPDU DeleteAppletByAID(int portnum, LPAID p_lpAID);
LPRESPONSEAPDU LoadApplet(int portnum, 
                          uchar* p_lpAppletBuffer, 
                          ulong  p_AppletLen,
                          LPAID  p_lpAID);
void SetJiBError(ushort p_Error);
ushort GetJiBError();

#ifdef __cplusplus
}
#endif

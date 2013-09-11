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

#ifndef _ds2490comm_h_
#define _ds2490comm_h_

#include <windows.h>
#include <winioctl.h>

// single byte alignment on structures
#pragma pack(push, 1)

// Error codes
#define DS2490COMM_ERR_NOERROR               0   // an error has not yet been encountered
#define DS2490COMM_ERR_GETLASTERROR          1   // use GetLastError() for more information
#define DS2490COMM_ERR_RESULTREGISTERS       2   // use DS2490COMM_GetLastResultRegister() for more info
#define DS2490COMM_ERR_USBDEVICE             3   // error from USB device driver
#define DS2490COMM_ERR_READWRITE             4   // an I/O error occurred while communicating w/ device
#define DS2490COMM_ERR_TIMEOUT               5   // an operation timed out before completion
#define DS2490COMM_ERR_INCOMPLETEWRITE       6   // not all data could be sent for output
#define DS2490COMM_ERR_INCOMPLETEREAD        7   // not all data could be received for an input
#define DS2490COMM_ERR_INITTOUCHBYTE         8   // the touch byte thread could not be started
#define g_DS2490COMM_LAST_SHORTEDBUS         9   // 1Wire bus shorted on
#define g_DS2490COMM_LAST_EMPTYBUS           10  // 1Wire bus empty

#define	IOCTL_INBUF_SIZE	        512
#define	IOCTL_OUTBUF_SIZE	        512
#define TIMEOUT_PER_BYTE            15    //1000 modified 4/27/00 BJV

// Definition is taken from DDK
typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;	

// IOCTL codes
#define DS2490_IOCTL_VENDOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DS2490_IOCTL_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DS2490_IOCTL_DEVICE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)


// Device Information is put here
typedef struct _USB_DEVICE_INFO
{
	ULONG	DriverVersion;
	USB_DEVICE_DESCRIPTOR	Descriptor;
	UCHAR	Unit;
} USB_DEVICE_INFO, *PUSB_DEVICE_INFO;	

// Setup packet
typedef struct _SETUP_PACKET
{
   // Only for vendor specifc bits for COMM commands.
	UCHAR	RequestTypeReservedBits;
    // The Request byte
	UCHAR	Request;
    // The Value byte
	USHORT	Value;
    // The Index byte
	USHORT	Index;
    // The length of extra Data In or Out
	USHORT	Length;
   // Does the extra data go In, or Out?
   BOOLEAN DataOut;
   // If there is extra data In, it goes here.
   UCHAR   DataInBuffer[0];
} SETUP_PACKET, *PSETUP_PACKET;

// Request byte, Command Type Code Constants 
#define CONTROL_CMD                 0x00
#define COMM_CMD		               0x01
#define MODE_CMD		               0x02
#define TEST_CMD		               0x03

//
// Value field, Control commands
//
// Control Command Code Constants 
#define CTL_RESET_DEVICE		      0x0000
#define CTL_START_EXE			      0x0001
#define CTL_RESUME_EXE			      0x0002
#define CTL_HALT_EXE_IDLE		      0x0003
#define CTL_HALT_EXE_DONE		      0x0004
#define CTL_CANCEL_CMD			      0x0005
#define CTL_CANCEL_MACRO		      0x0006
#define CTL_FLUSH_COMM_CMDS	      0x0007
#define CTL_FLUSH_RCV_BUFFER	      0x0008
#define CTL_FLUSH_XMT_BUFFER	      0x0009
#define CTL_GET_COMM_CMDS		      0x000A

//
// Value field COMM Command options
//
// COMM Bits (bitwise or into COMM commands to build full value byte pairs)
// Byte 1
#define COMM_TYPE		               0x0008
#define COMM_SE                     0x0008
#define COMM_D                      0x0008
#define COMM_Z                      0x0008
#define COMM_CH                     0x0008
#define COMM_SM                     0x0008
#define COMM_R                      0x0008
#define COMM_IM                     0x0001

// Byte 2
#define COMM_PS                     0x4000
#define COMM_PST		               0x4000
#define COMM_CIB                    0x4000
#define COMM_RTS                    0x4000
#define COMM_DT                     0x2000
#define COMM_SPU		               0x1000
#define COMM_F			               0x0800
#define COMM_ICP		               0x0200
#define COMM_RST		               0x0100

// Read Straight command, special bits 
#define COMM_READ_STRAIGHT_NTF          0x0008
#define COMM_READ_STRAIGHT_ICP          0x0004
#define COMM_READ_STRAIGHT_RST          0x0002
#define COMM_READ_STRAIGHT_IM           0x0001

//
// Value field COMM Command options (0-F plus assorted bits)
//
#define COMM_ERROR_ESCAPE               0x0601
#define COMM_SET_DURATION               0x0012
#define COMM_BIT_IO                     0x0020
#define COMM_PULSE                      0x0030
#define COMM_1_WIRE_RESET               0x0042
#define COMM_BYTE_IO                    0x0052
#define COMM_MATCH_ACCESS               0x0064
#define COMM_BLOCK_IO                   0x0074
#define COMM_READ_STRAIGHT              0x0080
#define COMM_DO_RELEASE                 0x6092
#define COMM_SET_PATH                   0x00A2
#define COMM_WRITE_SRAM_PAGE            0x00B2
#define COMM_WRITE_EPROM                0x00C4
#define COMM_READ_CRC_PROT_PAGE         0x00D4
#define COMM_READ_REDIRECT_PAGE_CRC     0x21E4
#define COMM_SEARCH_ACCESS              0x00F4

// Mode Command Code Constants 
// Enable Pulse Constants
#define ENABLEPULSE_PRGE		         0x01  // strong pull-up
#define ENABLEPULSE_SPUE		         0x02  // programming pulse

// 1Wire Bus Speed Setting Constants
#define ONEWIREBUSSPEED_REGULAR        0x00
#define ONEWIREBUSSPEED_FLEXIBLE       0x01
#define ONEWIREBUSSPEED_OVERDRIVE      0x02

//
// Value field Mode Commands options
//
#define MOD_PULSE_EN                   0x0000
#define MOD_SPEED_CHANGE_EN            0x0001
#define MOD_1WIRE_SPEED                0x0002
#define MOD_STRONG_PU_DURATION         0x0003
#define MOD_PULLDOWN_SLEWRATE          0x0004
#define MOD_PROG_PULSE_DURATION        0x0005
#define MOD_WRITE1_LOWTIME             0x0006
#define MOD_DSOW0_TREC                 0x0007

//
// This is the status structure as returned by DS2490_IOCTL_STATUS.
//
typedef struct _STATUS_PACKET
{
	UCHAR	EnableFlags;
	UCHAR	OneWireSpeed;
	UCHAR	StrongPullUpDuration;
	UCHAR	ProgPulseDuration;
	UCHAR	PullDownSlewRate;
	UCHAR	Write1LowTime;
	UCHAR	DSOW0RecoveryTime;
	UCHAR	Reserved1;
	UCHAR	StatusFlags;
	UCHAR	CurrentCommCmd1;
	UCHAR	CurrentCommCmd2;
	UCHAR	CommBufferStatus;  // Buffer for COMM commands
	UCHAR	WriteBufferStatus; // Buffer we write to
	UCHAR	ReadBufferStatus;  // Buffer we read from
	UCHAR	Reserved2;
	UCHAR	Reserved3;
   // There may be up to 16 bytes here, or there may not.
   UCHAR   CommResultCodes[16];
} STATUS_PACKET, *PSTATUS_PACKET;


//
// STATUS FLAGS
//
// Enable Flags
#define ENABLEFLAGS_SPUE		            0x01  // if set Strong Pull-up to 5V enabled
#define ENABLEFLAGS_PRGE		            0x02  // if set 12V programming pulse enabled
#define ENABLEFLAGS_SPCE		            0x04  // if set a dynamic 1-Wire bus speed change through Comm. Cmd. enabled

// Device Status Flags
#define STATUSFLAGS_SPUA	               0x01  // if set Strong Pull-up is active
#define STATUSFLAGS_PRGA	               0x02  // if set a 12V programming pulse is being generated
#define STATUSFLAGS_12VP	               0x04  // if set the external 12V programming voltage is present
#define STATUSFLAGS_PMOD	               0x08  // if set the DS2490 powered from USB and external sources
#define STATUSFLAGS_HALT	               0x10  // if set the DS2490 is currently halted
#define STATUSFLAGS_IDLE	               0x20  // if set the DS2490 is currently idle

// Result Registers
#define ONEWIREDEVICEDETECT               0xA5  // 1-Wire device detected on bus
#define COMMCMDERRORRESULT_NRS            0x01  // if set 1-WIRE RESET did not reveal a Presence Pulse or SET PATH did not get a Presence Pulse from the branch to be connected
#define COMMCMDERRORRESULT_SH             0x02  // if set 1-WIRE RESET revealed a short on the 1-Wire bus or the SET PATH couln not connect a branch due to short
#define COMMCMDERRORRESULT_APP            0x04  // if set a 1-WIRE RESET revealed an Alarming Presence Pulse
#define COMMCMDERRORRESULT_VPP            0x08  // if set during a PULSE with TYPE=1 or WRITE EPROM command the 12V programming pulse not seen on 1-Wire bus
#define COMMCMDERRORRESULT_CMP            0x10  // if set there was an error reading confirmation byte of SET PATH or WRITE EPROM was unsuccessful
#define COMMCMDERRORRESULT_CRC            0x20  // if set a CRC occurred for one of the commands: WRITE SRAM PAGE, WRITE EPROM, READ EPROM, READ CRC PROT PAGE, or READ REDIRECT PAGE W/CRC
#define COMMCMDERRORRESULT_RDP            0x40  // if set READ REDIRECT PAGE WITH CRC encountered a redirected page
#define COMMCMDERRORRESULT_EOS            0x80  // if set SEARCH ACCESS with SM=1 ended sooner than expected with too few ROM IDs

// Strong Pullup 
#define SPU_MULTIPLE_MS                   128
#define SPU_DEFAULT_CODE                  512 / SPU_MULTIPLE_MS   // default Strong pullup value

// Programming Pulse 
#define PRG_MULTIPLE_US                   8               // Programming Pulse Time Multiple (Time = PRG_MULTIPLE_US * DurationCode)
#define PRG_DEFAULT_CODE                  512 / PRG_MULTIPLE_US   // default Programming pulse value

// Support functions
SMALLINT DS2490Detect(HANDLE hDevice);
SMALLINT DS2490GetStatus(HANDLE hDevice, STATUS_PACKET *status, BYTE *pResultSize);
SMALLINT DS2490ShortCheck(HANDLE hDevice, SMALLINT *present,SMALLINT *vpp);
SMALLINT DS2490Reset(HANDLE hDevice);
SMALLINT DS2490Read(HANDLE hDevice, BYTE *buffer, WORD *pnBytes);
SMALLINT DS2490Write(HANDLE hDevice, BYTE *buffer, WORD *pnBytes);
SMALLINT DS2490HaltPulse(HANDLE hDevice);
SMALLINT AdapterRecover(int portnum);

#endif // _ds2490comm_h_
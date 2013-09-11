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
//  ibsha33.h - Include file for SHA iButton (DS2432)
//
//  Version: 2.00
//

#include <string.h>
#include <stdlib.h>
#include <time.h>

// Typedefs
#ifndef OW_UCHAR
   #define OW_UCHAR
   typedef unsigned char  uchar;
   #ifdef WIN32
      typedef unsigned short ushort;
      typedef unsigned long  ulong;
   #endif
#endif

// output level contants
#define LV_ALWAYS                   2
#define LV_OPTIONAL                 1
#define LV_VERBOSE                  0

// SHA iButton commands
#define SHA_COMPUTE_FIRST_SCRT      0x0F
#define SHA_COMPUTE_NEXT_SCRT       0xF0
#define SHA_VALIDATE_PAGE           0x3C
#define SHA_SIGN_PAGE               0xC3
#define SHA_COMPUTE_CHALLENGE       0xCC
#define SHA_AUTHENTICATE_HOST       0xAA
#define CMD_READ_MEMORY             0xF0
#define CMD_MATCH_SCRATCH           0x3C
#define CMD_WRITE_SCRATCH           0x0F
#define CMD_READ_SCRATCH            0xAA
#define CMD_ERASE_SCRATCH           0xC3
#define CMD_COPY_SCRATCH            0x55
#define CMD_READ_AUTH_PAGE          0xA5
#define CMD_COMPUTE_SHA             0x33
// other
#define IB_ROVING                   0
#define IB_COPR                     1 
#define SHA_FAM                     0x18
#define MAX_TRANS_ERROR             5

// exit codes
#define EXIT_SUCCESS                0
#define EXIT_ERROR                  1
#define EXIT_ERROR_WRITE            2
#define EXIT_ERROR_MONEY_SCRT       3
#define EXIT_ERROR_AUTH_SCRT        4
#define EXIT_ERROR_NO_SHA           5

// certificate 
typedef struct
{
   ushort obj_descriptor;   // descriptor of object 0x0501
   uchar  obj_len;          // object length
   
   uchar  pkt_len;          // packet length
   ushort trans_id;         // transaction id
   ushort code_mult;        // monetary unit code & multiplier
   uchar  balance[3];       // monetary balance (should be 3 bytes)
   uchar  money_mac[20];    // monetary mac
   uchar  cert_type;        // certificate type & algorithm
   uchar  pkt_cont_ptr;     // packet continuation pointer
   uchar  inv_crc[2];
    
   ulong  write_cycle_cnt;  // write cycle counter
   uchar  money_page;       // monetary page
   uchar  serial_num[7];    // ibutton serial number (no CRC8)

   char   file_name[4];     // file name of account
   uchar  file_ext;         // file extension of account
   uchar  rom_id[8];        // device rom ID
   uchar  auth_mac[20];     // authentication mac

   uchar  constant_data[20];// system constant data
   char   user_data[29];    // fixed user data

} eCertificate;

// structure to hold each state in the StateMachine
typedef struct
{
   int  Step;
   char StepDescription[50];
} Script;

// structure to hold transaction state
typedef struct
{
   short portnum[2];   // 1-wire port numbers for co-processor/roving
   
   int dev;            // current device select co-processor(1)/roving(0)

   uchar copr_rom[8];  // co-processor rom number
   uchar rov_rom[8];   // roving rom number

   uchar provider[5];  // provider file name (on roving);
   uchar copr_file[5]; // co-processor info filename

   char user_data[255]; // user information buffer

   // secret pages
   int c_mmaster_scrt, c_amaster_scrt, c_udevice_scrt; 
   int r_udevice_scrt, t_udevice_scrt; 
    
   // internal state of sha engine
   ushort address;
   uchar  sha_cmd;
   ulong  target_secret; 
   uchar  randnum[3];
   uchar  temp_buffer[64];
   int    buf_len;

} TransState;

// Function Prototypes
int ReadScratchSHAEE(int,ushort *,uchar *,uchar *);
int WriteScratchSHAEE(int,ushort,uchar *,int);
int ReadMem(int,ushort,uchar *);
int LoadFirSecret(int,ushort, uchar *,int);
int CopyScratchSHAEE(int,ushort,uchar *,uchar *, uchar *);
int NextSecret(int,ushort,uchar *);
int ReadAuthPageSHAEE(int,ushort,uchar *,uchar *,uchar *,uchar *);
int SelectSHAEE(int);
void ComputeSHAEE(unsigned int *,long *,long *, long *, long *,long *);
long KTN(int);
long NLF(long,long,long,int);




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
// owm.h - Include file for 1-Wire Master link and session
//         owmses.c, owmlnk.c
//
// Version: 1.00
//
// History: 
//


// Register address definitions
#define OWM_COMMAND          0
#define OWM_TRANSMIT_BUFFER  1
#define OWM_RECEIVE_BUFFER   1
#define OWM_INTERRUPT_FLAGS  2
#define OWM_INTERRUPT_ENABLE 3
#define OWM_CLOCK_DIVISOR    4
#define OWM_CONTROL          5
#define OWM_HYPER_DRIVE      6

// Clock Divisor Mask
#define OWM_CLK_EN_MASK      0x80

// Command Bit Masks
#define OWM_OW_IN_MASK       0x08
#define OWM_FOW_MASK         0x04
#define OWM_SRA_MASK         0x02
#define OWM_1WR_MASK         0x01

// Control Bit Masks
#define OWM_EOWMI_MASK       0x80
#define OWM_OD_MASK          0x40
#define OWM_BIT_CTL_MASK     0x20
#define OWM_STP_SPLY_MASK    0x10
#define OWM_STPEN_MASK       0x08
#define OWM_EN_FOW_MASK      0x04
#define OWM_PPM_MASK         0x02
#define OWM_LLM_MASK         0x01

// Interrupt Flags
#define OWM_OW_LOW_MASK      0x80
#define OWM_OW_SHORT_MASK    0x40
#define OWM_RSRF_MASK        0x20
#define OWM_RBF_MASK         0x10
#define OWM_TEMT_MASK        0x08
#define OWM_TBE_MASK         0x04
#define OWM_PDR_MASK         0x02
#define OWM_PD_MASK          0x01

// Interrupt Enables
#define OWM_EOWL_MASK        0x80
#define OWM_EOWSH_MASK       0x40
#define OWM_ERSF_MASK        0x20
#define OWM_ERBF_MASK        0x10
#define OWM_ETMT_MASK        0x08
#define OWM_ETBE_MASK        0x04
#define OWM_EPD_MASK         0x01

/* Modify this line to match your intended clock/crystal speed */
#define OWM_SYSTEM_CLOCK  14745600

// Calculate what prescale and divisor we should use in 1-Wire Master Initialization
#define OWM_SYSTEM_CLOCK_MOD (OWM_SYSTEM_CLOCK/1000000)

#if     (OWM_SYSTEM_CLOCK_MOD < 4)
#define OWM_DIVISOR 0x01 // 3
#elif   (OWM_SYSTEM_CLOCK_MOD < 5)
#define OWM_DIVISOR 0x08 // 4
#elif   (OWM_SYSTEM_CLOCK_MOD < 6)
#define OWM_DIVISOR 0x02 // 5
#elif   (OWM_SYSTEM_CLOCK_MOD < 7)
#define OWM_DIVISOR 0x05 // 6
#elif   (OWM_SYSTEM_CLOCK_MOD < 8)
#define OWM_DIVISOR 0x03 // 7
#elif   (OWM_SYSTEM_CLOCK_MOD < 10)
#define OWM_DIVISOR 0x0C // 8
#elif   (OWM_SYSTEM_CLOCK_MOD < 12)
#define OWM_DIVISOR 0x06 // 10
#elif   (OWM_SYSTEM_CLOCK_MOD < 14)
#define OWM_DIVISOR 0x09 // 12
#elif   (OWM_SYSTEM_CLOCK_MOD < 16)
#define OWM_DIVISOR 0x07 // 14
#elif   (OWM_SYSTEM_CLOCK_MOD < 20)
#define OWM_DIVISOR 0x10 // 16
#elif   (OWM_SYSTEM_CLOCK_MOD < 24)
#define OWM_DIVISOR 0x0A // 20
#elif   (OWM_SYSTEM_CLOCK_MOD < 28)
#define OWM_DIVISOR 0x0D // 24
#elif   (OWM_SYSTEM_CLOCK_MOD < 32)
#define OWM_DIVISOR 0x0B // 28
#elif   (OWM_SYSTEM_CLOCK_MOD < 40)
#define OWM_DIVISOR 0x14 // 32
#elif   (OWM_SYSTEM_CLOCK_MOD < 48)
#define OWM_DIVISOR 0x0E // 40
#elif   (OWM_SYSTEM_CLOCK_MOD < 56)
#define OWM_DIVISOR 0x11 // 48
#elif   (OWM_SYSTEM_CLOCK_MOD < 64)
#define OWM_DIVISOR 0x0F // 56
#elif   (OWM_SYSTEM_CLOCK_MOD < 80)
#define OWM_DIVISOR 0x18 // 64
#elif   (OWM_SYSTEM_CLOCK_MOD < 96)
#define OWM_DIVISOR 0x12 // 80
#elif   (OWM_SYSTEM_CLOCK_MOD < 112)
#define OWM_DIVISOR 0x15 // 96
#elif   (OWM_SYSTEM_CLOCK_MOD < 128)
#define OWM_DIVISOR 0x13 // 112
#else
#define OWM_DIVISOR 0x1C // 128
#endif



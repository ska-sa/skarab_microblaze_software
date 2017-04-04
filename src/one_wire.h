
#ifndef ONE_WIRE_H_
#define ONE_WIRE_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : one_wire.h
* ------------------------------------------------------------------------------
*  COMPANY              : PERALEX ELECTRONIC (PTY) LTD
* ------------------------------------------------------------------------------
*  COPYRIGHT NOTICE :
*
*  The copyright, manufacturing and patent rights stemming from this
*  document in any form are vested in PERALEX ELECTRONICS (PTY) LTD.
*
*  (c) Peralex 2011
*
*  PERALEX ELECTRONICS (PTY) LTD has ceded these rights to its clients
*  where contractually agreed.
* ------------------------------------------------------------------------------
*  DESCRIPTION :
*
*  This file contains the definitions and constants for accessing the One Wire
*  controller over the Wishbone bus.
* ------------------------------------------------------------------------------*/

#include <stdio.h>
#include "xparameters.h"
#include "xil_types.h"
#include "constant_defs.h"
#include "delay.h"

#define ONE_WIRE_CTL_DAT_MSK           0x1  // data bit
#define ONE_WIRE_CTL_DAT_OFST          (0)
#define ONE_WIRE_CTL_RST_MSK           0x2  // reset
#define ONE_WIRE_CTL_RST_OFST          (1)
#define ONE_WIRE_CTL_OVD_MSK           0x4  // overdrive
#define ONE_WIRE_CTL_OVD_OFST          (2)
#define ONE_WIRE_CTL_CYC_MSK           0x8  // cycle
#define ONE_WIRE_CTL_CYC_OFST          (3)
#define ONE_WIRE_CTL_PWR_MSK           0x10  // power (strong pull-up), if there is a single 1-wire line
#define ONE_WIRE_CTL_PWR_OFST          (5)
#define ONE_WIRE_CTL_IRQ_MSK           0x40  // irq status
#define ONE_WIRE_CTL_IRQ_OFST          (6)
#define ONE_WIRE_CTL_IEN_MSK           0x80  // irq enable
#define ONE_WIRE_CTL_IEN_OFST          (7)

#define ONE_WIRE_CTL_SEL_MSK           (0x00000f00)  // port select number
#define ONE_WIRE_CTL_SEL_OFST          (8)

#define ONE_WIRE_CTL_POWER_MSK         (0xffff0000)  // power (strong pull-up), if there is more than one 1-wire line
#define ONE_WIRE_CTL_POWER_OFST        (16)

#define DS2433_MODEL 					0x23

#define ONE_WIRE_READ_ROM				0x33
#define ONE_WIRE_MATCH_ROM				0x55
#define ONE_WIRE_SEARCH_ROM				0xF0
#define ONE_WIRE_SKIP_ROM				0xCC
#define ONE_WIRE_RESUME					0xA5

#define ONE_WIRE_WRITE_SCRATCHPAD 		0x0F
#define ONE_WIRE_READ_SCRATCHPAD  		0xAA
#define ONE_WIRE_COPY_SCRATCHPAD  		0x55
#define ONE_WIRE_READ_MEMORY      		0xF0

#define ONE_WIRE_PF_FLAG				0x20
#define ONE_WIRE_AA_FLAG				0x80

u32 uPower;
u16 uEnableInterrupts;
u16 uEnableOverdrive;
u16 uROM_NO[8];
u16 uLastDiscrepancy;
u16 uLastFamilyDiscrepancy;
u16 uLastDeviceFlag;

/*static const u8 dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};*/


void OneWireInit();
void OneWireEnableStrongPullup(u16 uEnable, u16 uOneWirePort);
int OneWireReset(u16 uOneWirePort);
void OneWireWriteBit(u16 uBit, u16 uOneWirePort);
u16 OneWireReadBit(u16 uOneWirePort);
void OneWireWrite(u16 uByte, u16 uOneWirePort);
u16 OneWireRead(u16 uOneWirePort);
void OneWireWriteBytes(u16 *uWriteBuffer, u16 uCount, u16 uOneWirePort);
void OneWireReadBytes(u16 *uReadBuffer, u16 uCount, u16 uOneWirePort);
void OneWireSelect(u16 uRom[8], u16 uOneWirePort);
void OneWireSkip(u16 uOneWirePort);
int OneWireReadRom(u16 uRom[8], u16 uOneWirePort);
void OneWireResume(u16 uOneWirePort);
void OneWireResetSearch();
void OneWireTargetSearch(u16 uFamilyCode);
int OneWireSearch(u16 *uNewAddr, u16 uOneWirePort);
u16 OneWireCrc8(u16 *uAddr, u16 uLen);

int DS2433WriteMem(u16 * uDeviceAddress, u16 uSkipRomAddress, u16 * uMemBuffer, u16 uBufferSize, u16 uTA1, u16 uTA2, u16 uOneWirePort);
int DS2433ReadMem(u16 * uDeviceAddress, u16 uSkipRomAddress, u16 * uMemBuffer, u16 uBufferSize, u16 uTA1, u16 uTA2, u16 uOneWirePort);


#endif

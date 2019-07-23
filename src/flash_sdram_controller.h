
#ifndef FLASH_SDRAM_CONTROLLER_H_
#define FLASH_SDRAM_CONTROLLER_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : flash_sdram_controller.h
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
*  This file contains the definitions and constants for accessing the flash and
*  SDRAM controller over the Wishbone bus. This is used for in-system reprogramming of
*  the NOR flash and boot SDRAM.
* ------------------------------------------------------------------------------*/

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_MODE      0x0
#define SDRAM_PROGRAM_MODE  0x1
#define SDRAM_READ_MODE   0x2
#define FLASH_OUTPUT_ENABLE 0x4

#define CLEAR_SDRAM         0x1
#define FINISHED_WRITING_TO_SDRAM 0x2
#define ABOUT_TO_BOOT_FROM_SDRAM  0x4
#define FINISHED_BOOTING_FROM_SDRAM 0x8
#define RESET_SDRAM_READ_ADDRESS  0x10
#define DEBUG_SDRAM_READ_MODE   0x20
#define BOOTING_FROM_SDRAM      0x40
#define WRITE_TO_SDRAM_STALL    0x80
#define CONTINUITY_TEST_MODE    0x100
#define CONTINUITY_TEST_LOW_OUTPUT  0x200
#define CONTINUITY_TEST_HIGH_OUTPUT 0x400

#define FLASH_SDRAM_MODE_REG_ADDRESS      0x4000
#define FLASH_SDRAM_UPPER_ADDRESS_REG_ADDRESS 0x4004
#define FLASH_SDRAM_CONFIG_IO_REG_ADDRESS   0x4008
#define FLASH_SDRAM_GBE_STATS_REG_ADDRESS   0x400C
#define FLASH_CONTINUITY_TEST_OUTPUT_REG    0x4020

#define FLASH_SDRAM_WB_PROGRAM_EN_REG_ADDRESS       0x4024  /* enables the programming of the SDRAM via the wishbone bus */
                                                            /* as opposed to the alternative method via the fpga fabric port. */
                                                            /* Set to zero to revert back to original programming via fabric port */
#define FLASH_SDRAM_WB_PROGRAM_DATA_WR_REG_ADDRESS  0x4028  /* The 32-bit data words to be written to the SDRAM are placed in this register */
#define FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS      0x402C  /* Control Register: bit 0: '1' = Set by firmware to ack reception of data word. Cleared by microblaze */
                                                            /*                   bit 1: microblaze sets to '1' to indicate start SDRAM programming */
                                                            /*                   bit 2: microblaze sets to '1' to indicate done SDRAM programming */

#define FLASH_READ_ARRAY          0xFF
#define FLASH_READ_DEVICE_IDENTIFIER    0x90
#define FLASH_CFI_QUERY           0x98
#define FLASH_READ_STATUS_REGISTER      0x70
#define FLASH_CLEAR_STATUS_REGISTER     0x50
#define FLASH_WORD_PROGRAM          0x41
#define FLASH_BUFFERED_PROGRAM        0xE9
#define FLASH_BUFFERED_PROGRAM_CONFIRM    0xD0
#define FLASH_BUFFERED_ENHANCED_PROGRAM       0x80
#define FLASH_BUFFERED_ENHANCED_PROGRAM_CONFIRM   0xD0
#define FLASH_BLOCK_ERASE         0x20
#define FLASH_BLOCK_ERASE_CONFIRM     0xD0
#define FLASH_PROGRAM_SUSPEND       0xB0
#define FLASH_PROGRAM_RESUME        0xD0
#define FLASH_LOCK_BLOCK          0x60
#define FLASH_LOCK_BLOCK_CONFIRM      0x01
#define FLASH_UNLOCK_BLOCK          0x60
#define FLASH_UNLOCK_BLOCK_CONFIRM      0xD0
#define FLASH_LOCK_DOWN_BLOCK       0x60
#define FLASH_LOCK_DOWN_BLOCK_CONFIRM   0x2F
#define FLASH_SET_CONFIG_REGISTER     0x60
#define FLASH_SET_CONFIG_REGISTER_CONFIRM 0x03
#define FLASH_SET_EXTENDED_CONFIG_REGISTER      0x60
#define FLASH_SET_EXTENDED_CONFIG_REGISTER_CONFIRM  0x04
#define FLASH_PROGRAM_OTP_AREA            0xC0
#define FLASH_BLANK_CHECK         0xBC
#define FLASH_BLANK_CHECK_CONFIRM     0xD0

#define FLASH_STATUS_WRITE_STATUS     0x80
#define FLASH_STATUS_ERASE_STATUS     0x20
#define FLASH_STATUS_PROGRAM_STATUS     0x10
#define FLASH_STATUS_VPP_STATUS       0x08
#define FLASH_STATUS_BLOCK_LOCK_ERROR   0x02
#define FLASH_STATUS_PROGRAM_STATUS_2   0x300

#define FLASH_PARTITION_ADDRESS_MASK    0x3800000
#define FLASH_BLOCK_ADDRESS_MASK      0x3FE0000

#define FLASH_TIMEOUT       100
#define FLASH_PROGRAM_TIMEOUT   1000
#define FLASH_ERASE_TIMEOUT     100000

void SetOutputMode(u8 uOutputMode, u8 uFlashOutputEnable);
void SetUpperAddressBits(u32 uAddress);
void ClearSdram();
void FinishedWritingToSdram();
void AboutToBootFromSdram();
void FinishedBootingFromSdram();
void ResetSdramReadAddress();
void ClearEthernetStatics();
u16 GetNumEthernetFrames();
u8 GetNumEthernetBadFrames();
u8 GetNumEthernetOverloadFrames();

// Nothing for Microblaze to do for SDRAM programming mode because
// packets are sent directly from the 1GBE fabric interface to the
// FLASH SDRAM controller (Microblaze and Wishbone bus not involved)
void EnableDebugSdramReadMode(u8 uEnable);
u32 ReadSdramWord();

u32 ContinuityTest(u32 uOutput);

void sudo_reboot_now( void );

void WriteFlashWord(u32 uAddress, u16 uData);
u16 ReadFlashWord(u32 uAddress);

void ReadArrayCmd(u32 uPartitionAddress);
u16 ReadStatusRegisterCmd(u32 uPartitionAddress);
void ClearStatusRegisterCmd(u32 uAddress);
void ProgramReadConfigurationRegisterCmd(u32 uRCRValue);
u16 ReadReadConfigurationRegisterCmd(u32 uPartitionAddress);
void ProgramExtendedConfigurationRegisterCmd(u32 uExtendedValue);
void BlockEraseCmd(u32 uBlockAddress);
void UnlockBlockCmd(u32 uBlockAddress);

// NOTE: cannot do single word flash programming in object mode
// Only available in control mode where only write up to first 512
// bytes of 1KB region
// FPGA images stored in object mode because needs to be continuous
// and is much larger than 512 bytes
// FPGA images will have to be programmed using buffered programming
// A3 = '0' selects A half (control and object)
// A3 = '1' selects B half (object only)
// Buffered programming puts in object mode
// Single word programming puts in control mode
void WordProgramCmd(u32 uAddress, u16 uData);


// High level FLASH commands
u16 ReadWord(u32 uAddress);
int ProgramWord(u32 uAddress, u16 uData);
int ProgramBuffer(u32 uAddress, u16 * puDataArray, u16 uTotalNumWords, u16 uNumWords, u16 uStartProgram, u16 uFinishProgram);
// Maximum buffer size is 512 words
int EraseBlock(u32 uBlockAddress);

#ifdef __cplusplus
}
#endif
#endif

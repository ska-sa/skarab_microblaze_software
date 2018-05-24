
#ifndef ISP_SPI_CONTROLLER_H_
#define ISP_SPI_CONTROLLER_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : isp_spi_controller.h
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
*  This file contains the definitions and constants for accessing the ISP SPI
*  controller over the Wishbone bus. This is used for in-system reprogramming of
*  the Spartan 3AN FPGA SPI Flash.
* ------------------------------------------------------------------------------*/

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ISP_SPI_ADDRESS_MASK_LOW	0xFFFF

#define ISP_SPI_WRITE_BYTE			0x100
#define ISP_SPI_READ_BYTE			0x200
#define ISP_SPI_CMD_FAST_READ		0x0
#define ISP_SPI_CMD_BUFFER_WRITE	0x400
#define ISP_SPI_CMD_BUFFER_PROGRAM	0x800
#define ISP_SPI_CMD_SECTOR_ERASE	0xC00
#define ISP_SPI_CMD_READ_STATUS		0x1000
#define ISP_SPI_START_TRANS			0x2000
#define ISP_SPI_TRANS_COMPLETE		0x100

#ifdef REDUCED_CLK_ARCH
/* for 39.0625MHz clock... */
#define ISP_SPI_TIMEOUT           25000
#define ISP_SPI_PROGRAM_TIMEOUT   25000
#define ISP_SPI_ERASE_TIMEOUT     250000
#else
/* for 156.25MHz clock... */
#define ISP_SPI_TIMEOUT           100000
#define ISP_SPI_PROGRAM_TIMEOUT   100000
#define ISP_SPI_ERASE_TIMEOUT     1000000
#endif


// DEFINITIONS FOR STATUS COMMAND BIT POSITIONS
#define ISP_SPI_STATUS_READY		0x80

#define ISP_SPI_ADDRESS_REG_ADDRESS		0x4018
#define ISP_SPI_DATA_CTRL_REG_ADDRESS	0x401C

u8 IsISPSPIReady();
int ISPSPIReadPage(u32 uAddress, u16 * puDataArray, u16 uNumBytes);
int ISPSPIProgramPage(u32 uAddress, u16 * puDataArray, u16 uNumBytes);
int ISPSPIEraseSector(u32 uSectorAddress);

#ifdef __cplusplus
}
#endif
#endif

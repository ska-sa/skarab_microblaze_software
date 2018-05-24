/**------------------------------------------------------------------------------
*  FILE NAME            : isp_spi_controller.c
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
*  This file contains the implementation of functions to access the ISP SPI
*  controller over the Wishbone bus.
* ------------------------------------------------------------------------------*/

#include <xil_io.h>
#include <xstatus.h>
#include <xparameters.h>
#include <xil_types.h>

#include "isp_spi_controller.h"
#include "constant_defs.h"
#include "delay.h"


//=================================================================================
//	IsISPSPIReady
//--------------------------------------------------------------------------------
//	This method reads the status from the Spartan 3AN SPI flash and checks whether
//	it is ready.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	None
//
//	Return
//	------
//	1 = ready, 0 = not ready
//=================================================================================
u8 IsISPSPIReady()
{
	u32 uAddress = 0x0;
	u32 uNumBytes = 0x1;
	u32 uReg;
	u32 uTimeout = 0x0;

	// Write address and number of bytes
	uReg = uAddress | (uNumBytes << 23);
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_ADDRESS_REG_ADDRESS, uReg);

	// Write the command
	uReg = ISP_SPI_CMD_READ_STATUS;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	// Set the start bit
	uReg = uReg | ISP_SPI_START_TRANS;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	//  Wait for transaction to complete
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS);
		uTimeout++;
	}
	while(((uReg & ISP_SPI_TRANS_COMPLETE) == 0x0) && (uTimeout < ISP_SPI_TIMEOUT));

	if (uTimeout == ISP_SPI_TIMEOUT)
	{
		xil_printf("Status timeout.\r\n");
		return 0x0;
	}

	// Set the read strobe bit
	uReg = ISP_SPI_READ_BYTE;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	// Read the byte read
	uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS);

	//xil_printf("Status register: %x\r\n", uReg);

	if ((uReg & ISP_SPI_STATUS_READY) == 0x0)
		return 0x0;
	else
		return 0x1;

}

//=================================================================================
//	ISPSPIReadPage
//--------------------------------------------------------------------------------
//	This method reads a page (up to 264 bytes) from Spartan 3AN SPI flash.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uAddress	IN		Address of page that want to read
//	puDataArray	OUT		Pointer to array of data to read into
//	uNumBytes	IN		Number of bytes to read
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ISPSPIReadPage(u32 uAddress, u16 * puDataArray, u16 uNumBytes)
{
	u32 uReg;
	u32 uTimeout = 0x0;
	u16 uByteCount;

	// Program the address and number of bytes
	uReg = uAddress | (uNumBytes << 23);
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_ADDRESS_REG_ADDRESS, uReg);

	// Write the command
	uReg = ISP_SPI_CMD_FAST_READ;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	// Set the start bit
	uReg = uReg | ISP_SPI_START_TRANS;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	//  Wait for transaction to complete
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS);
		Delay(10);
		uTimeout++;
	}
	while(((uReg & ISP_SPI_TRANS_COMPLETE) == 0x0) && (uTimeout < ISP_SPI_TIMEOUT));

	if (uTimeout == ISP_SPI_TIMEOUT)
	{
		xil_printf("Timeout waiting for page read to start.\r\n");
		return XST_FAILURE;
	}

	// Transfer the data from the internal FPGA FIFO in the ISP SPI controller
	for (uByteCount = 0x0; uByteCount < uNumBytes; uByteCount++)
	{
		uReg = 0x0;
		Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

		// Strobe the read bit
		uReg = uReg | ISP_SPI_READ_BYTE;
		Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

		// Read the byte from the FIFO
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS);

		puDataArray[uByteCount] = uReg & 0xFF;
		//xil_printf("CNT: %x READ: %x\r\n", uByteCount, puDataArray[uByteCount]);
	}

	return XST_SUCCESS;
}

//=================================================================================
//	ISPSPIProgramPage
//--------------------------------------------------------------------------------
//	This method programs a page (up to 264 bytes) to Spartan 3AN SPI flash.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uAddress	IN		Address of page that want to program
//	puDataArray	IN		Pointer to array of data to program
//	uNumBytes	IN		Number of bytes to program
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ISPSPIProgramPage(u32 uAddress, u16 * puDataArray, u16 uNumBytes)
{
	u16 uByteCount;
	u32 uReg;
	u32 uTimeout =0x0;
	u8 uISPReady = 0x0;

	// Transfer the data to the internal FPGA FIFO
	for (uByteCount = 0x0; uByteCount < uNumBytes; uByteCount++)
	{
		uReg = puDataArray[uByteCount];
		Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

		// Strobe the write bit
		uReg = uReg | ISP_SPI_WRITE_BYTE;
		Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);
	}

	// Used to be a read here although not sure why?

	// Write the data to the ISP SDRAM buffer
	// Upper address set to 0x0 while programming to SDRAM buffer
	uReg = (uAddress & ISP_SPI_ADDRESS_MASK_LOW) | (uNumBytes << 23);
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_ADDRESS_REG_ADDRESS, uReg);

	// Set the command
	uReg = ISP_SPI_CMD_BUFFER_WRITE;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	// Set the start bit
	uReg = uReg | ISP_SPI_START_TRANS;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	//  Wait for transaction to complete
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS);
		uTimeout++;
	}
	while(((uReg & ISP_SPI_TRANS_COMPLETE) == 0x0) && (uTimeout < ISP_SPI_TIMEOUT));

	if (uTimeout == ISP_SPI_TIMEOUT)
	{
		xil_printf("Timeout while transferring data to ISP SDRAM buffer.\r\n");
		return XST_FAILURE;
	}

	// Program the ISP SDRAM buffer - full address used (not masked)
	uReg = uAddress | (uNumBytes << 23);
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_ADDRESS_REG_ADDRESS, uReg);

	// Set the command
	uReg = ISP_SPI_CMD_BUFFER_PROGRAM;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	// Set the start bit
	uReg = uReg | ISP_SPI_START_TRANS;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	//  Wait for transaction to complete
	uTimeout = 0x0;
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS);
		uTimeout++;
	}
	while(((uReg & ISP_SPI_TRANS_COMPLETE) == 0x0) && (uTimeout < ISP_SPI_TIMEOUT));

	if (uTimeout == ISP_SPI_TIMEOUT)
	{
		xil_printf("Timeout while initiating programming.\r\n");
		return XST_FAILURE;
	}

	//  Wait for programming to complete
	uTimeout = 0x0;
	do
	{
		uISPReady = IsISPSPIReady();
		Delay(10);
		uTimeout++;
	}
	while((uISPReady == 0x0) && (uTimeout < ISP_SPI_PROGRAM_TIMEOUT));

	if (uTimeout == ISP_SPI_PROGRAM_TIMEOUT)
	{
		xil_printf("Timeout waiting for programming to complete.\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

//=================================================================================
//	ISPSPIEraseSector
//--------------------------------------------------------------------------------
//	This method erases a Spartan 3AN SPI flash.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uSectorAddress	IN	Address of sector to erase
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ISPSPIEraseSector(u32 uSectorAddress)
{

	//unsigned uAddressHigh;
	//unsigned uAddressLow;
	//unsigned uCtlData;
	//unsigned uStatusReg;
	//unsigned uTimeOut = 0x0;
	//bool bSuccess;

	u32 uReg;
	u32 uTimeout = 0x0;
	u8 uISPReady = 0x0;

	// Program the address
	uReg = uSectorAddress;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_ADDRESS_REG_ADDRESS, uReg);

	// Set the command
	uReg = ISP_SPI_CMD_SECTOR_ERASE;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	// Set the start bit
	uReg = uReg | ISP_SPI_START_TRANS;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS, uReg);

	//  Wait for transaction to complete
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ISP_SPI_DATA_CTRL_REG_ADDRESS);
		uTimeout++;
	}
	while(((uReg & ISP_SPI_TRANS_COMPLETE) == 0x0) && (uTimeout < ISP_SPI_TIMEOUT));

	if (uTimeout == ISP_SPI_TIMEOUT)
	{
		xil_printf("Sector erase command timeout.\r\n");
		return XST_FAILURE;
	}

	//  Wait for erasing to complete
	uTimeout = 0x0;
	do
	{
		uISPReady = IsISPSPIReady();
		Delay(10);
		uTimeout++;
	}
	while((uISPReady == 0x0) && (uTimeout < ISP_SPI_ERASE_TIMEOUT));

	if (uTimeout == ISP_SPI_ERASE_TIMEOUT)
	{
		xil_printf("Sector erase timeout.\r\n");
		return XST_FAILURE;
	}

	//xil_printf("SECT ERASE SUCCESS!\r\n");
	return XST_SUCCESS;

}

/**------------------------------------------------------------------------------
*  FILE NAME            : one_wire.c
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
*  This file contains the implementation of functions to access the one wire
*  controller over the Wishbone bus.
* ------------------------------------------------------------------------------*/

/******************************************************************************
*                                                                             *
* Minimalistic 1-wire (onewire) master with Avalon MM bus interface           *
* Copyright (C) 2010  Iztok Jeras                                             *
* Since the code is based on an Altera app note, I kept their license.        *
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2008 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
*                                                                             *
******************************************************************************/

#include "one_wire.h"


//=================================================================================
//	OneWireInit
//--------------------------------------------------------------------------------
//	This method initialises the one wire variables to values.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	None
//
//	Return
//	------
//	None
//=================================================================================
void OneWireInit()
{
	uPower = 0x0;
	uEnableInterrupts = 0x0;
	uEnableOverdrive = 0x0;
}

//=================================================================================
//	OneWireEnableStrongPullup
//--------------------------------------------------------------------------------
//	This method is used to set and clear the strong pull-up enable bit for the selected
//	one wire interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uEnable			IN	1 = enable, 0 = disable
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	None
//=================================================================================
void OneWireEnableStrongPullup(u16 uEnable, u16 uOneWirePort)
{
	u32 uMask;
	u32 uReg;

	if (uEnable == 1)
	{
		uMask = (0x1 << uOneWirePort);
		uPower = uPower | uMask;
	}
	else
	{
		uMask = !(0x1 << uOneWirePort);
		uPower = uPower & uMask;
	}

	uPower = uPower & 0xFFFF;

	uReg = uPower << ONE_WIRE_CTL_POWER_OFST;
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_WIRE_ADDR, uReg);
}

//=================================================================================
//	OneWireReset
//--------------------------------------------------------------------------------
//	This method performs a one-wire reset.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uOneWirePort	IN	Selected one wire port to reset
//
//	Return
//	------
//	XST_SUCCESS if present pulse detected
//=================================================================================
int OneWireReset(u16 uOneWirePort)
{
	u32 uReg;

  xil_printf("RST[");

	//uReg = uPower << ONE_WIRE_CTL_POWER_OFST;
	uReg = 0x0;
	uReg = uReg | (uOneWirePort << ONE_WIRE_CTL_SEL_OFST);

	if (uEnableInterrupts == 1)
		uReg = uReg | ONE_WIRE_CTL_IEN_MSK;

	uReg = uReg | ONE_WIRE_CTL_CYC_MSK;

	if (uEnableOverdrive == 1)
		uReg = uReg | ONE_WIRE_CTL_OVD_MSK;

	uReg = uReg | ONE_WIRE_CTL_RST_MSK;

	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_WIRE_ADDR, uReg);

	// Wait for transfer to end
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_WIRE_ADDR);
	}while ((uReg & ONE_WIRE_CTL_CYC_MSK) != 0);
  /* TODO FIXME possibility of endless loop */

	// Presence detect is low
  if ((uReg & ONE_WIRE_CTL_DAT_MSK) == 0x0){
    xil_printf("D] ");
    return XST_SUCCESS;
  } else {
    xil_printf("E] ");
		return XST_FAILURE;
  }

}

//=================================================================================
//	OneWireWriteBit
//--------------------------------------------------------------------------------
//	This method writes a bit to the selected one wire interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uBit			IN	Bit to write
//	uOneWirePort	IN	Port to write bit to
//
//	Return
//	------
//	None
//=================================================================================
void OneWireWriteBit(u16 uBit,  u16 uOneWirePort)
{
	u32 uReg;

	//uReg = uPower << ONE_WIRE_CTL_POWER_OFST;
	uReg = 0x0;
	uReg = uReg | (uOneWirePort << ONE_WIRE_CTL_SEL_OFST);

	if (uEnableInterrupts == 1)
		uReg = uReg | ONE_WIRE_CTL_IEN_MSK;

	uReg = uReg | ONE_WIRE_CTL_CYC_MSK;

	if (uEnableOverdrive == 1)
			uReg = uReg | ONE_WIRE_CTL_OVD_MSK;

	if (uBit == 0x1)
		uReg = uReg | ONE_WIRE_CTL_DAT_MSK;

	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_WIRE_ADDR, uReg);

	// Wait for transfer to end
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_WIRE_ADDR);
	}while ((uReg & ONE_WIRE_CTL_CYC_MSK) != 0);
  /* TODO FIXME possibility of endless loop */

}

//=================================================================================
//	OneWireReadBit
//--------------------------------------------------------------------------------
//	This method reads a bit from the selected one wire interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uOneWirePort	IN	Port to read from
//
//	Return
//	------
//	Bit read
//=================================================================================
u16 OneWireReadBit(u16 uOneWirePort)
{
	u32 uReg;

	//uReg = uPower << ONE_WIRE_CTL_POWER_OFST;
	uReg = 0x0;
	uReg = uReg | (uOneWirePort << ONE_WIRE_CTL_SEL_OFST);

	if (uEnableInterrupts == 1)
		uReg = uReg | ONE_WIRE_CTL_IEN_MSK;

	uReg = uReg | ONE_WIRE_CTL_CYC_MSK;

	if (uEnableOverdrive == 1)
			uReg = uReg | ONE_WIRE_CTL_OVD_MSK;

	// Set the data bit high to enable a read
	uReg = uReg | ONE_WIRE_CTL_DAT_MSK;

	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_WIRE_ADDR, uReg);

	// Wait for transfer to end
	do
	{
		uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_WIRE_ADDR);
	}while ((uReg & ONE_WIRE_CTL_CYC_MSK) != 0);
  /* TODO FIXME possibility of endless loop */

	 // Return read bit
	 return (uReg & ONE_WIRE_CTL_DAT_MSK);
}

//=================================================================================
//	one_wire_write
//--------------------------------------------------------------------------------
//	This method writes a byte to the selected one wire interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uByte			IN	Byte to write
//	uOneWirePort	IN	Port to write to
//
//	Return
//	------
//	None
//=================================================================================
void OneWireWrite(u16 uByte, u16 uOneWirePort)
{
	u16 uBitCount;
	u16 uSendByte = uByte;

  xil_printf("W ");

	for (uBitCount = 0; uBitCount < 8; uBitCount++)
	{
		// Data is sent LSB first
		OneWireWriteBit(uSendByte & 0x1, uOneWirePort);
		uSendByte = uSendByte >> 1;
	}

}

//=================================================================================
//	OneWireRead
//--------------------------------------------------------------------------------
//	This method reads a byte from the selected one wire interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uOneWirePort	IN	Port to read from
//
//	Return
//	------
//	Byte read
//=================================================================================
u16 OneWireRead(u16 uOneWirePort)
{
	u16 uByteRead = 0x0;
	u16 uBitCount;

  xil_printf("R ");

	for (uBitCount = 0; uBitCount < 8; uBitCount++)
	{
		// Data is sent LSB first
		uByteRead = uByteRead | (OneWireReadBit(uOneWirePort) << uBitCount);
	}

	return uByteRead;
}

//=================================================================================
//	OneWireWriteBytes
//--------------------------------------------------------------------------------
//	This method writes a sequence of bytes to one wire interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uWriteBuffer	IN	Buffer containing bytes to write
//	uCount			IN	Number of bytes in buffer
//	uOneWirePort	IN	Port to write to
//
//	Return
//	------
//	None
//=================================================================================
void OneWireWriteBytes(u16 *uWriteBuffer, u16 uCount, u16 uOneWirePort)
{
	u16 uByteCount;

	for (uByteCount = 0; uByteCount < uCount; uByteCount++)
	{
		OneWireWrite(uWriteBuffer[uByteCount], uOneWirePort);
	}

}

//=================================================================================
//	OneWireReadBytes
//--------------------------------------------------------------------------------
//	This method reads a sequence of bytes from one wire interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uReadBuffer		OUT	Buffer containing bytes read
//	uCount			IN	Number of bytes to read
//	uOneWirePort	IN	Port to read from
//
//	Return
//	------
//	None
//=================================================================================
void OneWireReadBytes(u16 *uReadBuffer, u16 uCount, u16 uOneWirePort)
{
	u16 uByteCount;

	for (uByteCount = 0; uByteCount < uCount; uByteCount++)
	{
		uReadBuffer[uByteCount] = OneWireRead(uOneWirePort);
	}

}

//=================================================================================
//	OneWireSelect
//--------------------------------------------------------------------------------
//	Issue a 1-Wire rom select command, you do the reset first.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uRom			IN	ROM to select
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	None
//=================================================================================
void OneWireSelect(u16 uRom[8], u16 uOneWirePort)
{
    u16 uIndex;

    OneWireWrite(ONE_WIRE_MATCH_ROM, uOneWirePort);           // Choose ROM

    for (uIndex = 0; uIndex < 8; uIndex++)
    	OneWireWrite(uRom[uIndex], uOneWirePort);


}

//=================================================================================
//	OneWireSkip
//--------------------------------------------------------------------------------
//	Issue a 1-Wire rom skip command, to address all on bus.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	None
//=================================================================================
void OneWireSkip(u16 uOneWirePort)
{
    OneWireWrite(ONE_WIRE_SKIP_ROM, uOneWirePort);     // Skip ROM
}

//=================================================================================
//	OneWireReadRom
//--------------------------------------------------------------------------------
//	Read the ROM, only works if just one device on interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uRom			OUT	ROM read
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	XST_SUCCESS if device present and ROM read
//=================================================================================
int OneWireReadRom(u16 uRom[8], u16 uOneWirePort)
{
    int RetVal = XST_FAILURE;
    u16 uIndex;

    xil_printf("[1WIRE] Read ROM...");

    if (OneWireReset(uOneWirePort) == XST_SUCCESS){
      OneWireWrite(ONE_WIRE_READ_ROM, uOneWirePort);           // Read ROM command

      for (uIndex = 0; uIndex < 8; uIndex++){
        uRom[uIndex] = OneWireRead(uOneWirePort);
      }

      // 05/06/2015 INCLUDE FINAL RESET IN RETURN STATUS
      RetVal = OneWireReset(uOneWirePort); // Just in case
      xil_printf("%s\r\n", RetVal == XST_SUCCESS ? " [DONE]" : " [FAIL]");
      return RetVal;

      //return XST_SUCCESS;
    } else {
      xil_printf(" [FAIL]\r\n");
      return XST_FAILURE;
    }
}


//=================================================================================
//	OneWireResume
//--------------------------------------------------------------------------------
//	Issue a 1-Wire rom resume command, to access device already selected
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	None
//=================================================================================
void OneWireResume(u16 uOneWirePort)
{
	OneWireWrite(ONE_WIRE_RESUME, uOneWirePort);
}

//=================================================================================
//	OneWireResetSearch
//--------------------------------------------------------------------------------
//	Clear the search state so that if will start from the beginning again.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	None
//
//	Return
//	------
//	None
//=================================================================================
void OneWireResetSearch()
{
	u16 uIndex;

	// reset the search state
	uLastDiscrepancy = 0;
	uLastDeviceFlag = 0;
	uLastFamilyDiscrepancy = 0;
	for(uIndex = 0; uIndex < 7; uIndex++)
	{
	    uROM_NO[uIndex] = 0;
    }

}

//=================================================================================
//	OneWireTargetSearch
//--------------------------------------------------------------------------------
//	Setup the search to find the device type 'uFamilyCode' on the next call to search.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uFamilyCode	IN		Device family code to search for
//
//	Return
//	------
//	None
//=================================================================================
void OneWireTargetSearch(u16 uFamilyCode)
{
	u16 uIndex;

	// set the search state to find SearchFamily type devices
	uROM_NO[0] = uFamilyCode;
	for (uIndex = 1; uIndex < 8; uIndex++)
	   uROM_NO[uIndex] = 0;
	uLastDiscrepancy = 64;
	uLastFamilyDiscrepancy = 0;
	uLastDeviceFlag = 0;
}

//=================================================================================
//	OneWireSearch
//--------------------------------------------------------------------------------
//  Look for the next device. Returns 1 if a new address has been
//  returned. A zero might mean that the bus is shorted, there are
//  no devices, or you have already retrieved all of them.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uNewAddr		IN	Address of device found
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	XST_SUCCESS if new address found
//=================================================================================
int OneWireSearch(u16 *uNewAddr, u16 uOneWirePort)
{
   u16 id_bit_number;
   u16 last_zero;
   u16 rom_byte_number;
   u16 search_result;
   u16 id_bit;
   u16 cmp_id_bit;
   u16 i;

   u16 rom_byte_mask;
   u16 search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;

   // if the last call was not the last one
   if (!uLastDeviceFlag)
   {
	  // 1-Wire reset
	  if (OneWireReset(uOneWirePort) == XST_FAILURE)
	  {
		 // reset the search
		 uLastDiscrepancy = 0;
		 uLastDeviceFlag = 0;
		 uLastFamilyDiscrepancy = 0;
		 return XST_FAILURE;
	  }

	  // issue the search command
	  OneWireWrite(ONE_WIRE_SEARCH_ROM, uOneWirePort);

	  // loop to do the search
	  do
	  {
		 // read a bit and its complement
		 id_bit = OneWireReadBit(uOneWirePort);
		 cmp_id_bit = OneWireReadBit(uOneWirePort);

		 // check for no devices on 1-wire
		 if ((id_bit == 1) && (cmp_id_bit == 1))
			break;
		 else
		 {
			// all devices coupled have 0 or 1
			if (id_bit != cmp_id_bit)
			   search_direction = id_bit;  // bit write value for search
			else
			{
			   // if this discrepancy if before the Last Discrepancy
			   // on a previous next then pick the same as last time
			   if (id_bit_number < uLastDiscrepancy)
				  search_direction = ((uROM_NO[rom_byte_number] & rom_byte_mask) > 0);
			   else
				  // if equal to last pick 1, if not then pick 0
				  search_direction = (id_bit_number == uLastDiscrepancy);

			   // if 0 was picked then record its position in LastZero
			   if (search_direction == 0)
			   {
				  last_zero = id_bit_number;

				  // check for Last discrepancy in family
				  if (last_zero < 9)
					 uLastFamilyDiscrepancy = last_zero;
			   }
			}

			// set or clear the bit in the ROM byte rom_byte_number
			// with mask rom_byte_mask
			if (search_direction == 1)
			  uROM_NO[rom_byte_number] |= rom_byte_mask;
			else
			  uROM_NO[rom_byte_number] &= ~rom_byte_mask;

			// serial number search direction write bit
			OneWireWriteBit(search_direction, uOneWirePort);

			// increment the byte counter id_bit_number
			// and shift the mask rom_byte_mask
			id_bit_number++;
			rom_byte_mask <<= 1;

			// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
			if (rom_byte_mask == 0)
			{
				rom_byte_number++;
				rom_byte_mask = 1;
			}
		 }
	  }
	  while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

	  // if the search was successful then
	  if (!(id_bit_number < 65))
	  {
		 // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
		 uLastDiscrepancy = last_zero;

		 // check for last device
		 if (uLastDiscrepancy == 0)
			uLastDeviceFlag = 1;

		 search_result = 1;
	  }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !uROM_NO[0])
   {
	  uLastDiscrepancy = 0;
	  uLastDeviceFlag = 0;
	  uLastFamilyDiscrepancy = 0;
	  search_result = 0;
   }
   for (i = 0; i < 8; i++)
	   uNewAddr[i] = uROM_NO[i];

   if (search_result == 1)
	   return XST_SUCCESS;
   else
	   return XST_FAILURE;

}

//=================================================================================
//	OneWireCrc8
//--------------------------------------------------------------------------------
//  Compute a Dallas Semiconductor 8 bit CRC, these are used in the ROM and scratchpad
//	registers.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uAddr		IN		Address to calculate CRC on
//	uLen		IN		Length of CRC to calculate
//
//	Return
//	------
//	Crc
//=================================================================================
u16 OneWireCrc8(u16 *uAddr, u16 uLen)
{
	u16 crc = 0;
	u16 inbyte;
	u16 i;
	u16 mix;

	/*while (uLen--)
	{
		crc = dscrc_table + (crc ^ *uAddr++);
	}*/

	while (uLen--)
	{
		inbyte = *uAddr++;
		for (i = 8; i; i--)
		{
			mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix)
				crc ^= 0x8C;
			inbyte >>= 1;
		}
	}

	return crc;
}

//=================================================================================
//	DS2433WriteMem
//--------------------------------------------------------------------------------
//	Write to the addressed page in DS2433 device.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uDeviceAddress	IN	Selected device address
//	uSkipRomAddress	IN	Skip device selection (only if just one device on bus)
//	uMemBuffer	IN		Bytes to write
//	uBufferSize	IN		Number of bytes to write
//	uTA1		IN		Page address
//	uTA2		IN		Page address
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int DS2433WriteMem(u16 * uDeviceAddress, u16 uSkipRomAddress, u16 * uMemBuffer, u16 uBufferSize, u16 uTA1, u16 uTA2,  u16 uOneWirePort)
{
	u16 uMyES; //store ES for auth
	u16 uIndex;

	if (OneWireReset(uOneWirePort) == XST_SUCCESS)
	{

		if (uSkipRomAddress == 1)
			OneWireSkip(uOneWirePort);
		else
			OneWireSelect(uDeviceAddress, uOneWirePort);

		OneWireWrite(ONE_WIRE_WRITE_SCRATCHPAD, uOneWirePort);
		OneWireWrite(uTA1, uOneWirePort); //begin address
		OneWireWrite(uTA2, uOneWirePort); //begin address

		//write _memPage to scratchpad
		for (uIndex = 0; uIndex < uBufferSize; uIndex++)
		{
			OneWireWrite(uMemBuffer[uIndex], uOneWirePort);
		}

		//read and check data in scratchpad
		OneWireReset(uOneWirePort);

		if (uSkipRomAddress == 1)
			OneWireSkip(uOneWirePort);
		else
			OneWireSelect(uDeviceAddress, uOneWirePort);

		OneWireWrite(ONE_WIRE_READ_SCRATCHPAD, uOneWirePort);
		if (OneWireRead(uOneWirePort) != uTA1)
			return XST_FAILURE;

		if (OneWireRead(uOneWirePort) != uTA2)
			return XST_FAILURE;

		uMyES = OneWireRead(uOneWirePort); // ES Register

		if ((uMyES & ONE_WIRE_PF_FLAG) != 0x0)
			return XST_FAILURE;

		if (((uMyES & ONE_WIRE_PF_FLAG) == 0x0)&&((uMyES & ONE_WIRE_AA_FLAG) != 0x0))
			return XST_FAILURE;

		//check data written
		for (uIndex = 0; uIndex < uBufferSize; uIndex++)
		{
			if (OneWireRead(uOneWirePort) != uMemBuffer[uIndex])
				return XST_FAILURE;
		}

		//issue copy with auth data
		OneWireReset(uOneWirePort);

		if (uSkipRomAddress == 1)
			OneWireSkip(uOneWirePort);
		else
			OneWireSelect(uDeviceAddress, uOneWirePort);

		OneWireWrite(ONE_WIRE_COPY_SCRATCHPAD, uOneWirePort);
		OneWireWrite(uTA1, uOneWirePort);
		OneWireWrite(uTA2, uOneWirePort);

		// Enable strong pull-up now
		//OneWireEnableStrongPullup(0x1, uOneWirePort);

		OneWireWrite(uMyES, uOneWirePort); //pull-up!

		// Enable strong pull-up now
		OneWireEnableStrongPullup(0x1, uOneWirePort);
		Delay(10000); //5ms min strong pullup delay

		// Disable strong pull-up now
		OneWireEnableStrongPullup(0x0, uOneWirePort);

		// GT 05/06/2015 INCLUDE FINAL RESET IN RETURN STATUS
		return OneWireReset(uOneWirePort); //just in case...

		//return XST_SUCCESS;
	}
	else
		return XST_FAILURE;
}

//=================================================================================
//	DS2433ReadMem
//--------------------------------------------------------------------------------
//	Read the addressed page in DS2433 device.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uDeviceAddress	IN	Selected device address
//	uSkipRomAddress	IN	Skip device selection (only if just one device on bus)
//	uMemBuffer		OUT	Buffer for read data
//	uBufferSize		OUT	Number of bytes to read
//	uTA1		IN		Page address
//	uTA2		IN		Page address
//	uOneWirePort	IN	Selected port
//
//	Return
//	------
//	XST_SUCCESS if device present
//=================================================================================
int DS2433ReadMem(u16 * uDeviceAddress, u16 uSkipRomAddress, u16 * uMemBuffer, u16 uBufferSize, u16 uTA1, u16 uTA2, u16 uOneWirePort)
{
	u16 uIndex;
  int RetVal = XST_FAILURE;

  xil_printf("[1WIRE] Read Memory Page...");

	if (OneWireReset(uOneWirePort) == XST_SUCCESS)
	{

		if (uSkipRomAddress == 1){
			OneWireSkip(uOneWirePort);
    } else {
			OneWireSelect(uDeviceAddress, uOneWirePort);
    }

		OneWireWrite(ONE_WIRE_READ_MEMORY, uOneWirePort);
		OneWireWrite(uTA1, uOneWirePort);
		OneWireWrite(uTA2, uOneWirePort);
		for (uIndex = 0; uIndex < uBufferSize; uIndex++)
		{
			uMemBuffer[uIndex] = OneWireRead(uOneWirePort);
		}

		// GT 05/06/2015 INCLUDE FINAL RESET IN RETURN STATUS
    RetVal = OneWireReset(uOneWirePort);
    xil_printf("%s\r\n", RetVal == XST_SUCCESS ? " [DONE]" : " [FAIL]");
		return RetVal;
	} else {
    xil_printf(" [FAIL]\r\n");
		return XST_FAILURE;
  }
}





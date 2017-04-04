/**------------------------------------------------------------------------------
*  FILE NAME            : delay.c
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
*  This file contains the implementation to create a microsecond delay.
* ------------------------------------------------------------------------------*/

#include "delay.h"

//=================================================================================
//	Delay
//--------------------------------------------------------------------------------
//	This method creates a delay of a certain length (based on a 156.25MHz clock)
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	unsigned int	uLength	length in useconds (approximate)
//
//	Return
//	------
//
//=================================================================================
void Delay(unsigned int uLengthInMicroSeconds)
{
	unsigned int uLengthCount;
	unsigned int uMicroCount;

	/* CALCULATION OF DELAY:
	 *
	 * asm("nop") uses 6 clock cycles
	 * 1 us = 156 clock cycles. (156.25 MHz clk)
	 * 156/6 = 26 nops
	 * Trial/Error showed that 24: 1.01376 us
	 * Trial/Error showed that 23: 0.97536 us
	 * 6 clock cycles / nop gives a resolution of 0.0384 us
	 *
	*/
	volatile unsigned uCycles = 23; //volatile keyword is necessary otherwise compiler optimization settings can eliminate the inner loop.
	for (uLengthCount = 0; uLengthCount < uLengthInMicroSeconds; uLengthCount++)
	{
		for (uMicroCount = 0; uMicroCount < uCycles; uMicroCount++)
		{
			asm("nop");
		}
	}
}

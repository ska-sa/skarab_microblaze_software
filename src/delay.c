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
//  Delay
//--------------------------------------------------------------------------------
//  This method creates a delay of a certain length (based on a 156.25MHz clock)
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  unsigned int  uLength length in useconds (approximate)
//
//  Return
//  ------
//
//=================================================================================
void Delay(unsigned int uLengthInMicroSeconds)
{
  unsigned int uLengthCount;
  unsigned int uMicroCount;

  /* CALCULATION OF DELAY:
   *
   * >>>>> Microblaze clocked at 156.25 MHz <<<<<
   * asm("nop") uses 6 clock cycles
   * 1 us = 156 clock cycles. (156.25 MHz clk)
   * 156/6 = 26 nops
   * Trial/Error showed that 24: 1.01376 us
   * Trial/Error showed that 23: 0.97536 us
   * 6 clock cycles / nop gives a resolution of 0.0384 us
   *
   * >>>>> Microblaze clocked at 39.0625 MHz <<<<<
   * The Microblze and WishBone architecture was also
   * implemented with a 4x slower clock
   * New uCycles = 23 / 4 = 5,75 = +- 6
   * TODO: measure this somehow
   */

#ifdef REDUCED_CLK_ARCH
#define COUNTS_PER_MICROSECOND 6
#else
#define COUNTS_PER_MICROSECOND 23
#endif

  volatile unsigned uCycles = COUNTS_PER_MICROSECOND; //volatile keyword is necessary otherwise compiler optimization settings can eliminate the inner loop.

  for (uLengthCount = 0; uLengthCount < uLengthInMicroSeconds; uLengthCount++)
  {
    for (uMicroCount = 0; uMicroCount < uCycles; uMicroCount++)
    {
      asm("nop");
    }
  }
}

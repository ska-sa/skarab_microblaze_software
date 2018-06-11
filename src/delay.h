#ifndef DELAY_H_
#define DELAY_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : delay.h
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
*  This file contains the definitions of a function to create a microsecond delay.
* ------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void Delay(unsigned int uLengthInMicroSeconds);

#ifdef __cplusplus
}
#endif
#endif

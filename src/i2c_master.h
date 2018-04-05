#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

/*
/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Include file for OpenCores I2C Master core                 ////
////                                                             ////
////  File    : oc_i2c_master.h                                  ////
////  Function: c-include file                                   ////
////                                                             ////
////  Authors: Richard Herveille (richard@asics.ws)              ////
////           Filip Miletic                                     ////
////                                                             ////
////           www.opencores.org                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2001 Richard Herveille                        ////
////                    Filip Miletic                            ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
*/

#include <stdio.h>
#include "xparameters.h"
#include "xil_types.h"
#include "constant_defs.h"
#include "delay.h"

/*
 * Definitions for the Opencores i2c master core
 */

/* --- Definitions for i2c master's registers --- */
	
/* ----- Read-write access                                            */

#define OC_I2C_PRER_LO 0x00     /* Low byte clock prescaler register  */	
#define OC_I2C_PRER_HI 0x01     /* High byte clock prescaler register */	
#define OC_I2C_CTR     0x02     /* Control register                   */	
										
/* ----- Write-only registers                                         */
										
#define OC_I2C_TXR     0x03     /* Transmit byte register             */	
#define OC_I2C_CR      0x04     /* Command register                   */	
	
/* ----- Read-only registers                                          */
										
#define OC_I2C_RXR     0x03     /* Receive byte register              */
#define OC_I2C_SR      0x04     /* Status register                    */
	
/* ----- Bits definition                                              */
	
/* ----- Control register                                             */
	
#define OC_I2C_EN (1<<7)        /* Core enable bit:                   */
                                /*      1 - core is enabled           */
                                /*      0 - core is disabled          */
#define OC_I2C_IEN (1<<6)       /* Interrupt enable bit               */
                                /*      1 - Interrupt enabled         */
                                /*      0 - Interrupt disabled        */
                                /* Other bits in CR are reserved      */

/* ----- Command register bits                                        */
 
#define OC_I2C_STA (1<<7)       /* Generate (repeated) start condition*/
#define OC_I2C_STO (1<<6)       /* Generate stop condition            */
#define OC_I2C_RD  (1<<5)       /* Read from slave                    */
#define OC_I2C_WR  (1<<4)       /* Write to slave                     */
#define OC_I2C_ACK (1<<3)       /* Acknowledge from slave             */
                                /*      1 - ACK                       */
                                /*      0 - NACK                      */
#define OC_I2C_IACK (1<<0)      /* Interrupt acknowledge              */

/* ----- Status register bits                                         */

#define OC_I2C_RXACK (1<<7)     /* ACK received from slave            */
                                /*      1 - ACK                       */
                                /*      0 - NACK                      */
#define OC_I2C_BUSY  (1<<6)     /* Busy bit                           */
#define OC_I2C_TIP   (1<<1)     /* Transfer in progress               */
#define OC_I2C_IF    (1<<0)     /* Interrupt flag                     */

/* bit testing and setting macros                                     */

#define OC_ISSET(reg,bitmask)       ((reg)&(bitmask))
#define OC_ISCLEAR(reg,bitmask)     (!(OC_ISSET(reg,bitmask)))
#define OC_BITSET(reg,bitmask)      ((reg)|(bitmask))
#define OC_BITCLEAR(reg,bitmask)    ((reg)|(~(bitmask)))
#define OC_BITTOGGLE(reg,bitmask)   ((reg)^(bitmask))
#define OC_REGMOVE(reg,value)       ((reg)=(value))

#define SPEED_100kHz	0x0
#define SPEED_400kHz	0x1

#ifdef REDUCED_CLK_ARCH
// Clock prescaler ratios for 39.0625MHz Wishbone clock
#define SPEED_100kHz_CLOCK_PRESCALER 78
#define SPEED_400kHz_CLOCK_PRESCALER 19
#else
// Clock prescaler ratios for 156.25MHz Wishbone clock
#define SPEED_100kHz_CLOCK_PRESCALER 0x138 // 312
#define SPEED_400kHz_CLOCK_PRESCALER 0x4E  // 78
#endif

#define I2C_TIMEOUT		10000

u32 GetI2CAddressOffset(u16 uId);
void InitI2C(u16 uId, u16 uSpeed);
int WriteI2CBytes(u16 uId, u16 uSlaveAddress, u16 * uWriteBytes, u16 uNumBytes);
int ReadI2CBytes(u16 uId, u16 uSlaveAddress, u16 * uReadBytes, u16 uNumBytes);
int PMBusReadI2CBytes(u16 uId, u16 uSlaveAddress, u16 uCommandCode, u16 * uReadBytes, u16 uNumBytes);
int HMCReadI2CBytes(u16 uId, u16 uSlaveAddress, u16 * uReadAddress, u16 * uReadBytes);
/* TODO: change the HMCReadI2CBytes function prototype to match HMCWriteI2CBytes */
int HMCWriteI2CBytes(u16 uId, u16 uSlaveAddress, u32 uWriteAddress, u32 uWriteData);

// Specific to HitechGlobal dev board
#define PCA9548_0_ADDRESS 	0x70
#define PCA9548_1_ADDRESS 	0x71
#define PCA9548_2_ADDRESS 	0x73
#define PCA9548_REFCLK_OSC 	0x80
#define PCA9548_SFP_OSC		0x08
#define PCA9548_FMC_OSC 	0x02

#define SI570_HS_N1_ADDRESS			0x07
#define SI570_FREEZE_DCO_ADDRESS	0x89
#define SI570_NEW_FREQ_ADDRESS		0x87

#define SI570_FREEZE_DCO_VAL		0x10
#define SI570_NEW_FREQ_VAL			0x40

#define SI570_FMC_OSC_ADDRESS		0x55
#define SI570_FMC_REG_7_VAL			0x02
#define SI570_FMC_REG_8_VAL			0x42

#define SI570_SFP_OSC_ADDRESS		0x58
#define SI570_SPF_REG_7_VAL			0x02
#define SI570_SFP_REG_8_VAL			0x42

#define SI570_REFCLK_OSC_ADDRESS	0x57
#define SI570_REFCLK_REG_7_VAL		0x01
#define SI570_REFCLK_REG_8_VAL		0xC2

int I2CPCA9548SelectChannel(u16 uId, u16 uChannelSelection);
int I2CFMCReadClkOscBytes(u16 uId, u16 uByteAddress, u16* uReadBytes, unsigned uNumBytes);
int I2CSFPReadClkOscBytes(u16 uId, u16 uByteAddress, u16* uReadBytes, unsigned uNumBytes);
int I2CReadRefclkOscBytes(u16 uId, u16 uByteAddress, u16* uReadBytes, unsigned uNumBytes);
int I2CSFPWriteClkOscBytes(u16 uId, u16 uByteAddress, u16* uWriteBytes, unsigned uNumBytes, u16 uOpenSwitch);
int I2CWriteRefclkOscBytes(u16 uId, u16 uByteAddress, u16* uWriteBytes, unsigned uNumBytes, u16 uOpenSwitch);
int I2CProgramSFPClkOsc();
int I2CProgramRefClkOsc();

#endif

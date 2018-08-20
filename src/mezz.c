#include <xstatus.h>
#include <xil_assert.h>
#include <xil_types.h>

#include "mezz.h"
#include "one_wire.h"
#include "register.h"
#include "constant_defs.h"
#include "print.h"

static MezzType read_mezz_type_id(u8 mezz_site);


struct sMezzObject *init_mezz_location(u8 mezz_site){
  static struct sMezzObject MezzContext[4];  /* statically allocated memory - but could be dynamic in future */
  MezzType mt;

  /* sanity checks */
  Xil_AssertNonvoid((mezz_site >= 0) && (mezz_site < 4));    /* API usage error  */

  if (MEZZ_MAGIC == MezzContext[mezz_site].m_magic){
    error_printf("MEZZ [%02x] Failed - attempted to overwrite previous state.", mezz_site);
    return &(MezzContext[mezz_site]);
  }

  MezzContext[mezz_site].m_magic = MEZZ_MAGIC;
  MezzContext[mezz_site].m_site = mezz_site;

  mt = read_mezz_type_id(mezz_site);    /* get the type id stored on the mezzanine card */
  MezzContext->m_type = mt;

  switch(mt){
    case MEZ_BOARD_TYPE_QSFP:
      if (uQSFPMezzaninePresent == QSFP_MEZZANINE_NOT_PRESENT){
        uQSFPMezzanineLocation = mezz_site;     /* TODO: need to get rid of global scope and place within local scope of obj */
      }
      uQSFPMezzaninePresent = QSFP_MEZZANINE_PRESENT;
      //ret = init_qsfp_mezz(&(MezzContext[mezz_site].QSFPContext));
      break;

    case MEZ_BOARD_TYPE_HMC_R1000_0005:
      //ret = init_hmc_mezz(mezz);
      break;

    case MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2:
      /* TODO globals... */
      if (uADC32RF45X2MezzaninePresent == ADC32RF45X2_MEZZANINE_NOT_PRESENT){
        uADC32RF45X2MezzanineLocation = mezz_site;
      }
      uADC32RF45X2MezzaninePresent = ADC32RF45X2_MEZZANINE_PRESENT;
      //ret = init_adc_mezz(&(MezzContext[mezz_site].AdcContext));
      break;

    /* unhandled cases */
    case MEZ_BOARD_TYPE_QSFP_PHY:
    case MEZ_BOARD_TYPE_OPEN:
    case MEZ_BOARD_TYPE_UNKNOWN:
    default:
      break;
  }

  return &(MezzContext[mezz_site]);   /* return a handle to this mezz data structure */
}




static MezzType read_mezz_type_id(u8 mezz_site){
  u32 reg;
  u32 mezz_mask;
  u32 mezz_ctl_shadow_reg;
  u16 device_rom[8];
  u16 read_bytes[32];
  int ret;
  u16 one_wire_port;

  /* determine if the mezz site is populated with a card */

  mezz_mask = 1 << mezz_site;           /* shift to relevant mezz position */
  one_wire_port = mezz_site + 1;

  reg = ReadBoardRegister(C_RD_MEZZANINE_STAT_0_ADDR);
  if (0 == (reg & mezz_mask)){
    return MEZ_BOARD_TYPE_OPEN;
  }

  /* enable the mezzanine site */
  /* TODO: narrow down scope of globals */
  mezz_ctl_shadow_reg = uWriteBoardShadowRegs[C_WR_MEZZANINE_CTL_ADDR >> 2];
  mezz_ctl_shadow_reg = mezz_ctl_shadow_reg | mezz_mask;
  WriteBoardRegister(C_WR_MEZZANINE_CTL_ADDR, mezz_ctl_shadow_reg);

  error_printf("MEZZ [%02x] ", mezz_site);

  /* get the ID from one-wire EEPROM */
  ret = OneWireReadRom(device_rom, one_wire_port);
  if (XST_FAILURE == ret){
    error_printf("UNKNOWN - Failed to read device-rom from one-wire EEPROM.\r\n");
    return MEZ_BOARD_TYPE_UNKNOWN;
  }

  ret = DS2433ReadMem(device_rom, 0x0, read_bytes, 0x7, 0x0, 0x0, one_wire_port);
  if (XST_FAILURE == ret){
    error_printf("UNKNOWN - Failed to read ID bytes from one-wire EEPROM.\r\n");
    return MEZ_BOARD_TYPE_UNKNOWN;
  }

  if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE3)&&(read_bytes[0x6] == 0x99)){
    error_printf("QSFP+ MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_QSFP;

  } else if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE3)&&(read_bytes[0x6] == 0xFD)){
    info_printf("QSFP+ PHY MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_QSFP_PHY;

  } else if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE7)&&(read_bytes[0x6] == 0xE5)){
    info_printf("ADC32RF45X2 MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2;

  } else if ((read_bytes[0x0] == 0x53)&&(read_bytes[0x4] == 0xFF)&&(read_bytes[0x5] == 0x00)&&(read_bytes[0x6] == 0x01)){
    info_printf("HMC R1000-0005 MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_HMC_R1000_0005;

  } else {
    info_printf("UNKNOWN - Unsupported PX number and manufacturer ID.\r\n");
    return MEZ_BOARD_TYPE_UNKNOWN;
  }
}

#include <xstatus.h>
#include <xil_assert.h>
#include <xil_types.h>

#include "mezz.h"
#include "one_wire.h"
#include "register.h"
#include "constant_defs.h"
#include "logging.h"

static MezzHWType read_mezz_type_id(u8 mezz_site);
/* static MezzFirmwType get_mezz_firmware_type(u8 mezz_site); */

/*********** Sanity Checks ***************/
#ifdef DO_SANITY_CHECKS
#define SANE_MEZZ_SITE(site) __sanity_check_mezz_site(site);
#else
#define SANE_MEZZ_SITE(site)
#endif

#ifdef DO_SANITY_CHECKS
void __sanity_check_mezz_site(u8 mezz_site){
  /* sanity checks */
  Xil_AssertVoid((mezz_site >= 0) && (mezz_site < 4));    /* API usage error */
}
#endif


struct sMezzObject *init_mezz_location(u8 mezz_site){
  static struct sMezzObject MezzContext[4];  /* statically allocated memory - but could be dynamic in future */
  MezzHWType mt;
  MezzFirmwType ft;

  SANE_MEZZ_SITE(mezz_site);

  if (MEZZ_MAGIC == MezzContext[mezz_site].m_magic){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "MEZZ [%02x] Failed - attempted to overwrite previous state.", mezz_site);
    return &(MezzContext[mezz_site]);
  }

  MezzContext[mezz_site].m_magic = MEZZ_MAGIC;
  MezzContext[mezz_site].m_site = mezz_site;

  MezzContext[mezz_site].m_allow_init = 0;

  mt = read_mezz_type_id(mezz_site);    /* get the type id stored on the mezzanine card */
  MezzContext[mezz_site].m_type = mt;

  ft = get_mezz_firmware_type(mezz_site);   /* get the firmware type compiled
                                               for this mezz */

  /* preset this value and set if firmware support present */
  MezzContext[mezz_site].m_firmw_support = FIRMW_SUPPORT_FALSE;

  switch(mt){
    case MEZ_BOARD_TYPE_QSFP:
      if (MEZ_FIRMW_TYPE_QSFP == ft){   /* firmware support? */
        MezzContext[mezz_site].m_firmw_support = FIRMW_SUPPORT_TRUE;
        if (uQSFPMezzaninePresent == QSFP_MEZZANINE_NOT_PRESENT){
          uQSFPMezzanineLocation = mezz_site;     /* TODO: need to get rid of global scope and place within local scope of obj */
        }
        uQSFPMezzaninePresent = QSFP_MEZZANINE_PRESENT;
        MezzContext[mezz_site].m_allow_init = 1;
        //ret = init_qsfp_mezz(&(MezzContext[mezz_site].QSFPContext));
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "MEZZ [%02x] WARNING: no firmware support for QSFP!\r\n", mezz_site);
      }
      break;

    case MEZ_BOARD_TYPE_HMC_R1000_0005:
      if (MEZ_FIRMW_TYPE_HMC_R1000_0005 == ft){   /* firmware support? */
        MezzContext[mezz_site].m_firmw_support = FIRMW_SUPPORT_TRUE;
        MezzContext[mezz_site].m_allow_init = 1;
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "MEZZ [%02x] WARNING: no firmware support for HMC!\r\n", mezz_site);
      }
      //ret = init_hmc_mezz(mezz);
      break;

    case MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2:
      if (MEZ_FIRMW_TYPE_SKARAB_ADC32RF45X2 == ft){   /* firmware support? */
        MezzContext[mezz_site].m_firmw_support = FIRMW_SUPPORT_TRUE;
        /* TODO globals... */
        if (uADC32RF45X2MezzaninePresent == ADC32RF45X2_MEZZANINE_NOT_PRESENT){
          uADC32RF45X2MezzanineLocation = mezz_site;
        }
        uADC32RF45X2MezzaninePresent = ADC32RF45X2_MEZZANINE_PRESENT;
        MezzContext[mezz_site].m_allow_init = 1;
        //ret = init_adc_mezz(&(MezzContext[mezz_site].AdcContext));
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "MEZZ [%02x] WARNING: no firmware support for ADC!\r\n", mezz_site);
      }
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




static MezzHWType read_mezz_type_id(u8 mezz_site){
  u32 reg;
  u32 mezz_mask;
  u32 mezz_ctl_shadow_reg;
  u16 device_rom[8];
  u16 read_bytes[32];
  int ret;
  u16 one_wire_port;

  SANE_MEZZ_SITE(mezz_site);

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

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "MEZZ [%02x] ", mezz_site);

  /* get the ID from one-wire EEPROM */
  ret = OneWireReadRom(device_rom, one_wire_port);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UNKNOWN - Failed to read device-rom from one-wire EEPROM.\r\n");
    return MEZ_BOARD_TYPE_UNKNOWN;
  }

  ret = DS2433ReadMem(device_rom, 0x0, read_bytes, 0x7, 0x0, 0x0, one_wire_port);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UNKNOWN - Failed to read ID bytes from one-wire EEPROM.\r\n");
    return MEZ_BOARD_TYPE_UNKNOWN;
  }

  if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE3)&&(read_bytes[0x6] == 0x99)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+ MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_QSFP;

  } else if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE3)&&(read_bytes[0x6] == 0xFD)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+ PHY MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_QSFP_PHY;

  } else if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE7)&&(read_bytes[0x6] == 0xE5)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ADC32RF45X2 MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2;

  } else if ((read_bytes[0x0] == 0x53)&&(read_bytes[0x4] == 0xFF)&&(read_bytes[0x5] == 0x00)&&(read_bytes[0x6] == 0x01)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC R1000-0005 MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_HMC_R1000_0005;

  } else {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "UNKNOWN - Unsupported PX number and manufacturer ID.\r\n");
    return MEZ_BOARD_TYPE_UNKNOWN;
  }
}


/*
 * bit signatures to detect firmware support/presence of the various mezzanine
 * cards
 */
#define BYTE_MASK_NONE_PRESENT  1  /* bxxxx0001 */
#define BYTE_MASK_QSFP_PRESENT  3  /* bxxxx0011 */
#define BYTE_MASK_HMC_PRESENT   5  /* bxxxx0101 */
#define BYTE_MASK_ADC_PRESENT   7  /* bxxxx0111 */

/* static */ MezzFirmwType get_mezz_firmware_type(u8 mezz_site){
  u32 reg;
  u32 masked_byte;
  u32 mask;
  MezzFirmwType firmw_type;

  SANE_MEZZ_SITE(mezz_site);

  reg = ReadBoardRegister(C_RD_MEZZANINE_STAT_1_ADDR);
  mask = 0x0f << (mezz_site * 8);
  masked_byte = reg & mask;
  masked_byte = masked_byte >> (mezz_site * 8);

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "MEZZ [%02x] Firmware status: (%d) ", mezz_site, masked_byte);

  switch (masked_byte){
    case BYTE_MASK_NONE_PRESENT:
      firmw_type = MEZ_FIRMW_TYPE_OPEN;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "OPEN\r\n");
      break;

    case BYTE_MASK_QSFP_PRESENT:
      firmw_type = MEZ_FIRMW_TYPE_QSFP;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "QSFP\r\n");
      break;

    case BYTE_MASK_HMC_PRESENT:
      firmw_type = MEZ_FIRMW_TYPE_HMC_R1000_0005;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "HMC\r\n");
      break;

    case BYTE_MASK_ADC_PRESENT:
      firmw_type = MEZ_FIRMW_TYPE_SKARAB_ADC32RF45X2;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "ADC\r\n");
      break;

    default:
      firmw_type = MEZ_FIRMW_TYPE_UNKNOWN;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "UNKNOWN\r\n");
      break;
  }

  return firmw_type;
}

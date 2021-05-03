/* SARAO, RvW */

#include <xstatus.h>
#include <xil_assert.h>
#include <xil_types.h>

#include "mezz.h"
#include "one_wire.h"
#include "register.h"
#include "constant_defs.h"
#include "logging.h"

/* static MezzHWType read_mezz_type_id(u8 mezz_site); */
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
  //static struct sMezzObject MezzContext[4];  /* statically allocated memory - but could be dynamic in future */
  struct sMezzObject *mezz_hdl;
  MezzHWType mt;
  MezzFirmwType ft;
  u8 ret;

  SANE_MEZZ_SITE(mezz_site);

  mezz_hdl = lookup_mezz_handle_by_site(mezz_site);

  if (MEZZ_MAGIC == mezz_hdl->m_magic){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "MEZZ [%02x] Failed - attempted to overwrite previous state.", mezz_site);
    return mezz_hdl;
  }

  mezz_hdl->m_magic = MEZZ_MAGIC;
  mezz_hdl->m_site = mezz_site;

  mezz_hdl->m_allow_init = 0;

  mt = read_mezz_type_id(mezz_site);    /* get the type id stored on the mezzanine card */
  mezz_hdl->m_type = mt;

  ft = get_mezz_firmware_type(mezz_site);   /* get the firmware type compiled
                                               for this mezz */

  /* preset this value and set if firmware support present */
  mezz_hdl->m_firmw_support = FIRMW_SUPPORT_FALSE;

  switch(mt){
    case MEZ_BOARD_TYPE_QSFP:
    case MEZ_BOARD_TYPE_QSFP_PHY:
      if (MEZ_FIRMW_TYPE_QSFP == ft){   /* firmware support? */
        mezz_hdl->m_firmw_support = FIRMW_SUPPORT_TRUE;
        /*
         * only ONE qsfp mezz card provided for - conceptually one could have multiple qsfp cards
         * but there may be broader implications to the code e.g. QSFPResetAndProgramCommandHandler() would
         * need to be updated to accept the id of the qsfp card being addresses and pass this to reset function call.
         * The uQSFPMezzaninePresent flag guards for this here i.e. if this is the first qsfp card found...
         */
        if (uQSFPMezzaninePresent == QSFP_MEZZANINE_NOT_PRESENT){
          uQSFPMezzanineLocation = mezz_site;     /* TODO: need to get rid of global scope and place within local scope of obj */
          mezz_hdl->m_allow_init = 1;
        }
        uQSFPMezzaninePresent = QSFP_MEZZANINE_PRESENT;
        //ret = init_qsfp_mezz(&(mezz_hdl->QSFPContext));
        ret = uQSFPInit(&(mezz_hdl->m_obj.QSFPContext));
        /*Assert: could have a more lenient error check but no reason for init to fail here */
        Xil_AssertNonvoid(XST_SUCCESS == ret);    /* development time error */
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "MEZZ [%02x] WARNING: no firmware support for QSFP!\r\n", mezz_site);
      }
      break;

    case MEZ_BOARD_TYPE_HMC_R1000_0005:
      if (MEZ_FIRMW_TYPE_HMC_R1000_0005 == ft){   /* firmware support? */
        mezz_hdl->m_firmw_support = FIRMW_SUPPORT_TRUE;
        mezz_hdl->m_allow_init = 1;
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "MEZZ [%02x] WARNING: no firmware support for HMC!\r\n", mezz_site);
      }
      //ret = init_hmc_mezz(mezz);
      break;

    case MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2:
      if (MEZ_FIRMW_TYPE_SKARAB_ADC32RF45X2 == ft){   /* firmware support? */
        mezz_hdl->m_firmw_support = FIRMW_SUPPORT_TRUE;
        /* TODO globals... */
#if 0
        /* the mezz site must now be passed along with the adc state obj */
        if (uADC32RF45X2MezzaninePresent == ADC32RF45X2_MEZZANINE_NOT_PRESENT){
          uADC32RF45X2MezzanineLocation = mezz_site;
        }
        uADC32RF45X2MezzaninePresent = ADC32RF45X2_MEZZANINE_PRESENT;
#endif
        mezz_hdl->m_allow_init = 1;
        //ret = init_adc_mezz(&(mezz_hdl->AdcContext));
        ret = AdcInit(&(mezz_hdl->m_obj.AdcContext), mezz_site);

#ifdef PERALEX_SPECIFIC
        static u8 uFirstADC32RF45Mezzanine = TRUE;

        if (uFirstADC32RF45Mezzanine == TRUE){
          uFirstADC32RF45Mezzanine = FALSE;
          // Configure the location of where to expect the GPS PPS
          /* TODO - shadow reg write missing in Peralex code (?) */
          WriteBoardRegister(C_WR_TIMESTAMP_CTL_ADDR, mezz_site << 8);
        }
#endif

        /*Assert: could have a more lenient error check but no reason for init to fail here */
        Xil_AssertNonvoid(XST_SUCCESS == ret);    /* development time error */
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "MEZZ [%02x] WARNING: no firmware support for ADC!\r\n", mezz_site);
      }
      break;

    /* unhandled cases */
    case MEZ_BOARD_TYPE_OPEN:
    case MEZ_BOARD_TYPE_UNKNOWN:
    default:
      break;
  }

  return mezz_hdl;   /* return a handle to this mezz data structure */
}




/* static */ MezzHWType read_mezz_type_id(u8 mezz_site){
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

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "MEZZ [%02x] Hardware status: ", mezz_site);

  reg = ReadBoardRegister(C_RD_MEZZANINE_STAT_0_ADDR);
  if (0 == (reg & mezz_mask)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "NONE\r\n");
    return MEZ_BOARD_TYPE_OPEN;
  }

  /* enable the mezzanine site */
  /* TODO: narrow down scope of globals */
  mezz_ctl_shadow_reg = uWriteBoardShadowRegs[C_WR_MEZZANINE_CTL_ADDR >> 2];
  mezz_ctl_shadow_reg = mezz_ctl_shadow_reg | mezz_mask;
  WriteBoardRegister(C_WR_MEZZANINE_CTL_ADDR, mezz_ctl_shadow_reg);

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
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+ MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_QSFP;

  } else if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE3)&&(read_bytes[0x6] == 0xFD)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+ PHY MEZZANINE\r\n");
    return MEZ_BOARD_TYPE_QSFP_PHY;

  } else if ((read_bytes[0x0] == 0x50)&&(read_bytes[0x4] == 0x01)&&(read_bytes[0x5] == 0xE7)&&((read_bytes[0x6] == 0xE5)||(read_bytes[0x6] == 0xE6)||(read_bytes[0x6] == 0xE7))){
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
#define BYTE_MASK_FIRMW_ID_NONE  0  /* b000 */
#define BYTE_MASK_FIRMW_ID_QSFP  1  /* b001 */
#define BYTE_MASK_FIRMW_ID_HMC   2  /* b010 */
#define BYTE_MASK_FIRMW_ID_ADC   3  /* b011 */

/* static */ MezzFirmwType get_mezz_firmware_type(u8 mezz_site){
  u32 reg;
  u32 masked_byte;
  MezzFirmwType firmw_type;

  SANE_MEZZ_SITE(mezz_site);

  reg = ReadBoardRegister(C_RD_MEZZANINE_STAT_1_ADDR);

  masked_byte = (reg >> (mezz_site * 8)) & 0x0f;      /* get the relevant nibble */
  masked_byte = masked_byte >> 1;                     /* shift one more place to get ID bits */

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "MEZZ [%02x] Firmware status: (%d) ", mezz_site, masked_byte);

  switch (masked_byte){
    case BYTE_MASK_FIRMW_ID_NONE:
      firmw_type = MEZ_FIRMW_TYPE_OPEN;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "OPEN\r\n");
      break;

    case BYTE_MASK_FIRMW_ID_QSFP:
      firmw_type = MEZ_FIRMW_TYPE_QSFP;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP\r\n");
      break;

    case BYTE_MASK_FIRMW_ID_HMC:
      firmw_type = MEZ_FIRMW_TYPE_HMC_R1000_0005;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC\r\n");
      break;

    case BYTE_MASK_FIRMW_ID_ADC:
      firmw_type = MEZ_FIRMW_TYPE_SKARAB_ADC32RF45X2;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ADC\r\n");
      break;

    default:
      firmw_type = MEZ_FIRMW_TYPE_UNKNOWN;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "UNKNOWN\r\n");
      break;
  }

  return firmw_type;
}



void run_qsfp_mezz_mgmt(void){
  struct sMezzObject *m;
  struct sQSFPObject *q;

  /* the codebase only supports one qsfp card cached in uQSFPMezzanineLocation */
  if (uQSFPMezzaninePresent == QSFP_MEZZANINE_PRESENT){
    m = lookup_mezz_handle_by_site(uQSFPMezzanineLocation);

    /* we can assert here because if present flag is set, we should
     * have set up the rest of the context state with valid parameters
     */
    Xil_AssertVoid(m != NULL);
    Xil_AssertVoid(MEZZ_MAGIC == m->m_magic);

    q = &(m->m_obj.QSFPContext);

    Xil_AssertVoid(q != NULL);
    Xil_AssertVoid(QSFP_MAGIC == q->QSFPMagic);

    QSFPStateMachine(q);
  }
}

void run_adc_mezz_mgmt(void){
  struct sMezzObject *m;
  struct sAdcObject *a;
  int i;

  /* the codebase must support multiple ADCs - therefore lookup required */

  for (i = 0; i < 4; i++){
    m = lookup_mezz_handle_by_site(i);

    /* order of conditions below matter */
    if (NULL == m){
      return;
    }

    if (MEZZ_MAGIC != m->m_magic){
      return;
    }

    if (MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2 != m->m_type){
      return;
    }

    if (0 == m->m_allow_init){
      return;
    }

    a = &(m->m_obj.AdcContext);

    /* can assert here - if we have gotten this far, everything else should be set correctly */
    Xil_AssertVoid(a != NULL);
    Xil_AssertVoid(ADC_MAGIC == a->AdcMagic);

    AdcStateMachine(a);
  }

}


void adc_mezz_sm_control(u8 mezz_site, AdcMezzSMOperation op){
  struct sMezzObject *m;
  struct sAdcObject *a;

  /* do not assert() in these functions since they are reachable via user commands */

  /* mezzanine sites indexed 0 to 3 */
  if (mezz_site > 3){
    return;
  }

  m = lookup_mezz_handle_by_site(mezz_site);
  if (NULL == m){
    return;
  }

  if (MEZZ_MAGIC != m->m_magic){
    return;
  }

  if (MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2 != m->m_type){
    return;
  }

  if (0 == m->m_allow_init){
    return;
  }

  a = &(m->m_obj.AdcContext);
  if (NULL == a){
    return;
  }

  if (ADC_MAGIC != a->AdcMagic){
    return;
  }

  switch(op){
    case ADC_MEZZ_SM_PAUSE_OP:
      uAdcStateMachinePause(a);
      break;

    case ADC_MEZZ_SM_RESUME_OP:
      uAdcStateMachineResume(a);
      break;

    case ADC_MEZZ_SM_RESET_OP:
      uAdcStateMachineReset(a);
      break;

    default:
      break;
  }
}

u8 is_adc_mezz(u8 mezz_site){
  /* do not assert() in these functions since they are reachable via user commands */
  struct sMezzObject *m;

  /* mezzanine sites indexed 0 to 3 */
  if (mezz_site > 3){
    return XST_FAILURE;
  }

  m = lookup_mezz_handle_by_site(mezz_site);
  if (NULL == m){
    return XST_FAILURE;
  }

  if (MEZZ_MAGIC != m->m_magic){
    return XST_FAILURE;
  }

  if (MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2 != m->m_type){
    return XST_FAILURE;
  }

  return XST_SUCCESS;
}


/* hide the mezz state handles */
struct sMezzObject *lookup_mezz_handle_by_site(u8 mezz_site){
  /*
   * statically initialise the pool of mezz state objects for each site
   */
  static struct sMezzObject MezzContext[4];  /* statically allocated memory - but could be dynamic in future */

  SANE_MEZZ_SITE(mezz_site);

  return &MezzContext[mezz_site];
}


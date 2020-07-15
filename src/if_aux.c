/* RvW, May 2020, SARAO */

#include <xil_types.h>
#include <xstatus.h>

#include "logging.h"
#include "id.h"

#define HOSTNAME_STRING_LEN  16
u8 *if_generate_hostname_string(u8 phy_interface_id){     /* arg is the physical interface id / position */
  static u8 hostname[HOSTNAME_STRING_LEN] = {'\0'};
  u8 digit;
  u8 serial_no[ID_SK_SERIAL_LEN];
  u8 status;

  status = get_skarab_serial(serial_no, ID_SK_SERIAL_LEN);
  if (status == XST_FAILURE){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Failed to read SKARAB serial number.\r\n");
    /* from here on in the mac address and hostname will be incorrect (ff:ff:ff...) */
  }

  /* hostname string length is 15 chars plus 1 for '/0'
   * e.g. "skarab020204-01" + '/0'
   */

  /* fill in the prefix */
  hostname[0] = 's';
  hostname[1] = 'k';
  hostname[2] = 'a';
  hostname[3] = 'r';
  hostname[4] = 'a';
  hostname[5] = 'b';

  /* fill in the dash */
  hostname[12] = '-';

  /* fill in the skarab serial number - hex based */
  digit = (serial_no[1] & 0xff) / 0x10;  /* upper digit of this octet */
  hostname[6] = digit > 9 ? ((digit - 10) + 0x41) : (digit + 0x30);

  digit = (serial_no[1] & 0xff) % 0x10;  /* lower digit of this octet */
  hostname[7] = digit > 9 ? ((digit - 10) + 0x41) : (digit + 0x30);

  digit = (serial_no[2] & 0xff) / 0x10;  /* upper digit of this octet */
  hostname[8] = digit > 9 ? ((digit - 10) + 0x41) : (digit + 0x30);

  digit = (serial_no[2] & 0xff) % 0x10;  /* lower digit of this octet */
  hostname[9] = digit > 9 ? ((digit - 10) + 0x41) : (digit + 0x30);

  digit = (serial_no[3] & 0xff) / 0x10;  /* upper digit of this octet */
  hostname[10] = digit > 9 ? ((digit - 10) + 0x41) : (digit + 0x30);

  digit = (serial_no[3] & 0xff) % 0x10;  /* lower digit of this octet */
  hostname[11] = digit > 9 ? ((digit - 10) + 0x41) : (digit + 0x30);

  /* fill in interface number in host name - decimal based */
  hostname[13] = (phy_interface_id / 10) + 48;  /* tens digit of interface id*/
  hostname[14] = (phy_interface_id % 10) + 48;  /* unit digit of interface id*/

  /* terminate string */
  hostname[15] = '\0';

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "I/F  [%02d] hostname <%s>\r\n", phy_interface_id, hostname);

  return hostname;     /* Return the pointer to the hostname.
                        * The memory pointed to here will
                        * persist - static declaration
                        */
}


#define MAC_ADDR_LEN  6

u8 *if_generate_mac_addr_array(u8 phy_interface_id){
  static u8 mac_addr[MAC_ADDR_LEN];
  u8 serial_no[ID_SK_SERIAL_LEN];
  u8 status;
  int i;

  status = get_skarab_serial(serial_no, ID_SK_SERIAL_LEN);
  if (status == XST_FAILURE){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Failed to read SKARAB serial number.\r\n");
    /* from here on in the mac address and hostname will be incorrect (ff:ff:ff...) */
  }

  mac_addr[0] = 0x06;
  mac_addr[1] = serial_no[0];
  mac_addr[2] = serial_no[1];
  mac_addr[3] = serial_no[2];
  mac_addr[4] = serial_no[3];
  mac_addr[5] = phy_interface_id;

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "I/F  [%02d] mac addr <", phy_interface_id);
  for (i = 0; i < MAC_ADDR_LEN; i++){
    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "%02x%s", mac_addr[i], (i == MAC_ADDR_LEN - 1) ? "" : ":");
  }
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, ">\r\n");

  return mac_addr;     /* Return the pointer to the mac_addr.
                        * The memory pointed to here will
                        * persist - static declaration
                        */
}

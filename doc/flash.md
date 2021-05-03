# FLASH storage data

The DS2433 one-wire EEPROM on the SKARAB motherboard and mezzanine cards are
used to store various configuration data and statistics, as detailed below.

## SKARAB MOTHERBOARD EEPROM

This section details the data and configuration parameters stored in the SKARAB
motherboard DS2433 EEPROM.

### TUNABLE CONFIG PARAMETERS:

The following parameters are stored in **PAGE15** of the DS2433 one wire eeprom
on the SKARAB motherboard (one wire port 0x0):

DHCP INIT (16-bit value):<BR>
                          Time delay at startup before first dhcp packet is sent. This allows for
                          the links to initialise before the first dhcp packet is broadcast.
```
  pg15_byte[0] = LSB    {0x01e0}
  pg15_byte[1] = MSB    {0x01e1}
  init time (milliseconds) = ((byte[1] << 8) + byte[0]) * 100
```

DHCP RETRY RATE (16-bit value):<BR>
                                Time between dhcp retries, until dhcp is configured.
```
  pg15_byte[2] = LSB    {0x01e2}
  pg15_byte[3] = MSB    {0x01e3}
  retry time (milliseconds) = ((byte[3] << 8) + byte[2]) * 100
```

HMC RECONFIG TIMEOUT (per reset) (16-bit value):<BR>
                                                 Sets the timeout for the HMC reconfigure mechanism,
                                                 i.e.  the time within which all HMCs should
                                                 initialise otherwise the FPGA is reconfigured, in
                                                 line with the HMC bringup strategy (See further
                                                     details about the HMC reconfigure mechanism
                                                     below).
```
  pg15_byte[4] = LSB    {0x01e4}
  pg15_byte[5] = MSB    {0x01e5}
  hmc timeout (milliseconds) = ((byte[5] << 8) + byte[4]) * 100
```

HMC RECONFIG MAX RETRIES (8-bit value):<BR>
                                        Sets the maximum number of retries that the HMC reconfigure
                                        mechanism will attempt to bring up the HMCs, after which,
                                        the mechanism will give up. This prevents infinite
                                        "reconfigure-loops" in the case of a persistent HMC hardware
                                        issue or other initialisation issue.
```
  pg15_byte[6]          {0x01e6}
```

LINK MON TIMEOUT (16-bit value):<BR>
                                 The link monitor task attempts to recover the SKARAB from
                                 (suspected) rx-link failures.  The value set by this parameter
                                 determines the timeout (i.e. time to wait if the rx link does not
                                     come up) before rebooting the SKARAB.
```
  pg15_byte[7] = LSB    {0x01e7}
  pg15_byte[8] = MSB    {0x01e8}
  link mon timeout (milliseconds) = ((byte[8] << 8) + byte[7]) * 100
```

### SKARAB SERIAL NUMBER
This is the hardware serial number of the SKARAB unit<BR>
(For example: SKARAB02030B => '02' = upper octet; '03' = middle octet; '0B' = lower octet)
Stored in **PAGE0** of SKARAB motherboard FLASH.
```
  pg0_byte[1] = UPPER_OCTET
  pg0_byte[2] = MIDDLE_OCTET
  pg0_byte[3] = LOWER_OCTET
```

casperfpga python code example on skarab02030b:
```
In : serial = fpga.transport.one_wire_ds2433_read_mem(one_wire_port=0, num_bytes=3, page=0, offset=1)
In : print [hex(x) for x in serial ]
Out: ['0x2', '0x3', '0xb']
```

## MEZZANINE CARD EEPROM

This section details the parameters stored on the mezzanine cards.

### MEZZANINE SIGNATURES STORED IN MEZZ FLASH:

Each mezzanine card has a specific signature written to the following bytes in
**PAGE0** of the DS2433 one wire eeprom on the respective mezzanine card (one
wire ports 0x1 to 0x4):

***note the byte indices are not sequential i.e. index 0,4,5,6***

QSFP+ MEZZANINE:
```
  pg0_byte[0] = 0x50
  pg0_byte[4] = 0x01
  pg0_byte[5] = 0xE3
  pg0_byte[6] = 0x99
```

QSFP+ PHY MEZZANINE:
```
  pg0_byte[0] = 0x50
  pg0_byte[4] = 0x01
  pg0_byte[5] = 0xE3
  pg0_byte[6] = 0xFD
```

ADC32RF45X2 MEZZANINE:
```
  pg0_byte[0] = 0x50
  pg0_byte[4] = 0x01
  pg0_byte[5] = 0xE7
  pg0_byte[6] = 0xE5 || 0xE6 || 0xE7
```

HMC R1000-0005 MEZZANINE:
```
  pg0_byte[0] = 0x53
  pg0_byte[4] = 0xFF
  pg0_byte[5] = 0x00
  pg0_byte[6] = 0x01
```

casperfpga python code example for QSFP+ on mezzanine3:
```
In : mezz3 = fpga.transport.one_wire_ds2433_read_mem(one_wire_port=4, num_bytes=7, page=0, offset=0)
In : print [hex(x) for x in mezz3 ]
Out: ['0x50', '0x0', '0x0', '0x0', '0x1', '0xe3', '0x99']

#note: choose relevant one_wire_port number. See 'fpga.transport.one_wire_ds2433_read_mem?' for help.
```

### HMC RECONFIGURE ATTEMPTS STATS STORED IN HMC MEZZ FLASH:

The Microblaze employs a monitoring mechanism to ensure the reliable bring-up of the
HMC cards. This mechanism monitors the INIT OK and POST OK signals of the HMC cards
populated on the SKARAB and a timeout invokes a reconfiguration of the FPGA in an
attempt to recover failed HMC bootups. Usually, each SKARAB is populated with three HMC
cards. The counters documented below are the statistics recorded for these HMC
timeout induced reconfiguations. If the current card caused the timeout, both counters are
incremented and if the current card has not caused the reboot, then only the total
retries value is incremented. In this way one can see how many times *this* card has
caused a reboot and how many times *this* card has been involved in a reboot, but not
necessarily the cause of it.

The reconfigure stats for each HMC is stored in **PAGE15** of the DS2433 eeprom on the
respective hmc mezzanine card at the following bytes:

HMC RETRIES (for current card)
  (32-bit value)
```
  pg15_byte[0] = LSB    {0x01e0}
  pg15_byte[1]          {0x01e1}
  pg15_byte[2]          {0x01e2}
  pg15_byte[3] = MSB    {0x01e3}
```

HMC TOTAL RETRIES (for current card)
  (32-bit value)
```
  pg15_byte[4] = LSB    {0x01e4}
  pg15_byte[5]          {0x01e5}
  pg15_byte[6]          {0x01e6}
  pg15_byte[7] = MSB    {0x01e7}
```
casperfpga python code on HMC mezzanine in slot 0:
```
In  : mezz1_data = fpga.transport.one_wire_ds2433_read_mem(one_wire_port=1, num_bytes=8, page=15, offset=0)
In  : print mezz1_data
Out : [48, 3, 0, 0, 82, 5, 0, 0]
In  : mezz1_data[0] + (mezz1_data[1]<<8) + (mezz1_data[2]<<16) + (mezz1_data[3]<<24)
Out : 816
In  : mezz1_data[4] + (mezz1_data[5]<<8) + (mezz1_data[6]<<16) + (mezz1_data[7]<<24)
Out : 1362

#confirmed by the following command:

In  : fpga.transport.get_hmc_reconfigure_stats(0)
Out : {'hmc_0': {'hmc_init_failures_recorded': 816,
       'hmc_total_init_retries_recorded': 1362}}
```

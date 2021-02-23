# Command Line Interpreter

A basic Command Line Interpreter (CLI) is provided by the running Microblaze via the
serial console. This provides a useful interface for debugging.

## How the serial ports are enumerated on the SKARAB...  Each SKARAB serial
connection enumerates 4 serial devices on the host, namely ttyUSBxx. To list, type:

```% ls /dev/ttyUSB*```

The first SKARAB enumerated will be numbered 0 to 3, i.e ttyUSB0, ttyUSB1, ttyUSB2
and ttyUSB3. The UART connection to the Microblaze subsystem is always the third
enumerated serial device, i.e. /dev/ttyUSB2 in this case. Therefore, for subsequent
SKARABs connected to the host and depending on how many USB connections the host can
have, the UARTs will be connected on devices ttyUSB6, ttyUSB10, ttyUSB14, etc.

NOTE: A useful CLI command to determine which SKARAB is connected to your current
serial connection is the ```whoami``` command.

## Serial Connection
Using a serial communication program, such as minicom, connect to the relevant
ttyUSBx device with the following parameters:

```115200, 8N1, no HW or SW flow control```

## CLI Commands

To get the CLI prompt, type '?' over the serial connection. This has to be done each
time a command is to be entered. This will display the following list of available
commands:

```
usage:
log-level [trace|debug|info|warn|error|fatal|off]
log-select [general|dhcp|arp|icmp|igmp|lldp|ctrl|buff|hardw|iface|all]
bounce-link [0|1|2|3|4]
test-timer
get-config
get-mezz-inv
reset-fw
reboot-fpga [|flash|sdram]
stats
whoami
uname
uptime
dump [stack|heap]
if-map
igmp
peek [iface|dhcp]
wb-read [HEX]
arp-req [off|on|stat]
arp-proc [off|on|stat]
memtest
clr-link-mon-count
help

```

### Description

```log-level [trace|debug|info|warn|error|fatal|off]```
The Microblaze outputs runtime information to the serial console. This command can be
used to dial the log-level up or down for more or less info.

```log-select [general|dhcp|arp|icmp|igmp|lldp|ctrl|buff|hardw|iface|all]```
This allows for finer selection of logging output.

```bounce-link [0|1|2|3|4]```
This command “flaps” the selected link on the skarab. It allows a reset of the link
and is useful during debugging. The numeric arguments shown above are the IDs of the
links and map to the physical port location on the SKARAB as follows:
```
0 -> 1gbe
1 -> 40gbe port 0
2 -> 40gbe port 1
3 -> 40gbe port 2
4 -> 40gbe port 3
```

```test-timer```
This command can be used to test the interrupt timer of the microblaze. It was mainly
implemented to debug the reduced clock architecture but still remains useful for
debugging. The output of the timer should tick over at 1s intervals for a total of
5s.

```get-config```
This command displays the preprocessor macros which were enabled at build time i.e.
the configuration of the features selected in Makefile.config during compilation.

```get-mezz-inv```
Displays the hardware inventory and firmware status for each mezzanine slot.

```reset-fw```
Invokes a firmware reset by asserting bit 30 of register C_WR_BRD_CTL_STAT_0.

```reboot-fpga [|flash|sdram]```
Reboots the FPGA from the specified location in the argument field i.e. from the
multiboot image in flash or the toolflow image last uploaded to sdram. If this
argument is omitted, the logic will attempt to reboot the FPGA from the location it
was previously loaded from.

```stats```
Displays the network interface stats - tx and rx packet counters, ethernet link
status, ip and subnet.

```whoami```
Displays the name of the current skarab to which the serial console is connected.

```uname```
Displays the ELF build information as well as the location from which the FPGA was
loaded.

```uptime```
Displays the uptime of the Microblaze in seconds (not the SKARAB). In other words, if
a firmware reset was issued, this counter will reset and not indicate the SKARAB or
"hardware" uptime.

``dump [stack|heap]``
Dump the runtime stack or heap of the Microblaze to the display.

```if-map```
Display the mapping of the network interfaces built into the running image. It will
indicate which interfaces are present, the respective logical and physical IDs, as
well as the wishbone address offset. It will also indicate if it is the 1gbe or a
40gbe link.

```igmp```
Displays the IGMP base addr, mask and state machine status for each interface.

```peek [iface|dhcp]```
This allows one to peek into the internal state of data structures for the interface
and dhcp logic at runtime. This is useful for debugging any potential software
issues.

```wb-read [HEX]```
This command allows one to read data over the wishbone bus via the serial console. It
accepts a 32-bit formatted hex address.

```arp-req [off|on|stat]```
By default, the SKARAB sends out ARP requests (at about 10Hz) in order to populate the
ARP table in firmware. This command can be used to disable/enable these messages.
This is usefuli during debugging to quiet down the link or the trace-level output, if
necessary.

```arp-proc [off|on|stat]```
This can be used to disable ARP processing in the Microblaze (enabled by default).

```memtest```
Perform the memory test on the executable code in BRAM. This is the same test run at
Microblaze bootup. It calculates an Adler cheksum and compares it to the expected
value calculated at compile-and-link time.

```clr-link-mon-count```
The link monitor logic keeps state of the resets invoked by link issues. This counter
value is also added to the link monitor timeout upon each reset. This command
can be used to clear this counter.

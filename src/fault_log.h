#ifndef _FAULT_LOG_H_
#define _FAULT_LOG_H_

#include <xil_types.h>
#include "constant_defs.h"

/* current monitor logs */
int get_num_current_log_entries(u8 *num_entries);
int get_current_log_entry(const u8 index, sLogDataEntryT *log_out);

/* voltage monitor logs */
int get_num_voltage_log_entries(u8 *num_entries);
int get_voltage_log_entry(const u8 index, sLogDataEntryT *log_out);

/* fan controller logs */
/* get the next entry of the cyclical log entries */
int get_fanctrlr_log_entry_next(sFanCtrlrLogDataEntryT *log_out);
/* clear the logs */
int clear_fanctrlr_logs(void);

/* sync the time counters of the monitoring devices */
int log_time_sync_devices(void);

#endif /*_FAULT_LOG_H_ */

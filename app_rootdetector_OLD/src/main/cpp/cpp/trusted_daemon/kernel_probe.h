#ifndef KERNEL_PROBE_H
#define KERNEL_PROBE_H

#include "../sandbox_microkernel/micro_kernel.h"

#define KPROBE_INTERVAL_CYCLES 5

int kernel_probe_init(void);
int kernel_probe_check(void *result);
int kernel_probe_cycle(void);
int kernel_probe_snapshot_syscall_table(void *result);

#endif

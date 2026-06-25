#ifndef MEMORY_CHECKER_H
#define MEMORY_CHECKER_H

#include <sys/types.h>
#include "../sandbox_microkernel/plugin_interface.h"

int memory_checker_scan(void *result);
int memory_checker_scan_process(pid_t pid);

#endif

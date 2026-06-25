#ifndef ANTI_HOOK_H
#define ANTI_HOOK_H

#include "../sandbox_microkernel/plugin_interface.h"

int anti_hook_check_libraries(void);
int anti_hook_check_got(void);
int anti_hook_check(void *result);

#endif

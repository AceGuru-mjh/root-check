#ifndef ANTI_DEBUG_H
#define ANTI_DEBUG_H

#include <stdint.h>

void anti_debug_init(void);
int  anti_debug_check_tracer(void);
int  anti_debug_check_port(void);
int  anti_debug_integrity(void);

#endif

#ifndef RANDOM_LAYOUT_H
#define RANDOM_LAYOUT_H

#include <stddef.h>

void random_layout_init(void);
void *randomize_buffer_allocation(size_t base_size);
void  free_randomized_buffer(void *ptr, size_t base_size);

#endif

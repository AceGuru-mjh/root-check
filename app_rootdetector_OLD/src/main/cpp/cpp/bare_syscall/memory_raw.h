#ifndef MEMORY_RAW_H
#define MEMORY_RAW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_OK              0
#define MEM_ERR_INVALID     (-1)
#define MEM_ERR_OPEN        (-2)
#define MEM_ERR_SEEK        (-3)
#define MEM_ERR_READ        (-4)
#define MEM_ERR_WRITE       (-5)
#define MEM_ERR_MUNMAP      (-6)

#define MEM_PROT_READ       1
#define MEM_PROT_WRITE      2
#define MEM_PROT_EXEC       4
#define MEM_MAP_PRIVATE     0
#define MEM_MAP_SHARED      1

typedef struct {
    void   *start;
    void   *end;
    size_t  size;
    int     prot;
    int     flags;
} mem_region_t;

int raw_mem_read(void *addr, void *buf, size_t size);
int raw_mem_write(void *addr, const void *buf, size_t size);
void *alloc_anonymous(size_t size, int prot);
int free_anonymous(void *addr, size_t size);
void *alloc_executable(size_t size);
void *randomize_addr(void *base_addr, size_t max_offset);
uint64_t raw_random(void);
void *raw_memcpy(void *dest, const void *src, size_t n);
void *raw_memset(void *s, int c, size_t n);
int raw_memcmp(const void *s1, const void *s2, size_t n);
int get_memory_region(void *addr, mem_region_t *region);
int is_address_mapped(void *addr);

#ifdef __cplusplus
}
#endif

#endif

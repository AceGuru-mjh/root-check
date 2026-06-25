#ifndef PAGE_TABLE_WALKER_H
#define PAGE_TABLE_WALKER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTW_OK              0
#define PTW_ERR_ACCESS      (-1)
#define PTW_ERR_NOTFOUND    (-2)
#define PTW_ERR_PARAM       (-3)
#define PTW_ERR_MAP         (-4)

#define PTW_PROT_NONE       0
#define PTW_PROT_READ       1
#define PTW_PROT_WRITE      2
#define PTW_PROT_EXEC       4
#define PTW_PROT_ANON       8
#define PTW_PROT_PRIV       16

typedef struct {
    uint64_t           vaddr_start;
    uint64_t           vaddr_end;
    uint64_t           paddr;
    size_t             size;
    int                prot_flags;
    int                is_anonymous;
    int                is_executable;
    int                is_writable;
    int                is_suspicious;
} ptw_region_t;

typedef struct {
    ptw_region_t      *regions;
    int                region_count;
    int                region_capacity;
    int                suspicious_count;
} ptw_result_t;

int ptw_init(void);
int ptw_walk(ptw_result_t *result);
int ptw_find_executable_anon(ptw_result_t *result);
int ptw_check_address(uint64_t vaddr, ptw_region_t *region);
void ptw_free_result(ptw_result_t *result);

#ifdef __cplusplus
}
#endif

#endif

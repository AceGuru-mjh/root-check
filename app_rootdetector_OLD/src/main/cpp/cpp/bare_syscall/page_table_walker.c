#include "page_table_walker.h"
#include "bare_syscall.h"
#include "raw_file_io.h"
#include "syscall_arm64.h"
#include <string.h>
#include <stdint.h>

#define INITIAL_REGION_CAPACITY 256

static uint64_t parse_hex_addr(const char *str, int *consumed) {
    uint64_t val = 0;
    int count = 0;
    while (str[count]) {
        char c = str[count];
        if (c >= '0' && c <= '9')      val = (val << 4) | (uint64_t)(c - '0');
        else if (c >= 'a' && c <= 'f') val = (val << 4) | (uint64_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val = (val << 4) | (uint64_t)(c - 'A' + 10);
        else break;
        count++;
    }
    if (consumed) *consumed = count;
    return val;
}

static uint64_t read_ttbr0(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(val));
    return val;
}

static uint64_t read_ttbr1(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(val));
    return val;
}

int ptw_init(void) {
    uint64_t ttbr0 = read_ttbr0();
    uint64_t ttbr1 = read_ttbr1();
    if (ttbr0 == 0 && ttbr1 == 0) return PTW_ERR_ACCESS;
    return PTW_OK;
}

static int add_region(ptw_result_t *result, uint64_t start, uint64_t end,
                      int prot, int anon, int exec, int write) {
    if (result->region_count >= result->region_capacity) {
        int new_cap = result->region_capacity * 2;
        ptw_region_t *new_regions = (ptw_region_t *)bare_mmap(
            NULL, new_cap * sizeof(ptw_region_t),
            3, 0x22, -1, 0);
        if (bare_is_error((long)new_regions)) return PTW_ERR_MAP;

        for (int i = 0; i < result->region_count; i++) {
            new_regions[i] = result->regions[i];
        }
        bare_munmap(result->regions,
                    result->region_capacity * sizeof(ptw_region_t));
        result->regions = new_regions;
        result->region_capacity = new_cap;
    }

    ptw_region_t *r = &result->regions[result->region_count];
    r->vaddr_start = start;
    r->vaddr_end = end;
    r->paddr = 0;
    r->size = (size_t)(end - start);
    r->prot_flags = prot;
    r->is_anonymous = anon;
    r->is_executable = exec;
    r->is_writable = write;
    r->is_suspicious = (anon && exec) ? 1 : 0;

    if (r->is_suspicious) result->suspicious_count++;

    result->region_count++;
    return PTW_OK;
}

int ptw_walk(ptw_result_t *result) {
    if (!result) return PTW_ERR_PARAM;

    result->regions = (ptw_region_t *)bare_mmap(
        NULL, INITIAL_REGION_CAPACITY * sizeof(ptw_region_t),
        3, 0x22, -1, 0);
    if (bare_is_error((long)result->regions)) return PTW_ERR_MAP;

    result->region_count = 0;
    result->region_capacity = INITIAL_REGION_CAPACITY;
    result->suspicious_count = 0;

    char *maps_buf = NULL;
    size_t maps_size = 0;

    int ret = read_proc_node("/proc/self/maps", &maps_buf, &maps_size);
    if (ret != RAW_IO_OK) {
        bare_munmap(result->regions,
                    INITIAL_REGION_CAPACITY * sizeof(ptw_region_t));
        return PTW_ERR_ACCESS;
    }

    const char *line = maps_buf;
    while (line && *line) {
        int consumed;
        uint64_t start = parse_hex_addr(line, &consumed);
        if (consumed == 0) break;
        line += consumed;

        if (*line != '-') break;
        line++;

        uint64_t end = parse_hex_addr(line, &consumed);
        if (consumed == 0) break;
        line += consumed;

        while (*line == ' ') line++;

        int prot = 0;
        int exec = 0;
        int write = 0;
        if (*line == 'r') prot |= PTW_PROT_READ;
        line++;
        if (*line == 'w') { prot |= PTW_PROT_WRITE; write = 1; }
        line++;
        if (*line == 'x') { prot |= PTW_PROT_EXEC; exec = 1; }
        line++;
        line++;

        while (*line && *line != '\n') {
            if (*line == ' ') { line++; continue; }
            break;
        }

        int anon = 0;
        const char *path_start = line;
        while (*line && *line != '\n') line++;

        if (path_start < line) {
            int path_len = (int)(line - path_start);
            if (path_len == 0 ||
                (path_len == 1 && path_start[0] == '0')) {
                anon = 1;
            }
        }

        add_region(result, start, end, prot, anon, exec, write);

        if (*line == '\n') line++;
    }

    raw_free_file(maps_buf, maps_size);
    return PTW_OK;
}

int ptw_find_executable_anon(ptw_result_t *result) {
    if (!result) return PTW_ERR_PARAM;

    int found = 0;
    for (int i = 0; i < result->region_count; i++) {
        if (result->regions[i].is_anonymous &&
            result->regions[i].is_executable) {
            result->regions[i].is_suspicious = 1;
            found++;
        }
    }
    result->suspicious_count = found;
    return found;
}

int ptw_check_address(uint64_t vaddr, ptw_region_t *region) {
    if (!region) return PTW_ERR_PARAM;

    ptw_result_t result;
    int ret = ptw_walk(&result);
    if (ret != PTW_OK) return ret;

    int found = 0;
    for (int i = 0; i < result.region_count; i++) {
        if (vaddr >= result.regions[i].vaddr_start &&
            vaddr < result.regions[i].vaddr_end) {
            *region = result.regions[i];
            found = 1;
            break;
        }
    }

    ptw_free_result(&result);
    return found ? PTW_OK : PTW_ERR_NOTFOUND;
}

void ptw_free_result(ptw_result_t *result) {
    if (result && result->regions) {
        bare_munmap(result->regions,
                    result->region_capacity * sizeof(ptw_region_t));
        result->regions = NULL;
        result->region_count = 0;
        result->region_capacity = 0;
        result->suspicious_count = 0;
    }
}

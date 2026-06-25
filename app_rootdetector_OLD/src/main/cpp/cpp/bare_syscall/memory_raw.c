/*
 * memory_raw.c - Raw memory operations implementation
 *
 * Domain D: 底层裸系统汇编抽象层
 * All operations use bare syscalls, NO libc dependency
 */

#include "memory_raw.h"
#include "syscall_arm64.h"
#include "raw_file_io.h"

/* Internal helper: allocate anonymous mmap region */
static void *bare_mmap_alloc(size_t size) {
    long ret = bare_mmap(NULL, size, BARE_PROT_READ | BARE_PROT_WRITE,
                         BARE_MAP_PRIVATE | BARE_MAP_ANONYMOUS, -1, 0);
    if (bare_is_error(ret)) {
        return NULL;
    }
    return (void *)ret;
}

/* Helper: parse hex number from string */
static uint64_t parse_hex(const char *str, int *consumed) {
    uint64_t val = 0;
    int count = 0;

    while (str[count]) {
        char c = str[count];
        if (c >= '0' && c <= '9') {
            val = (val << 4) | (uint64_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            val = (val << 4) | (uint64_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            val = (val << 4) | (uint64_t)(c - 'A' + 10);
        } else {
            break;
        }
        count++;
    }

    if (consumed) *consumed = count;
    return val;
}

__attribute__((visibility("default")))
int raw_mem_read(void *addr, void *buf, size_t size) {
    if (!addr || !buf || size == 0) {
        return MEM_ERR_INVALID;
    }

    /* Open /proc/self/mem */
    long fd = bare_openat(BARE_AT_FDCWD, "/proc/self/mem", BARE_O_RDONLY, 0);
    if (bare_is_error(fd)) {
        return MEM_ERR_OPEN;
    }

    /* Seek to address */
    long ret = bare_lseek((int)fd, (long)addr, BARE_SEEK_SET);
    if (bare_is_error(ret)) {
        bare_close((int)fd);
        return MEM_ERR_SEEK;
    }

    /* Read memory */
    size_t total_read = 0;
    while (total_read < size) {
        ret = bare_read((int)fd, (char *)buf + total_read, size - total_read);
        if (bare_is_error(ret)) {
            bare_close((int)fd);
            return MEM_ERR_READ;
        }
        if (ret == 0) break;
        total_read += (size_t)ret;
    }

    bare_close((int)fd);
    return (total_read == size) ? MEM_OK : MEM_ERR_READ;
}

__attribute__((visibility("default")))
int raw_mem_write(void *addr, const void *buf, size_t size) {
    if (!addr || !buf || size == 0) {
        return MEM_ERR_INVALID;
    }

    /* Open /proc/self/mem for writing */
    long fd = bare_openat(BARE_AT_FDCWD, "/proc/self/mem", BARE_O_WRONLY, 0);
    if (bare_is_error(fd)) {
        return MEM_ERR_OPEN;
    }

    /* Seek to address */
    long ret = bare_lseek((int)fd, (long)addr, BARE_SEEK_SET);
    if (bare_is_error(ret)) {
        bare_close((int)fd);
        return MEM_ERR_SEEK;
    }

    /* Write memory */
    size_t total_written = 0;
    while (total_written < size) {
        ret = bare_write((int)fd, (const char *)buf + total_written, size - total_written);
        if (bare_is_error(ret)) {
            bare_close((int)fd);
            return MEM_ERR_WRITE;
        }
        if (ret == 0) break;
        total_written += (size_t)ret;
    }

    bare_close((int)fd);
    return (total_written == size) ? MEM_OK : MEM_ERR_WRITE;
}

__attribute__((visibility("default")))
void *alloc_anonymous(size_t size, int prot) {
    if (size == 0) return NULL;

    /* Round up to page size (4KB) */
    size_t page_size = 4096;
    size_t alloc_size = (size + page_size - 1) & ~(page_size - 1);

    long ret = bare_mmap(NULL, alloc_size, prot,
                         BARE_MAP_PRIVATE | BARE_MAP_ANONYMOUS, -1, 0);
    if (bare_is_error(ret)) {
        return NULL;
    }

    return (void *)ret;
}

__attribute__((visibility("default")))
int free_anonymous(void *addr, size_t size) {
    if (!addr || size == 0) {
        return MEM_ERR_INVALID;
    }

    /* Round up to page size */
    size_t page_size = 4096;
    size_t alloc_size = (size + page_size - 1) & ~(page_size - 1);

    long ret = bare_munmap(addr, alloc_size);
    if (bare_is_error(ret)) {
        return MEM_ERR_MUNMAP;
    }

    return MEM_OK;
}

__attribute__((visibility("default")))
void *alloc_executable(size_t size) {
    return alloc_anonymous(size, BARE_PROT_READ | BARE_PROT_WRITE | BARE_PROT_EXEC);
}

__attribute__((visibility("default")))
void *randomize_addr(void *base_addr, size_t max_offset) {
    if (!base_addr || max_offset == 0) {
        return base_addr;
    }

    /* Get random offset using cycle counter */
    uint64_t cycle;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycle));

    /* Mix with address for better distribution */
    uint64_t addr_val = (uint64_t)(uintptr_t)base_addr;
    cycle ^= addr_val;
    cycle = (cycle >> 13) ^ cycle;
    cycle = (cycle << 7) ^ cycle;
    cycle = (cycle >> 17) ^ cycle;

    /* Calculate offset within range, aligned to 4KB page */
    size_t offset = (size_t)(cycle % (uint64_t)max_offset);
    offset &= ~(size_t)0xFFF;

    return (void *)((char *)base_addr + offset);
}

__attribute__((visibility("default")))
uint64_t raw_random(void) {
    uint64_t val1, val2;

    /* Read cycle counter twice for entropy */
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val1));
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val2));

    /* Mix values using xorshift-style operations */
    val1 ^= val2 << 13;
    val1 ^= val1 >> 7;
    val1 ^= val2 << 17;

    return val1;
}

__attribute__((visibility("default")))
void *raw_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    /* Word-aligned copy for performance */
    if (n >= 16 && ((uintptr_t)d & 7) == 0 && ((uintptr_t)s & 7) == 0) {
        uint64_t *d64 = (uint64_t *)d;
        const uint64_t *s64 = (const uint64_t *)s;

        while (n >= 64) {
            d64[0] = s64[0]; d64[1] = s64[1];
            d64[2] = s64[2]; d64[3] = s64[3];
            d64[4] = s64[4]; d64[5] = s64[5];
            d64[6] = s64[6]; d64[7] = s64[7];
            d64 += 8;
            s64 += 8;
            n -= 64;
        }

        while (n >= 8) {
            *d64++ = *s64++;
            n -= 8;
        }

        d = (unsigned char *)d64;
        s = (const unsigned char *)s64;
    }

    /* Byte copy for remainder */
    while (n--) {
        *d++ = *s++;
    }

    return dest;
}

__attribute__((visibility("default")))
void *raw_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char val = (unsigned char)c;

    /* Word-aligned set for performance */
    if (n >= 16 && ((uintptr_t)p & 7) == 0) {
        uint64_t *p64 = (uint64_t *)p;
        uint64_t val64 = (uint64_t)val;
        val64 |= (val64 << 8);
        val64 |= (val64 << 16);
        val64 |= (val64 << 32);

        while (n >= 64) {
            p64[0] = val64; p64[1] = val64;
            p64[2] = val64; p64[3] = val64;
            p64[4] = val64; p64[5] = val64;
            p64[6] = val64; p64[7] = val64;
            p64 += 8;
            n -= 64;
        }

        while (n >= 8) {
            *p64++ = val64;
            n -= 8;
        }

        p = (unsigned char *)p64;
    }

    /* Byte set for remainder */
    while (n--) {
        *p++ = val;
    }

    return s;
}

__attribute__((visibility("default")))
int raw_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;

    /* Word-aligned compare for performance */
    if (n >= 8 && ((uintptr_t)a & 7) == 0 && ((uintptr_t)b & 7) == 0) {
        const uint64_t *a64 = (const uint64_t *)a;
        const uint64_t *b64 = (const uint64_t *)b;

        while (n >= 8) {
            if (*a64 != *b64) {
                a = (const unsigned char *)a64;
                b = (const unsigned char *)b64;
                goto byte_compare;
            }
            a64++;
            b64++;
            n -= 8;
        }

        a = (const unsigned char *)a64;
        b = (const unsigned char *)b64;
    }

byte_compare:
    while (n--) {
        if (*a != *b) {
            return (int)(*a) - (int)(*b);
        }
        a++;
        b++;
    }

    return 0;
}

__attribute__((visibility("default")))
int get_memory_region(void *addr, mem_region_t *region) {
    if (!addr || !region) {
        return MEM_ERR_INVALID;
    }

    char *maps_buf = NULL;
    size_t maps_size = 0;

    int ret = read_proc_node("/proc/self/maps", &maps_buf, &maps_size);
    if (ret != RAW_IO_OK) {
        return MEM_ERR_READ;
    }

    uint64_t target = (uint64_t)(uintptr_t)addr;
    const char *line = maps_buf;
    int found = 0;

    while (line && *line) {
        int consumed;
        uint64_t start = parse_hex(line, &consumed);
        if (consumed == 0) break;
        line += consumed;

        if (*line != '-') {
            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
            continue;
        }
        line++;

        uint64_t end = parse_hex(line, &consumed);
        line += consumed;

        if (target >= start && target < end) {
            region->start = (void *)(uintptr_t)start;
            region->end = (void *)(uintptr_t)end;
            region->size = (size_t)(end - start);

            /* Parse permissions: skip spaces */
            while (*line == ' ') line++;

            int prot = 0;
            if (*line == 'r') prot |= MEM_PROT_READ;
            line++;
            if (*line == 'w') prot |= MEM_PROT_WRITE;
            line++;
            if (*line == 'x') prot |= MEM_PROT_EXEC;
            line++;

            region->prot = prot;
            region->flags = MEM_MAP_PRIVATE;

            found = 1;
            break;
        }

        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }

    raw_free_file(maps_buf, maps_size);
    return found ? MEM_OK : MEM_ERR_READ;
}

__attribute__((visibility("default")))
int is_address_mapped(void *addr) {
    mem_region_t region;
    int ret = get_memory_region(addr, &region);
    return (ret == MEM_OK) ? 1 : 0;
}

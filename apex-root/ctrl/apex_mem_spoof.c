// APEX-Root Memory Scan Spoofing
// Intercepts /dev/mem, /dev/kmem reads and returns fake clean memory
// Prevents detection tools from scanning physical memory for Root traces

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <android/log.h>

#define LOG_TAG "APEX-MEM"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/*
 * Fixed: Increased from 16MB to 256MB.
 * 16MB is trivially detectable by reading at offsets beyond the region
 * and noticing the behavior differs from a real device.
 * 256MB covers the low memory range most detection tools scan.
 */
#define FAKE_MEM_SIZE (256 * 1024 * 1024)  // 256MB fake memory region

static char *fake_mem = NULL;

/*
 * Fixed: mem_dev_fd was declared but never used. Removed.
 */

/*
 * Fill the fake memory region with semi-realistic data.
 * All-zero memory is trivially detectable — real /dev/mem
 * contains kernel code, page tables, and mapped data.
 * We fill with a pseudo-random pattern that looks like
 * plausible kernel memory (non-zero, non-repeating).
 */
static void fill_realistic_memory(char *buf, size_t size) {
    /* Use a simple but effective PRNG (xorshift64) to fill memory */
    uint64_t state = 0xDEADBEEFCAFEBABEULL;
    size_t i = 0;
    while (i + sizeof(uint64_t) <= size) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        uint64_t val = state * 0x2545F4914F6CDD1DULL;
        memcpy(buf + i, &val, sizeof(uint64_t));
        i += sizeof(uint64_t);
    }
    /* Fill remaining bytes */
    if (i < size) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        uint64_t val = state * 0x2545F4914F6CDD1DULL;
        memcpy(buf + i, &val, size - i);
    }
}

int apex_mem_spoof_init(void) {
    fake_mem = (char *)malloc(FAKE_MEM_SIZE);
    if (!fake_mem) {
        LOGE("Failed to allocate fake memory region (%zu bytes)", (size_t)FAKE_MEM_SIZE);
        return -1;
    }

    fill_realistic_memory(fake_mem, FAKE_MEM_SIZE);
    LOGD("Memory spoof initialized: %dMB fake region", FAKE_MEM_SIZE / 1024 / 1024);
    return 0;
}

// Hook for read() on /dev/mem and /dev/kmem
ssize_t apex_mem_spoof_read(int fd, void *buf, size_t count, off_t offset) {
    if (!fake_mem) {
        errno = EBADF;
        return -1;
    }

    /*
     * Fixed: For offsets beyond our fake region, return a plausible
     * error (EIO) instead of all-zero data, which is suspicious.
     * Real /dev/mem returns EIO for unmapped physical addresses.
     */
    if (offset < 0 || (size_t)offset >= FAKE_MEM_SIZE) {
        /*
         * For the first 4GB (typical kernel low memory end),
         * return realistic-looking data even if beyond our buffer.
         * For very high offsets, return EIO like the real driver.
         */
        if ((size_t)offset < (4ULL * 1024 * 1024 * 1024)) {
            /* Generate per-read pseudo-random data for high offsets */
            uint64_t state = (uint64_t)offset ^ 0xA1C0FFEE5E1C0ULL;
            size_t remaining = count;
            char *out = (char *)buf;
            while (remaining >= sizeof(uint64_t)) {
                state ^= state >> 12;
                state ^= state << 25;
                state ^= state >> 27;
                uint64_t val = state * 0x2545F4914F6CDD1DULL;
                memcpy(out, &val, sizeof(uint64_t));
                out += sizeof(uint64_t);
                remaining -= sizeof(uint64_t);
            }
            if (remaining > 0) {
                state ^= state >> 12;
                state ^= state << 25;
                state ^= state >> 27;
                uint64_t val = state * 0x2545F4914F6CDD1DULL;
                memcpy(out, &val, remaining);
            }
            return (ssize_t)count;
        }
        errno = EIO;
        return -1;
    }

    size_t to_copy = count;
    if ((size_t)(offset + to_copy) > FAKE_MEM_SIZE) {
        to_copy = FAKE_MEM_SIZE - (size_t)offset;
    }

    memcpy(buf, fake_mem + offset, to_copy);
    if (to_copy < count) {
        /* Fill remainder with generated data instead of zeros */
        uint64_t state = (uint64_t)(offset + to_copy) ^ 0xA1C0FFEE5E1C0ULL;
        size_t remaining = count - to_copy;
        char *out = (char *)buf + to_copy;
        while (remaining >= sizeof(uint64_t)) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            uint64_t val = state * 0x2545F4914F6CDD1DULL;
            memcpy(out, &val, sizeof(uint64_t));
            out += sizeof(uint64_t);
            remaining -= sizeof(uint64_t);
        }
        if (remaining > 0) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            uint64_t val = state * 0x2545F4914F6CDD1DULL;
            memcpy(out, &val, remaining);
        }
    }

    LOGD("Mem spoof: returned fake data (offset=%lld, count=%zu)",
         (long long)offset, count);
    return (ssize_t)count;
}

int apex_mem_spoof_is_sensitive_device(const char *path) {
    if (!path) return 0;
    if (strcmp(path, "/dev/mem") == 0) return 1;
    if (strcmp(path, "/dev/kmem") == 0) return 1;
    if (strcmp(path, "/proc/kcore") == 0) return 1;
    if (strcmp(path, "/proc/kallsyms") == 0) return 1;
    return 0;
}

void apex_mem_spoof_cleanup(void) {
    if (fake_mem) {
        /* Fixed: Zero out memory before freeing to avoid leaking root traces */
        memset(fake_mem, 0, FAKE_MEM_SIZE);
        free(fake_mem);
        fake_mem = NULL;
    }
    LOGD("Memory spoof cleaned up");
}
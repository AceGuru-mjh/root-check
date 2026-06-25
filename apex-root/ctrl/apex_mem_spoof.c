// APEX-Root Memory Scan Spoofing
// Intercepts /dev/mem, /dev/kmem reads and returns fake clean memory
// Prevents detection tools from scanning physical memory for Root traces

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <android/log.h>

#define LOG_TAG "APEX-MEM"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define FAKE_MEM_SIZE (16 * 1024 * 1024)  // 16MB fake memory region
static char *fake_mem = NULL;
static int mem_dev_fd = -1;

int apex_mem_spoof_init(void) {
    fake_mem = (char *)malloc(FAKE_MEM_SIZE);
    if (!fake_mem) {
        LOGE("Failed to allocate fake memory region");
        return -1;
    }

    memset(fake_mem, 0, FAKE_MEM_SIZE);
    LOGD("Memory spoof initialized: %dMB fake region", FAKE_MEM_SIZE / 1024 / 1024);
    return 0;
}

// Hook for read() on /dev/mem and /dev/kmem
int apex_mem_spoof_read(int fd, void *buf, size_t count, off_t offset) {
    if (!fake_mem) {
        return -1;
    }

    // Return fake zeroed memory as if device has no Root traces
    size_t to_copy = count;
    if (offset + to_copy > FAKE_MEM_SIZE) {
        to_copy = FAKE_MEM_SIZE - offset;
    }

    if (offset >= FAKE_MEM_SIZE) {
        memset(buf, 0, count);
        LOGD("Mem spoof: returned zeroed data (offset=%lld, count=%zu)",
             (long long)offset, count);
        return count;
    }

    memcpy(buf, fake_mem + offset, to_copy);
    if (to_copy < count) {
        memset((char *)buf + to_copy, 0, count - to_copy);
    }

    LOGD("Mem spoof: returned fake data (offset=%lld, count=%zu, copied=%zu)",
         (long long)offset, count, to_copy);
    return count;
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
        free(fake_mem);
        fake_mem = NULL;
    }
    LOGD("Memory spoof cleaned up");
}

#include "layer5_sidechannel.h"
#include "../common/syscall.h"
#include <cstring>

static int64_t get_ns() {
    int64_t ts[2];
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0"
                 : : "i"(__NR_clock_gettime), "i"(1), "r"(ts)
                 : "x0", "x1", "x8");
    return ts[0] * 1000000000LL + ts[1];
}

static int64_t measure_syscall(int nr) {
    int64_t start = get_ns();
    int64_t ret;
    asm volatile("mov x8, %1; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(nr) : "x0", "x8");
    int64_t end = get_ns();
    return end - start;
}

bool detectSyscallTimingAnomaly() {
    // Measure several syscalls. Hooked syscalls take significantly longer.
    int64_t baseline = measure_syscall(__NR_getpid);
    int64_t open_at = measure_syscall(__NR_openat);
    int64_t read_ts = measure_syscall(__NR_read);

    // If read/openat are way slower than getpid, likely hooked
    // Normal: all within 2x of baseline
    if (baseline < 100) baseline = 100;
    if (baseline > 10000) return true; // syscall table likely intercepted

    if (open_at > baseline * 10 && read_ts > baseline * 10) return true;
    return false;
}

bool detectCacheTimingAnomaly() {
    // Measure small syscall loop to detect cache-based detection bypass
    int64_t times[10];
    for (int i = 0; i < 10; i++) {
        times[i] = measure_syscall(__NR_getpid);
    }

    // Calculate variance
    int64_t avg = 0;
    for (int i = 0; i < 10; i++) avg += times[i];
    avg /= 10;

    int64_t variance = 0;
    for (int i = 0; i < 10; i++) {
        int64_t diff = times[i] - avg;
        variance += diff * diff;
    }
    variance /= 10;

    // Very high variance suggests timing manipulation
    return variance > avg * avg;
}

bool detectBinderLatencyAnomaly() {
    // Check if /dev/binder is accessible (should not be from app context
    // if it is, system likely has modified SELinux policy)
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/binder"), "i"(O_RDWR), "i"(0));
    if (fd >= 0) {
        int64_t d;
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
        return true; // App can access binder directly = SELinux modified
    }
    return false;
}

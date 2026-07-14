#include "selinux_context.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdio>

// ─── Raw file I/O helpers ──────────────────────────────────

static bool read_file_at(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    #else
        d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

static int64_t open_file(const char* path, int flags) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "r"(flags), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    return fd;
}

static void close_fd(int64_t fd) {
    if (fd >= 0) {
        int64_t d;
        #if defined(__aarch64__)
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
        #else
            d = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
    }
}

// ─── SELinux context reading via /proc/self/attr/current ───

static bool read_own_context(char* buf, size_t size) {
    return read_file_at("/proc/self/attr/current", buf, size);
}

// ─── Check for SELinux context jump (Shamiko technique) ────

bool detectSELinuxContextJump() {
    // Shamiko changes SELinux context at runtime to hide its presence
    // Read current context
    char context[256];
    if (!read_own_context(context, sizeof(context))) return false;

    // Strip newline
    size_t clen = strlen(context);
    while (clen > 0 && (context[clen-1] == '\n' || context[clen-1] == '\r')) context[--clen] = '\0';

    // A normal app context is u:r:untrusted_app:s0[:c...]
    // If we see magisk, su, or other privileged contexts, it's a jump
    if (strstr(context, "magisk")) return true;
    if (strstr(context, ":su:")) return true;
    if (strstr(context, "kernel")) return true;
    if (strstr(context, "init")) return true;

    // Check if the context changed from what we'd expect
    // Read /proc/self/attr/prev for previous context
    char prev[256];
    if (read_file_at("/proc/self/attr/prev", prev, sizeof(prev))) {
        size_t plen = strlen(prev);
        while (plen > 0 && (prev[plen-1] == '\n' || prev[plen-1] == '\r')) prev[--plen] = '\0';
        if (plen > 0 && strcmp(context, prev) != 0) {
            // Context jump detected
            return true;
        }
    }

    return false;
}

// ─── Check if SELinux is permissive ────────────────────────

bool detectSELinuxPermissive() {
    char buf[16];
    if (!read_file_at("/sys/fs/selinux/enforce", buf, sizeof(buf))) {
        // If selinuxfs is not mounted, SELinux may be disabled
        // Check if it's just not accessible vs non-existent
        int64_t fd = open_file("/sys/fs/selinux", O_RDONLY);
        if (fd < 0) {
            // selinuxfs not accessible at all - suspicious
            return true;
        }
        close_fd(fd);
        return false;
    }
    // 0 = permissive, 1 = enforcing
    return buf[0] == '0';
}

// ─── Check for SELinux policy modification ─────────────────

bool detectSELinuxPolicyModification() {
    char buf[4096];

    // Check if /sys/fs/selinux/access is accessible (should not be for apps)
    int64_t fd = open_file("/sys/fs/selinux/access", O_RDONLY);
    if (fd >= 0) {
        close_fd(fd);
        // App can access selinuxfs directly = policy modified
        return true;
    }

    // Check for Shamiko's sepolicy.override
    if (read_file_at("/data/adb/modules/shamiko/sepolicy.override", buf, sizeof(buf))) {
        return true;
    }

    // Check for magiskpolicy in memory
    if (read_file_at("/proc/self/maps", buf, sizeof(buf))) {
        if (strstr(buf, "magiskpolicy")) return true;
    }

    // Check if we can read /sys/fs/selinux/policy (should be root-restricted)
    fd = open_file("/sys/fs/selinux/policy", O_RDONLY);
    if (fd >= 0) {
        close_fd(fd);
        return true;
    }

    return false;
}

// ─── Full SELinux scan ─────────────────────────────────────

int runSELinuxFullScan(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;

    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectSELinuxPermissive()) {
        append("[SELinux] Permissive mode detected\n"); findings++;
    }
    if (detectSELinuxContextJump()) {
        append("[SELinux] Context jump detected (Shamiko?)\n"); findings++;
    }
    if (detectSELinuxPolicyModification()) {
        append("[SELinux] Policy modification detected\n"); findings++;
    }

    // Check actual context
    char ctx[256];
    if (read_own_context(ctx, sizeof(ctx))) {
        size_t clen = strlen(ctx);
        while (clen > 0 && (ctx[clen-1] == '\n' || ctx[clen-1] == '\r')) ctx[--clen] = '\0';
        append("[SELinux] Current context: ");
        append(ctx);
        append("\n");
    }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}

#include "layer3_mem.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>

// ─── Memory fingerprint database ───────────────────────────

struct MemSignature {
    const char* name;
    const char* pattern;
    size_t pattern_len;
    bool (*matcher)(const char* maps_buf, size_t maps_len);
};

// Common memory name matchers
static bool match_lib(const char* maps, size_t len, const char* lib_name) {
    return strstr(maps, lib_name) != nullptr;
}
static bool match_anon_exec(const char* maps, size_t len) {
    // Match anonymous executable mappings: anon_inode: or [anon: with r-xp
    const char* markers[] = {
        "anon_inode:", " [anon:", "[stack:", "[heap:"
    };
    for (auto m : markers) {
        const char* p = maps;
        while ((p = strstr(p, m)) != nullptr) {
            // Check if followed by r-xp (executable)
            const char* nl = p;
            while (nl > maps && *nl != '\n') nl--;
            if (*nl == '\n') nl++;
            // Find perms field
            while (*nl == ' ' || *nl == '\t') nl++;
            const char* space = nl;
            while (*space && *space != ' ') space++;
            if (space - nl >= 4 && nl[0] == 'r' && nl[1] == '-' && nl[2] == 'x') return true;
            p++;
        }
    }
    return false;
}
static bool match_memfd(const char* maps, size_t len) {
    return strstr(maps, "/memfd:") != nullptr;
}
static bool match_dmabuf(const char* maps, size_t len) {
    return strstr(maps, "dmabuf") != nullptr;
}

static const MemSignature MEMORY_SIGNATURES[] = {
    {"magiskd",       nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "magisk"); }},
    {"magiskpolicy",  nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "magiskpolicy"); }},
    {"magiskinit",    nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "magiskinit"); }},
    {"zygisk",        nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "zygisk"); }},
    {"libzygisk",     nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "libzygisk"); }},
    {"lsposed",       nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "lspd") || match_lib(m, l, "lsposed"); }},
    {"libriru",       nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "libriru"); }},
    {"shamiko",       nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "shamiko"); }},
    {"frida",         nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "frida") || match_lib(m, l, "gum-js-loop") || match_lib(m, l, "frida-agent"); }},
    {"xposed",        nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "xposed") || match_lib(m, l, "edxp"); }},
    {"substrate",     nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "substrate") || match_lib(m, l, "cydia"); }},
    {"dobby",         nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "dobby") || match_lib(m, l, "libdobby"); }},
    {"whale",         nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "whale") || match_lib(m, l, "libwhale"); }},
    {"anon_exec",     nullptr, 0, match_anon_exec},
    {"memfd_private", nullptr, 0, match_memfd},
    {"zygisknext",    nullptr, 0, [](const char* m, size_t l) { return match_lib(m, l, "zygisknext") || match_memfd(m, l); }},
};

#define NUM_SIGNATURES (sizeof(MEMORY_SIGNATURES) / sizeof(MEMORY_SIGNATURES[0]))

// ─── Read /proc/self/maps via raw syscall ──────────────────

static bool read_maps(char* buf, size_t size, size_t* out_len) {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/maps"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    int64_t dummy;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    *out_len = (size_t)n;
    return true;
}

// ─── Public API ────────────────────────────────────────────

bool detectSuspiciousMemory() {
    char buf[16384];
    size_t len = 0;
    if (!read_maps(buf, sizeof(buf), &len)) return true;

    // Check for RWX mappings (unusual)
    int rwx_count = 0;
    char* line = buf;
    char* end = buf + len;
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';

        char* perms = line;
        while (*perms && *perms != ' ') perms++;
        if (*perms == ' ') perms++;
        if (strncmp(perms, "rwx", 3) == 0) rwx_count++;

        line = nl + 1;
    }
    return rwx_count > 3;
}

bool detectHiddenProcessMemory() {
    char buf[4096];
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/status"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
    int64_t dummy;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    if (n <= 0) return false;
    buf[n < (int64_t)sizeof(buf) ? n : (int64_t)sizeof(buf)-1] = '\0';

    const char tracer_mark[] = "TracerPid:";
    const char* tracer = strstr(buf, tracer_mark);
    if (tracer) {
        tracer += sizeof(tracer_mark) - 1;
        while (*tracer == ' ' || *tracer == '\t') tracer++;
        if (*tracer != '0') return true;
    }
    return false;
}

// ─── Deep memory fingerprint scan ──────────────────────────

int deepMemoryFingerprintScan(char* out_report, size_t out_size) {
    char buf[65536];
    size_t maps_len = 0;
    if (!read_maps(buf, sizeof(buf), &maps_len)) {
        if (out_size > 0) out_report[0] = '\0';
        return 0;
    }

    int findings = 0;
    int pos = 0;

    for (size_t i = 0; i < NUM_SIGNATURES; i++) {
        // Check RWX count
        if (MEMORY_SIGNATURES[i].matcher(buf, maps_len)) {
            const char* fmt = "[MEM] %s detected\n";
            int remaining = (int)out_size - pos - 1;
            if (remaining > 0) {
                int written = 0;
                for (const char* p = fmt; *p; p++) {
                    if (*p == '%' && *(p+1) == 's') {
                        for (const char* n = MEMORY_SIGNATURES[i].name; *n; n++) {
                            if (pos < (int)out_size - 1) out_report[pos++] = *n;
                        }
                        p++;
                    } else {
                        if (pos < (int)out_size - 1) out_report[pos++] = *p;
                    }
                }
            }
            findings++;
        }
    }
    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}

bool detectMagiskMemory() {
    char buf[65536]; size_t len = 0;
    return read_maps(buf, sizeof(buf), &len) &&
           (match_lib(buf, len, "magisk") || match_lib(buf, len, "magiskinit"));
}

bool detectZygiskMemory() {
    char buf[65536]; size_t len = 0;
    return read_maps(buf, sizeof(buf), &len) &&
           (match_lib(buf, len, "zygisk") || match_lib(buf, len, "libzygisk"));
}

bool detectLSPosedMemory() {
    char buf[65536]; size_t len = 0;
    return read_maps(buf, sizeof(buf), &len) &&
           (match_lib(buf, len, "lspd") || match_lib(buf, len, "lsposed"));
}

bool detectShamikoMemory() {
    char buf[65536]; size_t len = 0;
    return read_maps(buf, sizeof(buf), &len) &&
           (match_lib(buf, len, "shamiko") || match_anon_exec(buf, len));
}

bool detectFridaMemory() {
    char buf[65536]; size_t len = 0;
    return read_maps(buf, sizeof(buf), &len) &&
           (match_lib(buf, len, "frida") || match_lib(buf, len, "frida-agent") ||
            match_lib(buf, len, "gum") || match_lib(buf, len, "gadget"));
}

int countRWXPages() {
    char buf[16384]; size_t len = 0;
    if (!read_maps(buf, sizeof(buf), &len)) return -1;
    int count = 0;
    char* line = buf; char* end = buf + len;
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';
        char* perms = line;
        while (*perms && *perms != ' ') perms++;
        if (*perms == ' ') perms++;
        if (strncmp(perms, "rwx", 3) == 0) count++;
        line = nl + 1;
    }
    return count;
}

int fullMemoryFingerprintScan() {
    char buf[65536]; size_t len = 0;
    if (!read_maps(buf, sizeof(buf), &len)) return 0;
    int mask = 0;
    if (match_lib(buf, len, "magisk"))       mask |= MEM_FINGERPRINT_MAGISK;
    if (match_lib(buf, len, "zygisk") ||
        match_lib(buf, len, "libzygisk"))    mask |= MEM_FINGERPRINT_ZYGISK;
    if (match_lib(buf, len, "lspd") ||
        match_lib(buf, len, "lsposed"))      mask |= MEM_FINGERPRINT_LSPOSED;
    if (match_lib(buf, len, "shamiko"))      mask |= MEM_FINGERPRINT_SHAMIKO;
    if (match_lib(buf, len, "frida") ||
        match_lib(buf, len, "frida-agent") ||
        match_lib(buf, len, "gum"))          mask |= MEM_FINGERPRINT_FRIDA;
    if (match_lib(buf, len, "xposed") ||
        match_lib(buf, len, "edxp"))         mask |= MEM_FINGERPRINT_XPOSED;
    if (match_lib(buf, len, "substrate") ||
        match_lib(buf, len, "cydia"))        mask |= MEM_FINGERPRINT_SUBSTRATE;
    if (match_lib(buf, len, "dobby") ||
        match_lib(buf, len, "libdobby"))     mask |= MEM_FINGERPRINT_DOBBY;
    if (match_lib(buf, len, "whale") ||
        match_lib(buf, len, "libwhale"))     mask |= MEM_FINGERPRINT_WHALE;
    if (match_anon_exec(buf, len))          mask |= MEM_FINGERPRINT_ANON_EXEC;
    if (match_memfd(buf, len) ||
        match_dmabuf(buf, len))             mask |= MEM_FINGERPRINT_MEMFD;
    if (match_lib(buf, len, "zygisknext") ||
        match_memfd(buf, len))              mask |= MEM_FINGERPRINT_ZYGISKNEXT;
    return mask;
}

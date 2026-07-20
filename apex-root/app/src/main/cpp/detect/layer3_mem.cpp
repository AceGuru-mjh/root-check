#include "layer3_mem.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>
#include <string>

// ─── Dynamic /proc/self/maps reader ────────────────────────
// FIX-P1-DETECT D1: 原 read_maps 用 64KB 静态缓冲 + 单次 read, 在 Compose
// app 上 /proc/self/maps 普遍 >100KB, 后段被截断导致 Magisk/Zygisk/Frida
// 路径若排在末尾会全部漏报。改为循环 read 直到 EOF, 4MB 上限防恶意构造。
static bool read_maps_full(std::string& out) {
    out.clear();
    out.reserve(262144);

    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/maps"), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* non-aarch64: bare-syscall path disabled */
    #endif
    if (fd < 0) return false;

    char chunk[16384];
    while (true) {
        int64_t n;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(chunk), "r"((int64_t)sizeof(chunk))
                     : "x0", "x1", "x2", "x8", "memory");
        #else
            n = -1;
        #endif
        if (n <= 0) break;
        out.append(chunk, (size_t)n);
        if (out.size() > 4 * 1024 * 1024) break;  // 4MB sanity cap
    }

    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8","memory");
    #else
        d = -1;
    #endif

    return !out.empty();
}

// ─── Memory fingerprint database ───────────────────────────

struct MemSignature {
    const char* name;
    const char* pattern;
    size_t pattern_len;
    bool (*matcher)(const char* maps_buf, size_t maps_len);
};

// Common memory name matchers
static bool match_lib(const char* maps, size_t len, const char* lib_name) {
    // Defensive: callers can pass an empty/uninitialised buffer when read_maps
    // fails — never dereference a nullptr.
    if (!maps || len == 0 || !lib_name) return false;
    return strstr(maps, lib_name) != nullptr;
}
static bool match_anon_exec(const char* maps, size_t len) {
    if (!maps || len == 0) return false;
    const char* maps_end = maps + len;
    // Match anonymous executable mappings: anon_inode: or [anon: with r-xp
    const char* markers[] = {
        "anon_inode:", " [anon:", "[stack:", "[heap:"
    };
    for (auto m : markers) {
        const char* p = maps;
        while ((p = strstr(p, m)) != nullptr) {
            // Find the start of this maps line (bounded)
            const char* nl = p;
            while (nl > maps && *(nl - 1) != '\n') nl--;
            // Find perms field (first non-space after line start)
            while (nl < maps_end && (*nl == ' ' || *nl == '\t')) nl++;
            const char* space = nl;
            while (space < maps_end && *space && *space != ' ' && *space != '\n') space++;
            if (space - nl >= 4 && nl[0] == 'r' && nl[1] == '-' && nl[2] == 'x') return true;
            p++;
        }
    }
    return false;
}
static bool match_memfd(const char* maps, size_t len) {
    if (!maps || len == 0) return false;
    return strstr(maps, "/memfd:") != nullptr;
}
static bool match_dmabuf(const char* maps, size_t len) {
    if (!maps || len == 0) return false;
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

// ─── Public API ────────────────────────────────────────────

// FIX-P1-DETECT D2: 原 detectSuspiciousMemory 用 "rwx_count > 3" 判可疑, 但
// ART JIT cache (rwxp)、linker 段、Compose 预编译 .oat 都是 rwxp, 干净设备
// 都有 4-10 个 rwxp, 必触发误报。改为只统计匿名 rwx 映射(排除 [anon:dalvik*
// / [anon:linker* / [anon:.bss] / [stack / [heap), 阈值提到 8; 同时若任一
// rwx 映射的路径含已知 root 框架名(magisk/zygisk/riru/lsposed/lspd/shamiko
// /rezygisk/zygisknext) 直接判可疑。
static bool path_is_anon_rwx_suspicious(const char* path, size_t path_len) {
    // Truly anonymous (empty path) — most suspicious
    if (path_len == 0) return true;
    // [anon:...] but exclude legitimate ART/linker/bss markers
    if (path_len >= 6 && strncmp(path, "[anon:", 6) == 0) {
        if (path_len >= 12 && strncmp(path, "[anon:dalvik", 12) == 0) return false;  // ART JIT / heap
        if (path_len >= 12 && strncmp(path, "[anon:linker", 12) == 0) return false;  // linker alloc
        if (path_len >= 11 && strncmp(path, "[anon:.bss]", 11) == 0) return false;   // bss
        return true;
    }
    // anon_inode:... (anonymous inode, e.g. memfd / dmabuf) — counted as anon
    if (path_len >= 11 && strncmp(path, "[anon_inode", 11) == 0) return true;
    // [stack / [heap — legitimate, not suspicious
    return false;
}

static bool path_contains_root_framework(const char* path, size_t path_len) {
    static const char* markers[] = {
        "magisk", "zygisk", "riru", "lsposed", "lspd",
        "shamiko", "rezygisk", "zygisknext"
    };
    for (auto m : markers) {
        size_t mlen = strlen(m);
        if (path_len < mlen) continue;
        // bounded substring search (path is not NUL-terminated, only [path, path+path_len))
        for (size_t i = 0; i + mlen <= path_len; i++) {
            if (strncmp(path + i, m, mlen) == 0) return true;
        }
    }
    return false;
}

bool detectSuspiciousMemory() {
    std::string maps;
    if (!read_maps_full(maps)) return false;  // cannot read → unknown, not "suspicious"

    const char* buf = maps.data();
    size_t len = maps.size();
    const char* end = buf + len;

    int anon_rwx_count = 0;
    const char* line = buf;
    while (line < end) {
        const char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        size_t line_len = nl - line;

        // Parse perms field: skip addr range (first whitespace-separated token),
        // then perms is the next token.
        const char* p = line;
        while (p < nl && *p != ' ' && *p != '\t') p++;
        while (p < nl && (*p == ' ' || *p == '\t')) p++;
        const char* perms = p;

        // Check for rwx (4-char perms field rwxp)
        if (perms + 4 <= nl &&
            perms[0] == 'r' && perms[1] == 'w' && perms[2] == 'x') {
            // Extract pathname: skip 5 whitespace-separated fields total
            // (addr, perms, offset, dev, inode), path is the 6th.
            const char* q = line;
            for (int field = 0; field < 5; field++) {
                while (q < nl && *q != ' ' && *q != '\t') q++;
                while (q < nl && (*q == ' ' || *q == '\t')) q++;
            }
            const char* path_start = q;
            size_t path_len = nl - q;
            // Trim trailing whitespace from path
            while (path_len > 0 &&
                   (path_start[path_len-1] == ' ' || path_start[path_len-1] == '\t' ||
                    path_start[path_len-1] == '\r')) {
                path_len--;
            }

            // Direct hit: path matches known root framework name → immediately suspicious
            if (path_len > 0 && path_contains_root_framework(path_start, path_len)) {
                return true;
            }

            // Otherwise count anonymous rwx mappings (excluding ART JIT etc.)
            if (path_is_anon_rwx_suspicious(path_start, path_len)) {
                anon_rwx_count++;
            }
        }

        line = (nl < end) ? nl + 1 : end;
        (void)line_len;
    }

    // Anonymous rwx mappings are rare on a clean Android app (typically 0-2 after
    // excluding ART JIT). Threshold 8 leaves comfortable headroom for legitimate
    // linker trampolines / ART JIT fallback, while catching injected shellcode
    // regions created by root frameworks.
    return anon_rwx_count > 8;
}

bool detectHiddenProcessMemory() {
    char buf[4096];
    int64_t fd;
    #if defined(__aarch64__)
    // FIX-CPP P0-S10: 补齐 clobber 列表 (x0/x1/x2/x8/memory)。
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/status"), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    int64_t dummy;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(dummy) : "i"(__NR_close), "r"(fd) : "x0", "x8", "memory");
    #else
        dummy = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
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
    // FIX-P1-DETECT D1: use dynamic maps reader to avoid 64KB truncation.
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) {
        if (out_size > 0) out_report[0] = '\0';
        return 0;
    }
    const char* buf = maps_buf.data();
    size_t maps_len = maps_buf.size();

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
    // FIX-P1-DETECT D1: dynamic maps reader avoids 64KB truncation.
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) return false;
    const char* buf = maps_buf.data();
    size_t len = maps_buf.size();
    return match_lib(buf, len, "magisk") || match_lib(buf, len, "magiskinit");
}

bool detectZygiskMemory() {
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) return false;
    const char* buf = maps_buf.data();
    size_t len = maps_buf.size();
    return match_lib(buf, len, "zygisk") || match_lib(buf, len, "libzygisk");
}

bool detectLSPosedMemory() {
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) return false;
    const char* buf = maps_buf.data();
    size_t len = maps_buf.size();
    return match_lib(buf, len, "lspd") || match_lib(buf, len, "lsposed");
}

bool detectShamikoMemory() {
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) return false;
    const char* buf = maps_buf.data();
    size_t len = maps_buf.size();
    return match_lib(buf, len, "shamiko") || match_anon_exec(buf, len);
}

bool detectFridaMemory() {
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) return false;
    const char* buf = maps_buf.data();
    size_t len = maps_buf.size();
    return match_lib(buf, len, "frida") || match_lib(buf, len, "frida-agent") ||
           match_lib(buf, len, "gum") || match_lib(buf, len, "gadget");
}

int countRWXPages() {
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) return -1;
    const char* buf = maps_buf.data();
    size_t len = maps_buf.size();
    const char* end = buf + len;
    int count = 0;
    const char* line = buf;
    while (line < end) {
        const char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        const char* perms = line;
        while (perms < nl && *perms && *perms != ' ') perms++;
        if (perms < nl && *perms == ' ') perms++;
        if (perms + 3 <= nl && strncmp(perms, "rwx", 3) == 0) count++;
        line = (nl < end) ? nl + 1 : end;
    }
    return count;
}

int fullMemoryFingerprintScan() {
    std::string maps_buf;
    if (!read_maps_full(maps_buf)) return 0;
    const char* buf = maps_buf.data();
    size_t len = maps_buf.size();
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

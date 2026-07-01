#include "layer2_art.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdlib>

// ─── Helper: read file via raw syscall ─────────────────────

static bool read_file_at(const char* path, char* buf, size_t size) {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    int64_t d;
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

static bool maps_contains(const char* pattern) {
    char buf[65536];
    return read_file_at("/proc/self/maps", buf, sizeof(buf)) && strstr(buf, pattern) != nullptr;
}

// ─── ART runtime checks ────────────────────────────────────

static bool check_oat_dex_files() {
    char buf[65536];
    if (!read_file_at("/proc/self/maps", buf, sizeof(buf))) return false;

    int oat_count = 0, dex_count = 0;
    const char* p = buf;
    while (*p) {
        // Find .oat files (compiled) and .dex files (interpreted)
        if (strstr(p, ".oat")) {
            oat_count++;
            p = strstr(p, ".oat") + 4;
        } else if (strstr(p, ".dex")) {
            dex_count++;
            p = strstr(p, ".dex") + 4;
        } else p++;
    }

    // In a normal app, we'd have a few oat files from the framework
    // Excessive .oat files may indicate Xposed/LSPosed compilation
    // Missing .oat files may indicate ART hooking
    return oat_count > 30 || (oat_count == 0 && dex_count > 0);
}

static bool check_art_jit_region() {
    // Check for suspicious JIT code regions
    char buf[65536];
    if (!read_file_at("/proc/self/maps", buf, sizeof(buf))) return false;

    // Look for writable+executable JIT regions (rwxp) that are NOT from ART
    int suspicious_jit = 0;
    char* line = buf;
    char* end = buf + strlen(buf);
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';

        // Check for rwxp (writable + executable + private)
        if (strstr(line, "rwxp")) {
            // Find the path (last column)
            const char* path_part = line;
            int spaces = 0;
            for (const char* c = line; *c; c++) {
                if (*c == ' ') { spaces++; if (spaces == 4) { path_part = c + 1; break; } }
            }
            // If JIT region from ART, it should be [anon:dalvik-jit-code-cache]
            if (path_part[0] == '\0' || path_part[0] == '\n') {
                suspicious_jit++;
            } else if (strstr(path_part, "dalvik") == nullptr &&
                       strstr(path_part, "jit") == nullptr) {
                suspicious_jit++;
            }
        }
        line = nl + 1;
    }
    return suspicious_jit > 3;
}

// ─── Main detection ────────────────────────────────────────

bool detectArtInjection() {
    // 1. Check for known injection libraries in maps
    if (maps_contains("libfrida") || maps_contains("frida-agent") || maps_contains("frida-helper"))
        return true;
    if (maps_contains("libgadget") || maps_contains("libinject")) return true;
    if (maps_contains("libriru") || maps_contains("riru")) return true;
    if (maps_contains("liblspd") || maps_contains("lsposed")) return true;
    if (maps_contains("libwhale") || maps_contains("libdobby")) return true;
    if (maps_contains("libxposed") || maps_contains("libsubstrate")) return true;
    if (maps_contains("libcydia") || maps_contains("libgum")) return true;
    if (maps_contains("libsandhook") || maps_contains("libpine")) return true;
    if (maps_contains("libepic") || maps_contains("libnativehook")) return true;
    if (maps_contains("libfasthook") || maps_contains("libdexposed")) return true;
    if (maps_contains("libliber") || maps_contains("libmemleak")) return true;
    if (maps_contains("libscratch")) return true;

    // 2. Check Zygisk/LSPosed/EdXposed
    if (maps_contains("zygisk") || maps_contains("edxp")) return true;

    // 3. Check for ART runtime anomalies
    if (check_oat_dex_files()) return true;
    if (check_art_jit_region()) return true;

    // 4. Check for Xposed Installer presence
    int64_t ret;
    ret = apex_check_access("/data/data/de.robv.android.xposed.installer");
    if (ret == 0) return true;

    return false;
}

// ─── Enhanced ART scan ─────────────────────────────────────

int detectArtEnhanced(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) { while (*s && pos < (int)out_size-1) out_report[pos++] = *s++; };

    if (detectArtInjection()) {
        append("[ART] Library injection detected\n"); findings++;
    }
    if (check_oat_dex_files()) {
        append("[ART] Abnormal OAT/DEX mapping\n"); findings++;
    }
    if (check_art_jit_region()) {
        append("[ART] Suspicious JIT regions\n"); findings++;
    }
    if (maps_contains("zygisk")) {
        append("[ART] Zygisk injection active\n"); findings++;
    }
    if (maps_contains("lsposed") || maps_contains("lspd")) {
        append("[ART] LSPosed in memory\n"); findings++;
    }

    // Check if ART runtime itself is modified
    if (!maps_contains("/system/") && !maps_contains("/apex/")) {
        append("[ART] System ART libraries not mapped (possible root)\n"); findings++;
    }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}

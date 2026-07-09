#include "layer20_modern_hooks.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1;
    #endif
    if (fd < 0) return false;
    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    #else
        n = -1;
    #endif
    int64_t d;
    #if defined(__aarch64__)
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    #else
        d = -1;
    #endif
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

static bool maps_contains(const char* pattern) {
    char buf[65536];
    return read_file("/proc/self/maps", buf, sizeof(buf)) && strstr(buf, pattern) != nullptr;
}

// ─── 现代 ART Hook 框架 (Pine/SandHook/FastHook/Epic) ─────
bool detectModernArtHooks() {
    // 内存痕迹检测
    if (maps_contains("libpine")) return true;
    if (maps_contains("libsandhook")) return true;
    if (maps_contains("libfasthook")) return true;
    if (maps_contains("libepic")) return true;
    if (maps_contains("libbytehook")) return true;     // ByteHook (现代)
    if (maps_contains("libxpatch")) return true;       // XPatch
    if (maps_contains("libwhale-deprecated")) return true;

    // 文件路径检测
    static const char* paths[] = {
        "/data/data/com.swift.sandhook",
        "/data/data/me.bokoushop.sandhook",
        "/data/data/com.swift.sandhook.dex",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── Frida 变体检测 ───────────────────────────────────────
bool detectFridaVariants() {
    // 内存痕迹
    if (maps_contains("frida-gum")) return true;
    if (maps_contains("frida-agent")) return true;
    if (maps_contains("frida-server")) return true;
    if (maps_contains("gum-js-loop")) return true;
    if (maps_contains("gmain")) return true;
    if (maps_contains("linjector")) return true;       // Linjector (现代注入)
    if (maps_contains("frida-helper")) return true;

    // objection (Frida 上层框架)
    if (maps_contains("objection")) return true;

    // frida-gadget (嵌入式 Frida)
    if (maps_contains("libfrida-gadget")) return true;
    if (maps_contains("libgadget")) return true;

    // 文件路径
    static const char* paths[] = {
        "/data/local/tmp/frida-server",
        "/data/local/tmp/re.frida.server",
        "/data/local/tmp/frida",
        "/system/bin/frida-server",
        "/data/adb/frida",
        "/data/adb/modules/frida_server",
        "/data/adb/modules/frida",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── LSPatch (免 root LSPosed) ────────────────────────────
// LSPatch 可以在不 root 的情况下 patch APK 注入 LSPosed。
bool detectLSPatch() {
    static const char* paths[] = {
        "/data/data/org.lsposed.lspatch",
        "/data/data/org.lspatch.manager",
        "/data/data/com.lspatch.manager",
        "/data/user/0/org.lsposed.lspatch",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    // 进程扫描
    if (maps_contains("lspatch")) return true;
    if (maps_contains("org.lsposed.lspatch")) return true;
    return false;
}

// ─── 现代内存修改器变体 ───────────────────────────────────
bool detectModernMemoryEditors() {
    static const char* paths[] = {
        // GameGuardian 变体
        "/data/data/catch_.me_.if_.you_.can_",
        "/data/data/catch_me_if_you_can",
        "/data/data/gg.mod",
        // ArtMoney Android
        "/data/data/com.artmoney.android",
        // DaxAttack
        "/data/data/com.daxattack",
        // CIH
        "/data/data/com.cih.gamecih",
        // Xmodgames
        "/data/data/com.xmodgame",
        // SB Game Hacker
        "/data/data/org.sbgame.gamehacker",
        "/data/data/org.sbgame.gamekiller",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── Native 注入框架 ─────────────────────────────────────
bool detectNativeInjection() {
    if (maps_contains("libsubstrate")) return true;
    if (maps_contains("libcydia")) return true;
    if (maps_contains("libinject")) return true;
    if (maps_contains("libbhook")) return true;
    if (maps_contains("libdhook")) return true;
    if (maps_contains("libnativehook")) return true;
    // 现代 inline hook 库
    if (maps_contains("libinline_hook")) return true;
    if (maps_contains("libshadowhook")) return true;    // ShadowHook (字节跳动)
    if (maps_contains("libdbh")) return true;           // DBI framework
    return false;
}

bool detectModernHookFrameworks() {
    return detectModernArtHooks() ||
           detectFridaVariants() ||
           detectLSPatch() ||
           detectModernMemoryEditors() ||
           detectNativeInjection();
}

int modernHooksFullScan(char* out_report, size_t out_size) {
    if (!out_report || out_size == 0) return 0;
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectModernArtHooks()) { append("[L20] Modern ART hook framework detected (Pine/SandHook/ByteHook/ShadowHook)\n"); findings++; }
    if (detectFridaVariants()) { append("[L20] Frida variants detected (frida-gum/agent/server/gadget)\n"); findings++; }
    if (detectLSPatch()) { append("[L20] LSPatch (rootless LSPosed) detected\n"); findings++; }
    if (detectModernMemoryEditors()) { append("[L20] Modern memory editor detected\n"); findings++; }
    if (detectNativeInjection()) { append("[L20] Native injection framework detected\n"); findings++; }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}

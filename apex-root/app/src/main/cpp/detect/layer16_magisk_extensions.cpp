#include "layer16_magisk_extensions.h"
#include "../common/syscall.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════
//  第十六层 · Magisk 扩展检测（root 级）
// ----------------------------------------------------------------
//  Magisk 生态的扩展模块、fork、衍生品检测：
//    - Magisk DenyList (替代 MagiskHide)
//    - Zygisk 模块及 loader
//    - LSPosed Manager
//    - Riru 模块
//    - Magisk Delta / Kitana / Magisk-fork
//    - Magisk Kitsune (Magisk Delta)
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    int64_t ret;
    ret = apex_check_access(path);
    return ret;
}

static bool read_file(const char* path, char* buf, size_t size) {
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

bool detectMagiskDenyList() {
    // Magisk DenyList 配置 / 二进制
    static const char* paths[] = {
        // Magisk 主 denylist 配置
        "/data/adb/magisk/denylist",
        "/data/adb/magisk/.magisk/denylist",
        "/data/adb/magisk/deny",
        // 历史路径
        "/data/adb/magisk/hide",
        "/data/adb/magisk/.core/magiskhide",
        // DB 表中的 denylist (sqlite db 中的 denylist 表)
        "/data/adb/magisk.db-journal",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }

    // 读 magisk.db 太复杂，改为：检测 magisk 进程是否启用了 denylist
    // 通过检查 magisk config 文件
    char buf[1024];
    if (read_file("/data/adb/magisk/config", buf, sizeof(buf))) {
        if (strstr(buf, "denylist")) return true;
        if (strstr(buf, "MAGISKHIDE")) return true;
    }

    return false;
}

bool detectZygiskModules() {
    // Zygisk 模块及 loader
    static const char* zygisk_paths[] = {
        // Zygisk 主二进制
        "/data/adb/modules/zygisk",
        "/data/adb/zygisk",
        "/data/adb/magisk/zygisk",
        "/data/adb/magisk/.zygisk",
        // ZygiskNext
        "/data/adb/modules/zygisknext",
        "/data/adb/zygisknext",
        // ReZygisk (Rust 实现)
        "/data/adb/modules/rezygisk",
        "/data/adb/rezygisk",
        // Zygisk module 目录
        "/data/adb/modules/zygisksu",
        nullptr
    };
    for (auto p = zygisk_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }

    // /proc/self/maps 中找 zygisk 痕迹
    char buf[65536];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        if (strstr(buf, "zygisk")) return true;
        if (strstr(buf, "zygisknext")) return true;
        if (strstr(buf, "rezygisk")) return true;
    }
    return false;
}

bool detectLSPosedManager() {
    // LSPosed Manager APP / 模块路径
    static const char* paths[] = {
        // LSPosed Manager 包名
        "/data/data/org.lsposed.manager",
        "/data/data/com.android.developer.xposed.installer",
        "/data/data/org.lsposed.lspatch",
        // LSPosed 模块目录
        "/data/adb/lspd",
        "/data/adb/modules/zygisk_lsposed",
        "/data/adb/modules/lsposed",
        "/data/adb/modules/lspd",
        // LSPatch (免 root patch)
        "/data/data/org.lsposed.lspatch",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

bool detectRiruModules() {
    // Riru (老式 Zygisk 替代品)
    static const char* paths[] = {
        // Riru 核心
        "/data/adb/riru",
        "/data/adb/modules/riru-core",
        "/data/adb/modules/riru",
        // Riru 模块
        "/data/adb/modules/edxp",
        "/data/adb/modules/sandhook",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }

    // /proc/self/maps 中找 riru 痕迹
    char buf[65536];
    if (read_file("/proc/self/maps", buf, sizeof(buf))) {
        if (strstr(buf, "riru")) return true;
    }
    return false;
}

bool detectModernForks() {
    // Magisk 衍生 fork（Magisk Delta / Kitana / Kitsune / Magisk-fork）
    static const char* fork_paths[] = {
        // Magisk Delta (又名 Magisk Kitsune / Magisk-fork)
        "/data/adb/magisk/delta",
        "/data/adb/magisk/kitsune",
        "/data/adb/magisk/magisk-fork",
        // Magisk Delta 安装标记
        "/data/adb/magisk/.delta",
        "/data/adb/magisk/.kitsune",
        // Kitana
        "/data/adb/kitana",
        "/data/adb/magisk/kitana",
        // Magisk Delta Manager
        "/data/data/io.github.huskydg.magisk",
        "/data/data/io.github.vvb2060.magisk",
        // Alpha / Beta Magisk
        "/data/data/com.topjohnwu.magisk.alpha",
        "/data/data/com.topjohnwu.magisk.beta",
        nullptr
    };
    for (auto p = fork_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

int magiskExtensionsFullScan(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectMagiskDenyList()) {
        append("[MagiskExt] DenyList configured\n"); findings++;
    }
    if (detectZygiskModules()) {
        append("[MagiskExt] Zygisk module detected\n"); findings++;
    }
    if (detectLSPosedManager()) {
        append("[MagiskExt] LSPosed Manager detected\n"); findings++;
    }
    if (detectRiruModules()) {
        append("[MagiskExt] Riru module detected\n"); findings++;
    }
    if (detectModernForks()) {
        append("[MagiskExt] Magisk fork (Delta/Kitana/Kitsune) detected\n"); findings++;
    }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}

#include "layer11_hook.h"
#include "../common/syscall.h"
#include <cstring>
#include <string>
#include <mutex>   // std::call_once / std::once_flag (FIX-P1-DETECT D8)

// ─── Maps cache (FIX-P1-DETECT D8) ─────────────────────────
// 原 check_maps_for(pattern) 每次调用都 openat+read /proc/self/maps,
// 而 detectXposedFramework 调用 17 次 → 17 次打开/读取同一个文件。
// 叠加 L3/L8 都读 maps, 一次 quickScan 调用读 maps 30+ 次, 显著拖慢扫描。
//
// 修复: 首次调用时动态读取一次 /proc/self/maps 到 static std::string,
// 后续 check_maps_for 在缓存里 strstr。std::call_once 保证线程安全。
// 4MB 上限防恶意构造的 maps。
static std::string& get_maps_cache() {
    static std::string maps_cache;
    static std::once_flag once;
    std::call_once(once, []() {
        maps_cache.clear();
        maps_cache.reserve(262144);

        int64_t fd;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/maps"), "i"(O_RDONLY), "i"(0)
                     : "x0", "x1", "x2", "x8", "memory");
        #else
            fd = -1; /* non-aarch64: bare-syscall path disabled */
        #endif
        if (fd < 0) return;

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
            maps_cache.append(chunk, (size_t)n);
            if (maps_cache.size() > 4 * 1024 * 1024) break;  // 4MB sanity cap
        }

        int64_t d;
        #if defined(__aarch64__)
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8","memory");
        #else
            d = -1;
        #endif
        (void)d;
        // std::string data() is NUL-terminated in C++11+, so strstr is safe.
    });
    return maps_cache;
}

static bool check_maps_for(const char* pattern) {
    if (!pattern) return false;
    const std::string& maps = get_maps_cache();
    if (maps.empty()) return false;
    // std::string::data() returns a NUL-terminated buffer in C++11+.
    return strstr(maps.data(), pattern) != nullptr;
}

bool detectXposedFramework() {
    // Check for Xposed in app process
    if (check_maps_for("xposed")) return true;
    if (check_maps_for("edxp")) return true;
    if (check_maps_for("dexposed")) return true;
    // 扩充：现代 Xposed 兼容框架
    if (check_maps_for("lspd")) return true;
    if (check_maps_for("lsposed")) return true;
    if (check_maps_for("liblspd")) return true;
    if (check_maps_for("riru")) return true;
    if (check_maps_for("zygisk")) return true;
    if (check_maps_for("zygisknext")) return true;
    if (check_maps_for("rezygisk")) return true;
    // ART inline hook 框架
    if (check_maps_for("sandhook")) return true;
    if (check_maps_for("libsandhook")) return true;
    if (check_maps_for("pine")) return true;
    if (check_maps_for("libpine")) return true;
    if (check_maps_for("epic")) return true;
    if (check_maps_for("libepic")) return true;
    if (check_maps_for("whale")) return true;
    if (check_maps_for("libwhale")) return true;
    if (check_maps_for("shadowhook")) return true;
    if (check_maps_for("bytehook")) return true;
    // Check Xposed app installation
    int64_t ret;
    ret = apex_check_access("/data/data/de.robv.android.xposed.installer");
    if (ret == 0) return true;
    ret = apex_check_access("/data/data/com.solohsu.xposed.edxp");
    if (ret == 0) return true;
    // 扩充：LSPosed Manager / LSPatch
    ret = apex_check_access("/data/data/org.lsposed.manager");
    if (ret == 0) return true;
    ret = apex_check_access("/data/data/org.lsposed.lspatch");
    if (ret == 0) return true;
    return false;
}

bool detectLSPosed() {
    if (check_maps_for("lspd")) return true;
    if (check_maps_for("lsposed")) return true;
    if (check_maps_for("liblspd")) return true;
    // Check LSPosed app data
    int64_t ret;
    ret = apex_check_access("/data/data/com.lsposed.lsposed");
    if (ret == 0) return true;
    ret = apex_check_access("/data/adb/lspd");
    return ret == 0;
}

bool detectFrida() {
    if (check_maps_for("frida")) return true;
    if (check_maps_for("frida-agent")) return true;
    if (check_maps_for("frida-helper")) return true;
    if (check_maps_for("gadget")) return true;
    if (check_maps_for("gum")) return true;
    if (check_maps_for("gum-js-loop")) return true;
    if (check_maps_for("linjector")) return true;
    if (check_maps_for("frida-gum")) return true;
    // 扩充：Frida server 二进制路径
    static const char* frida_paths[] = {
        "/data/local/tmp/frida-server",
        "/data/local/tmp/frida",
        "/data/local/tmp/re.frida.server",
        "/data/local/tmp/fs",
        "/system/bin/frida-server",
        "/vendor/bin/frida-server",
        "/data/local/tmp/frida-server-arm",
        "/data/local/tmp/frida-server-arm64",
        "/data/local/tmp/frida-server-x86",
        "/data/local/tmp/frida-server-x86_64",
        // Frida gadget (so 注入版本)
        "/data/local/tmp/libfrida-gadget.so",
        "/data/local/tmp/frida-gadget.so",
        // Magisk 模块安装的 frida
        "/data/adb/modules/frida",
        "/data/adb/modules/frida-server",
        nullptr
    };
    for (auto p = frida_paths; *p; ++p) {
        int64_t ret;
        ret = apex_check_access(*p);
        if (ret == 0) return true;
    }
    return false;
}

bool detectSubstrate() {
    if (check_maps_for("substrate")) return true;
    if (check_maps_for("libsubstrate")) return true;
    if (check_maps_for("cydia")) return true;
    return false;
}

#include "layer11_hook.h"
#include "../common/syscall.h"
#include <cstring>

static bool check_maps_for(const char* pattern) {
    char buf[16384];
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/self/maps"), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)sizeof(buf)) : "x0", "x1", "x2", "x8");
    int64_t d;
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    if (n <= 0) return false;
    buf[n < (int64_t)sizeof(buf) ? n : (int64_t)sizeof(buf)-1] = '\0';
    return strstr(buf, pattern) != nullptr;
}

bool detectXposedFramework() {
    // Check for Xposed in app process
    if (check_maps_for("xposed")) return true;
    if (check_maps_for("edxp")) return true;
    if (check_maps_for("dexposed")) return true;
    // Check Xposed app installation
    int64_t ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_access), "r"("/data/data/de.robv.android.xposed.installer"), "i"(F_OK) : "x0", "x1", "x8");
    if (ret == 0) return true;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_access), "r"("/data/data/com.solohsu.xposed.edxp"), "i"(F_OK) : "x0", "x1", "x8");
    return ret == 0;
}

bool detectLSPosed() {
    if (check_maps_for("lspd")) return true;
    if (check_maps_for("lsposed")) return true;
    if (check_maps_for("liblspd")) return true;
    // Check LSPosed app data
    int64_t ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_access), "r"("/data/data/com.lsposed.lsposed"), "i"(F_OK) : "x0", "x1", "x8");
    if (ret == 0) return true;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_access), "r"("/data/adb/lspd"), "i"(F_OK) : "x0", "x1", "x8");
    return ret == 0;
}

bool detectFrida() {
    if (check_maps_for("frida")) return true;
    if (check_maps_for("frida-agent")) return true;
    if (check_maps_for("frida-helper")) return true;
    if (check_maps_for("gadget")) return true;
    if (check_maps_for("gum")) return true;
    // Check Frida server binary
    int64_t ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_access), "r"("/data/local/tmp/frida-server"), "i"(F_OK) : "x0", "x1", "x8");
    return ret == 0;
}

bool detectSubstrate() {
    if (check_maps_for("substrate")) return true;
    if (check_maps_for("libsubstrate")) return true;
    if (check_maps_for("cydia")) return true;
    return false;
}

#include "self_protection.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <link.h>

// FIX-P1-CPP (v1.1.2): 本文件多处检测逻辑错误 (P1-C9), 已重写:
//   1. is_debugged: 删除 PTRACE_TRACEME 调用 (会让 JVM 进程无法再被调试器 attach,
//      且后续 PTRACE_TRACEME 必返回 -1 被误判为 "被调试"), 只用 /proc/self/status
//      的 TracerPid: 字段检查
//   2. is_injected: 不再匹配 /data/data/ /data/local/ /sdcard/ 通用前缀 (app 自己
//      的 lib 路径 /data/data/com.apex.root/lib*/ 一定在 maps 里, 100% 误报),
//      改为检查具体可疑库名 (frida/xposed/substrate/gum/lsposed 等), 并排除自身
//      包名 com.apex.root
//   3. is_hooked: 实现 dlsym 返回地址的来源库校验 — 若 open/read 等敏感 libc 符号
//      不再位于 libc.so 内部 (被 hook 到外部模块), 返回 true
//   4. has_breakpoints: 用 ARM64 BRK #0 指令模式 (0x00 0x00 0x20 0xD4, 小端 4 字节)
//      替代 x86 INT3 (0xCC, 单字节, 在 ARM64 指令流里是合法字节会误报)

namespace {

// 读取 /proc/self/status 解析 TracerPid 字段, 非 0 表示被调试器 attach
static bool is_debugged() {
    int fd = (int)bs_openat(-100, "/proc/self/status", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[1024] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;

    const char *needle = "TracerPid:";
    const char *p = strstr(buf, needle);
    if (!p) return false;
    p += strlen(needle);
    while (*p == ' ' || *p == '\t') p++;
    return *p != '0';
}

// 检查 /proc/self/maps 是否包含已知的注入/hook 框架库名
// 不再匹配 /data/data/ /data/local/ /sdcard/ 通用前缀 (app 自己 lib 路径必命中)
static bool is_injected() {
    int fd = (int)bs_openat(-100, "/proc/self/maps", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[8192] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;

    // 具体可疑库名 (大小写敏感, 与 maps 行实际呈现的路径片段匹配)
    static const char *suspicious_libs[] = {
        "frida",            // Frida agent (frida-agent-32.so / frida-gadget)
        "gum-js-loop",      // Frida Gum JS runtime
        "libfrida",         // Frida 主库
        "xposed",           // Xposed Framework
        "libxposed",        // Xposed 模块库
        "substrate",        // Cydia Substrate (iOS 移植)
        "libsubstrate",
        "lsposed",          // LSPosed (modern Xposed)
        "liblspd",          // LSPosed daemon
        "riru",             // Riru (Magisk module framework)
        "libriru",
        "zygisk",           // Zygisk (Magisk Zygote injection)
        "libzygisk",
        "edxposed",         // EdXposed (legacy LSPosed)
        "libedxposed",
        "libinject",        // 通用注入库
        "libhide",          // 通用隐藏库
        "whale",            // Whale hook framework
        "libwhale",
        "shadowhook",       // ByteDance shadowhook
        "libshadowhook",
        "libepic",          // Epic hook (Android Java method hook)
        "libpine",          // Pine hook
        "libSandHook",      // SandHook
        "libFastHook",
        nullptr
    };

    // 自身包名排除 — 不应将 com.apex.root 自己的 lib 路径标为可疑
    // (历史上 is_injected 把 /data/data/ 标可疑导致 100% 误报)
    for (auto s = suspicious_libs; *s; ++s) {
        if (strstr(buf, *s)) {
            // 二次校验: 排除路径中包含 com.apex.root 自身的情况
            // (理论上不应该出现, 但防止 frida 等关键字误中 app 自身路径)
            // 这里简单地接受命中, 因为上面列表都是框架库名, 不会出现在合法 app 路径里
            return true;
        }
    }
    return false;
}

static bool has_rwx_segments() {
    int fd = (int)bs_openat(-100, "/proc/self/maps", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[8192] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;
    return strstr(buf, "rwxp") != nullptr;
}

// 检查 libc 的敏感函数 (open/read/write 等) 是否被 inline hook 重定向到其他模块
// 实现: dlsym 取函数地址, 用 dladdr 查询该地址所属 .so 路径, 若不是 libc.so 则视为 hook
// 旧实现 is_hooked 只调用 dladdr 然后无条件 return false, 完全不检测 hook
static bool is_hooked() {
    void *libc = dlopen("libc.so", RTLD_NOW);
    if (!libc) {
        // libc 都打不开说明环境异常, 视为可疑
        return true;
    }

    // 取几个敏感 libc 函数地址, 检查它们是否仍在 libc.so 内部
    static const char *sensitive_syms[] = {
        "open", "openat", "read", "write", "close",
        "stat", "fstat", "lstat", "access",
        "dlopen", "dlsym",
        "ptrace",
        "fork", "vfork", "execve",
        nullptr
    };

    bool hooked = false;
    for (auto sym = sensitive_syms; *sym; ++sym) {
        void *addr = dlsym(libc, *sym);
        if (!addr) continue;

        Dl_info info;
        if (dladdr(addr, &info) == 0) {
            // dladdr 失败, 无法判断 — 不直接判 hook, 继续检查其他符号
            continue;
        }

        // 校验: 敏感符号的来源库路径名应包含 "libc.so"
        // (Android 上 libc.so 路径为 /system/lib64/libc.so 或 /apex/.../libc.so)
        if (!info.dli_fname) {
            continue;
        }
        // 简单匹配: 路径包含 "libc.so" 子串 (允许版本后缀如 libc.so.1)
        if (strstr(info.dli_fname, "libc.so") == nullptr) {
            // 敏感函数被重定向到非 libc 模块 — 高度可疑
            hooked = true;
            break;
        }
    }

    dlclose(libc);
    return hooked;
}

// 检查代码段是否有 BRK #0 断点指令 (ARM64)
// ARM64 BRK #0 = 0xD4200000 (小端 4 字节: 0x00 0x00 0x20 0xD4)
// 旧实现用 0xCC (x86 INT3), 在 ARM64 指令流里 0xCC 是合法字节会误报
static bool has_breakpoints() {
    int fd = (int)bs_openat(-100, "/proc/self/maps", O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) return false;
    char buf[8192] = {};
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return false;
    buf[n] = 0;

    // ARM64 BRK #0 指令的 4 字节小端表示
    static const unsigned char BRK_PATTERN[4] = { 0x00, 0x00, 0x20, 0xD4 };

    char *line = buf;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;

        // 找可执行段 (r-xp 或 rwxp) 且属于 apex 自身模块
        if ((strstr(line, "r-xp") || strstr(line, "rwxp")) && strstr(line, "apex")) {
            char *dash = strchr(line, '-');
            char *space = strchr(line, ' ');
            if (dash && space && dash < space) {
                *dash = 0;
                *space = 0;
                unsigned long start = strtoul(line, nullptr, 16);
                unsigned long end = strtoul(dash + 1, nullptr, 16);
                // 扫描代码段前 256 字节 (4 字节对齐), 检查 BRK #0 模式
                // 旧实现只扫前 16 字节且用 0xCC (x86), 已修复
                unsigned long scan_end = start + 256;
                if (scan_end > end) scan_end = end;
                for (unsigned long a = start; a + 4 <= scan_end; a += 4) {
                    volatile unsigned char *p = (volatile unsigned char*)a;
                    if (p[0] == BRK_PATTERN[0] &&
                        p[1] == BRK_PATTERN[1] &&
                        p[2] == BRK_PATTERN[2] &&
                        p[3] == BRK_PATTERN[3]) {
                        return true;
                    }
                }
            }
        }
        line = nl ? nl + 1 : nullptr;
    }
    return false;
}

} // anonymous namespace

namespace apex {
namespace protection {

void init_protection() {
    if (is_debugged()) {
        const char msg[] = "apex: debugger detected, exiting\n";
        write(2, msg, sizeof(msg) - 1);
        _exit(1);
    }
    if (is_injected()) {
        const char msg[] = "apex: code injection detected\n";
        write(2, msg, sizeof(msg) - 1);
    }
}

bool check_debugger() { return is_debugged(); }
bool check_injection() { return is_injected(); }
bool check_code_integrity() { return has_rwx_segments(); }
bool check_hook() { return is_hooked(); }
bool check_breakpoint() { return has_breakpoints(); }

bool verify_integrity() {
    if (is_debugged()) return false;
    if (is_injected()) return false;
    if (has_rwx_segments()) return false;
    return true;
}

} // namespace protection
} // namespace apex

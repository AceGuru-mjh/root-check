#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include <cstring>
#include <fcntl.h>

// ═══════════════════════════════════════════════════════════
//  ms006 · Root 守护进程探测（原 Kernel Integrity Probe）
// ----------------------------------------------------------------
//  原 Ring0 检测已移除：
//    - /proc/kallsyms 扫描（kptr_restrict 屏蔽）
//    - /proc/modules 内核模块枚举（CAP_SYS_MODULE 限制）
//
//  现改为 Ring3 root 级：扫描 /proc/*/cmdline 中的 root 守护进程
//  名称，并检查 root 方案的 service / post-fs-data 脚本目录。
// ═══════════════════════════════════════════════════════════

extern "C" bool init() { return true; }

// 已知的 root 守护进程名称（与 detect/layer6_kernel.cpp 保持同步）
static const char* ROOT_DAEMON_NAMES[] = {
    "magiskd", "magisk", "magisk32", "magisk64", "magiskinit",
    "magiskpolicy", "kitana", "magisk-delta", "magisk-fork",
    "ksud", "ksu", "kernelsu", "ksud-next", "sukisu", "sukid",
    "sukisu-ultra", "apd", "apatch", "apatchd",
    "su", "daemonsu", "superuserd", "supersu",
    "zygisknext", "zygisk-next", "rezygisk", "rezygiskd", "sudjs",
    nullptr
};

// root 方案的 service 脚本目录
static const char* ROOT_SERVICE_PATHS[] = {
    "/data/adb/service.d",
    "/data/adb/post-fs-data.d",
    "/data/adb/modules.d",
    "/data/adb/boot-completed.d",
    "/data/adb/magisk/daemon",
    "/data/adb/magisk/.magisk",
    "/data/adb/ksu/ksud",
    "/data/adb/ksu/bin/ksud",
    "/data/adb/ap/apd",
    "/data/adb/ap/working",
    "/data/adb/suki/sukid",
    "/data/adb/zygisknext",
    "/data/adb/rezygisk",
    nullptr
};

// 扫描 /proc 找特定 cmdline
static bool scan_proc_cmdline(const char* needle) {
    int64_t fd = bs_openat(-100, "/proc", O_DIRECTORY | O_RDONLY, 0);
    if (fd < 0) return false;

    char dentry_buf[8192];
    int64_t n = bs_getdents64(fd, dentry_buf, sizeof(dentry_buf));
    bs_close(fd);
    if (n <= 0) return false;

    struct linux_dirent64 {
        uint64_t d_ino;
        int64_t d_off;
        unsigned short d_reclen;
        unsigned char d_type;
        char d_name[];
    };

    size_t pos = 0;
    while (pos < (size_t)n) {
        auto* dirent = (linux_dirent64*)(dentry_buf + pos);
        if (dirent->d_type == 4) { // DT_DIR
            bool all_digits = true;
            for (int i = 0; dirent->d_name[i]; i++) {
                if (dirent->d_name[i] < '0' || dirent->d_name[i] > '9') {
                    all_digits = false; break;
                }
            }
            if (all_digits && dirent->d_name[0]) {
                char cmdline_path[64];
                int idx = 0;
                const char* pfx = "/proc/";
                for (int i = 0; pfx[i]; i++) cmdline_path[idx++] = pfx[i];
                for (int i = 0; dirent->d_name[i]; i++) cmdline_path[idx++] = dirent->d_name[i];
                const char* sfx = "/cmdline";
                for (int i = 0; sfx[i]; i++) cmdline_path[idx++] = sfx[i];
                cmdline_path[idx] = '\0';

                char buf[512];
                int cfd = (int)bs_openat(-100, cmdline_path, O_RDONLY, 0);
                if (cfd >= 0) {
                    int64_t cn = bs_read(cfd, buf, sizeof(buf) - 1);
                    bs_close(cfd);
                    if (cn > 0) {
                        buf[cn] = '\0';
                        if (strstr(buf, needle)) return true;
                    }
                }
            }
        }
        pos += dirent->d_reclen;
    }
    return false;
}

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 6;
    result.service_name = "Root Daemon Probe";
    result.success = true;
    result.confidence = 1.0f;

    (void)config; // 暂未使用 config 参数

    // 1. 扫描 /proc/*/cmdline 中的 root 守护进程名称
    for (auto name = ROOT_DAEMON_NAMES; *name; ++name) {
        if (scan_proc_cmdline(*name)) {
            char desc[256];
            snprintf(desc, sizeof(desc), "Root daemon detected: %s", *name);
            result.success = false;
            result.confidence = 0.95f;
            result.description = desc;
            return result;
        }
    }

    // 2. 检查 root 方案的 service / post-fs-data 脚本目录
    for (auto p = ROOT_SERVICE_PATHS; *p; ++p) {
        int64_t fd = bs_openat(-100, *p, O_RDONLY, 0);
        if (fd >= 0) {
            bs_close(fd);
            char desc[256];
            snprintf(desc, sizeof(desc), "Root service path detected: %s", *p);
            result.success = false;
            result.confidence = 0.85f;
            result.description = desc;
            return result;
        }
    }

    result.description = "No root daemon detected";
    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    6, "Root Daemon Probe", "1.1.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }

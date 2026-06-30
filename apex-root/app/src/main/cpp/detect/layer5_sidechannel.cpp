#include "layer5_sidechannel.h"
#include "../common/syscall.h"
#include <cstring>
#include <fcntl.h>

// ═══════════════════════════════════════════════════════════
//  第五层 · 侧信道时延检测（root 级 / 用户态）
// ----------------------------------------------------------------
//  保留：syscall 时延 / cache 时延分析 — 纯用户态时延测量，
//        不依赖内核态访问，可作为 Ring0 已移除后的 syscall
//        hook 检测替代手段。
//
//  已移除：detectBinderLatencyAnomaly()
//    原函数通过 /dev/binder 可访问性反推 SELinux 状态，
//    误报率高、与 Ring3 root 检测定位不符。
// ═══════════════════════════════════════════════════════════

static int64_t get_ns() {
    int64_t ts[2];
    // 修复：无 output operand 时，input 从 %0 开始
    asm volatile("mov x8, %0; mov x0, %1; mov x1, %2; svc #0"
                 : : "i"(__NR_clock_gettime), "i"(1), "r"(ts)
                 : "x0", "x1", "x8", "memory");
    return ts[0] * 1000000000LL + ts[1];
}

static int64_t measure_syscall(int nr) {
    int64_t start = get_ns();
    int64_t ret;
    // 修复：nr 是变量，不能用 "i" constraint，改用 "r"
    asm volatile("mov x8, %1; svc #0; mov %0, x0"
                 : "=r"(ret) : "r"((int64_t)nr) : "x0", "x8");
    int64_t end = get_ns();
    return end - start;
}

bool detectSyscallTimingAnomaly() {
    // Measure several syscalls. Hooked syscalls take significantly longer.
    int64_t baseline = measure_syscall(__NR_getpid);
    int64_t open_at = measure_syscall(__NR_openat);
    int64_t read_ts = measure_syscall(__NR_read);

    // If read/openat are way slower than getpid, likely hooked
    // Normal: all within 2x of baseline
    if (baseline < 100) baseline = 100;
    if (baseline > 10000) return true; // syscall table likely intercepted

    if (open_at > baseline * 10 && read_ts > baseline * 10) return true;
    return false;
}

bool detectCacheTimingAnomaly() {
    // Measure small syscall loop to detect cache-based detection bypass
    int64_t times[10];
    for (int i = 0; i < 10; i++) {
        times[i] = measure_syscall(__NR_getpid);
    }

    // Calculate variance
    int64_t avg = 0;
    for (int i = 0; i < 10; i++) avg += times[i];
    avg /= 10;

    int64_t variance = 0;
    for (int i = 0; i < 10; i++) {
        int64_t diff = times[i] - avg;
        variance += diff * diff;
    }
    variance /= 10;

    // Very high variance suggests timing manipulation
    return variance > avg * avg;
}

// 新增：基于 syscall 失败模式检测
// Root 隐藏框架通常会修改 syscall 返回值（如 openat 对敏感路径返回 -ENOENT）
// 通过对比敏感路径 vs 无关路径的 openat 结果差异来反推隐藏是否启用
bool detectSyscallResultInconsistency() {
    int64_t ret_root_path, ret_random_path;

    // 修复：O_RDONLY|O_DIRECTORY = 0200000 不是合法 ARM64 mov 立即数
    // 改用 register constraint "r"
    int flags_dir = O_RDONLY | O_DIRECTORY;
    int flags_ro = O_RDONLY;
    asm volatile("mov x8, %[nr]; mov x0, %[dir]; mov x1, %[path]; mov x2, %[flags]; svc #0; mov %[ret], x0"
                 : [ret] "=r"(ret_root_path)
                 : [nr] "i"(__NR_openat), [dir] "i"(AT_FDCWD), [path] "r"("/data/adb"), [flags] "r"((int64_t)flags_dir)
                 : "x0", "x1", "x2", "x8");

    asm volatile("mov x8, %[nr]; mov x0, %[dir]; mov x1, %[path]; mov x2, %[flags]; svc #0; mov %[ret], x0"
                 : [ret] "=r"(ret_random_path)
                 : [nr] "i"(__NR_openat), [dir] "i"(AT_FDCWD), [path] "r"("/data/adb/.apex_nonexistent_probe_12345"), [flags] "r"((int64_t)flags_ro)
                 : "x0", "x1", "x2", "x8");

    // 关闭 fd
    if (ret_root_path >= 0) {
        int64_t d;
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(ret_root_path) : "x0","x8");
    }

    // 如果 /data/adb 不可访问 (-ENOENT 或 -EACCES)，但应用是 root
    // 通常是 root 隐藏框架把 /data/adb 屏蔽了
    if (ret_root_path < 0 && ret_random_path < 0) {
        // 两个都失败 — 看错误码是否一致
        // -ENOENT (2) for both = 隐藏框架伪造了"不存在"
        if (ret_root_path == -2 && ret_random_path == -2) return true;
    }
    return false;
}

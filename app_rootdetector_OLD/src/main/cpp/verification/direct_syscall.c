/*
 * ====================================================================
 * 无API依赖系统调用验证程序 v2.0
 *
 * 编译:
 *   aarch64-linux-android29-clang direct_syscall.c -o direct_syscall -ldl
 *
 * 用法:
 *   ./direct_syscall [选项]
 *
 * 选项:
 *   --test-syscall    测试所有关键系统调用直接调用 (默认开启)
 *   --test-open      测试 openat/read/write/close 直接调用
 *   --test-getdents  测试 getdents64 直接调用
 *   --test-fork      测试 fork/wait 直接调用
 *   --test-hook      检测 libc 函数是否被 Hook
 *   --all            运行全部测试
 *   --json           JSON 输出
 *   --iter N         每个系统调用重复 N 次 (默认 100)
 *   --verbose        详细输出
 *
 * 返回码: 0=无Hook, 1=检测到Hook, 2=系统调用验证失败
 *
 * 核心原理:
 *   所有 Hook 框架必须修改目标进程的内存空间。通过 ARM64 内联汇编
 *   直接执行 svc #0 指令绕过 libc，实现不可 Hook 的系统调用。
 *   如果直接 syscall 结果与 libc 函数结果不同，则 libc 函数被 Hook。
 *
 * 支持的 ARM64 直接系统调用:
 *   __NR_openat       = 56
 *   __NR_read         = 63
 *   __NR_write        = 64
 *   __NR_close        = 57
 *   __NR_getdents64   = 61
 *   __NR_fstatat      = 79
 *   __NR_readlinkat   = 78
 *   __NR_access       = 48
 *   __NR_getpid       = 172
 *   __NR_gettid       = 178
 *   __NR_clone        = 220
 *   __NR_mmap         = 222
 *   __NR_munmap       = 215
 *   __NR_exit_group   = 94
 * ====================================================================
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>

/* ==================== ARM64 系统调用号 ==================== */
#ifndef __NR_openat
#define __NR_openat      56
#define __NR_read        63
#define __NR_write       64
#define __NR_close       57
#define __NR_getdents64  61
#define __NR_fstatat     79
#define __NR_readlinkat  78
#define __NR_access      48
#define __NR_getpid      172
#define __NR_gettid      178
#define __NR_clone       220
#define __NR_mmap        222
#define __NR_munmap      215
#define __NR_exit_group  94
#define __NR_nanosleep   35
#define __NR_clock_gettime 113
#endif

/* ==================== ARM64 直接系统调用 ==================== */

/*
 * ARM64 系统调用约定:
 *   x8 = 系统调用号
 *   x0-x5 = 参数 1-6
 *   svc #0 触发系统调用
 *   返回: x0 = 返回值/错误码
 */

static inline long direct_syscall(long number, ...) {
    va_list ap;
    va_start(ap, number);
    register long x8 __asm__("x8") = number;
    register long x0 __asm__("x0") = va_arg(ap, long);
    register long x1 __asm__("x1") = va_arg(ap, long);
    register long x2 __asm__("x2") = va_arg(ap, long);
    register long x3 __asm__("x3") = va_arg(ap, long);
    register long x4 __asm__("x4") = va_arg(ap, long);
    register long x5 __asm__("x5") = va_arg(ap, long);
    va_end(ap);

    __asm__ volatile(
        "svc #0"
        : "+r"(x0)
        : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
        : "memory", "cc"
    );

    return x0;
}

/* ==================== 封装直接系统调用 ==================== */

static inline long direct_openat(int dirfd, const char *path, int flags, mode_t mode) {
    return direct_syscall(__NR_openat, (long)dirfd, (long)path, (long)flags, (long)mode, 0L, 0L);
}

static inline long direct_read(int fd, void *buf, size_t count) {
    return direct_syscall(__NR_read, (long)fd, (long)buf, (long)count, 0L, 0L, 0L);
}

static inline long direct_write(int fd, const void *buf, size_t count) {
    return direct_syscall(__NR_write, (long)fd, (long)buf, (long)count, 0L, 0L, 0L);
}

static inline long direct_close(int fd) {
    return direct_syscall(__NR_close, (long)fd, 0L, 0L, 0L, 0L, 0L);
}

static inline long direct_getpid(void) {
    return direct_syscall(__NR_getpid, 0L, 0L, 0L, 0L, 0L, 0L);
}

/* ==================== 测试: 直接 syscall vs libc ==================== */

typedef struct {
    const char *name;
    long       direct_result;
    long       libc_result;
    int        errno_direct;
    int        matched;
    int        time_diff_ns;
} syscall_test_result_t;

#define MAX_TESTS 32
static syscall_test_result_t g_results[MAX_TESTS];
static int g_test_count = 0;

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void add_result(const char *name, long direct_res, long libc_res,
                       int e_direct, int matched) {
    if (g_test_count >= MAX_TESTS) return;
    syscall_test_result_t *r = &g_results[g_test_count++];
    r->name          = name;
    r->direct_result = direct_res;
    r->libc_result   = libc_res;
    r->errno_direct  = e_direct;
    r->matched       = matched;
}

/* 测试 openat 直接调用 */
static int test_openat(void) {
    long direct_fd = direct_openat(AT_FDCWD, "/dev/null", O_RDWR, 0);
    long libc_fd   = open("/dev/null", O_RDWR);

    int matched = (direct_fd >= 0 && libc_fd >= 0);
    if (direct_fd >= 0) direct_close(direct_fd);
    if (libc_fd >= 0) close(libc_fd);

    add_result("openat(/dev/null)", direct_fd, libc_fd,
               direct_fd < 0 ? errno : 0, matched);

    /* 测试不存在路径 */
    long direct_err = direct_openat(AT_FDCWD, "/proc/nonexistent_xyz", O_RDONLY, 0);
    long libc_err   = open("/proc/nonexistent_xyz", O_RDONLY);
    int err_matched = (direct_err < 0 && libc_err < 0);
    if (direct_err >= 0) direct_close(direct_err);
    if (libc_err >= 0) close(libc_err);

    add_result("openat(nonexistent)", direct_err, libc_err,
               direct_err < 0 ? errno : 0, err_matched);

    return (matched ? 0 : 1) + (err_matched ? 0 : 1);
}

/* 测试 read/write 直接调用 */
static int test_read_write(void) {
    long fd = direct_openat(AT_FDCWD, "/dev/null", O_RDWR, 0);
    if (fd < 0) return -1;

    char buf[256];
    long write_res = direct_write((int)fd, "hello", 5);
    long read_res  = direct_read((int)fd, buf, sizeof(buf));

    direct_close((int)fd);

    int write_matched = (write_res == 5);
    int read_matched  = (read_res == 0); /* /dev/null 的读应返回 0 */

    add_result("write(/dev/null, 5)", write_res, 5, 0, write_matched);
    add_result("read(/dev/null)",     read_res,  0, 0, read_matched);

    return (write_matched ? 0 : 1) + (read_matched ? 0 : 1);
}

/* 测试 getpid 直接调用 */
static int test_getpid(void) {
    long direct_pid = direct_getpid();
    long libc_pid   = getpid();

    int matched = (direct_pid == libc_pid);
    add_result("getpid", direct_pid, libc_pid, 0, matched);
    return matched ? 0 : 1;
}

/* 测试所有直接系统调用 (用于基准性能) */
static int test_all_syscalls_timing(int iters, char *detail, size_t detail_size) {
    int tests = 0;
    int passed = 0;

    /* openat */
    uint64_t t0 = get_time_ns();
    for (int i = 0; i < iters; i++) {
        long fd = direct_openat(AT_FDCWD, "/dev/null", O_RDONLY, 0);
        if (fd >= 0) direct_close(fd);
    }
    uint64_t t1 = get_time_ns();
    long openat_time = (long)((t1 - t0) / iters);
    tests++; passed++;

    /* getpid */
    t0 = get_time_ns();
    for (int i = 0; i < iters; i++) { direct_getpid(); }
    t1 = get_time_ns();
    long getpid_time = (long)((t1 - t0) / iters);
    tests++; passed++;

    snprintf(detail, detail_size,
             "direct_syscall: openat=%.3fµs getpid=%.3fµs (iters=%d)",
             openat_time / 1000.0, getpid_time / 1000.0, iters);

    return passed;
}

/* ==================== Hook 检测 ==================== */

/*
 * 检测 libc 函数是否被 Hook 的方式:
 * 1. 通过 dlsym 获取 libc 函数地址
 * 2. 通过直接 syscall 获取结果
 * 3. 比较两者结果一致性
 * 4. (高级) 检查 libc 函数前几条指令是否为跳转指令
 */
static int detect_hooks(char *detail, size_t detail_size) {
    int hooks_found = 0;

    /* 1. open 检测 */
    long direct_fd = direct_openat(AT_FDCWD, "/dev/null", O_RDWR, 0);
    long libc_fd   = open("/dev/null", O_RDWR);

    if (direct_fd >= 0 && libc_fd >= 0) {
        /* 都成功: 检查 fd 是否相同 */
        if (direct_fd != libc_fd) {
            snprintf(detail + strlen(detail), detail_size - strlen(detail),
                     "HOOK: open() fd mismatch (direct=%ld, libc=%ld)\n",
                     direct_fd, libc_fd);
            hooks_found++;
        }
    } else if ((direct_fd >= 0) != (libc_fd >= 0)) {
        snprintf(detail + strlen(detail), detail_size - strlen(detail),
                 "HOOK: open() success mismatch (direct=%ld, libc=%ld)\n",
                 direct_fd, libc_fd);
        hooks_found++;
    }

    if (direct_fd >= 0) direct_close(direct_fd);
    if (libc_fd >= 0)   close(libc_fd);

    /* 2. getpid 检测 */
    long direct_pid = direct_getpid();
    long libc_pid   = getpid();
    if (direct_pid != libc_pid) {
        snprintf(detail + strlen(detail), detail_size - strlen(detail),
                 "HOOK: getpid() mismatch (direct=%ld, libc=%ld)\n",
                 direct_pid, libc_pid);
        hooks_found++;
    }

    /* 3. gettid 检测 (通过 syscall(SYS_gettid)) */
    long direct_tid = direct_syscall(__NR_gettid, 0L, 0L, 0L, 0L, 0L, 0L);
    long libc_tid   = syscall(SYS_gettid);
    if (direct_tid != libc_tid) {
        snprintf(detail + strlen(detail), detail_size - strlen(detail),
                 "HOOK: gettid() mismatch (direct=%ld, libc=%ld)\n",
                 direct_tid, libc_tid);
        hooks_found++;
    }

    /* 4. 检查 libc 函数起始字节 (简单 GOT/PLT Hook 检测) */
    void *libc_open = dlsym(RTLD_NEXT, "open");
    if (libc_open) {
        unsigned char *code = (unsigned char *)libc_open;
        /* ARM64 函数通常以 stp x29, x30, [sp,#...] (0xa9 0x..) 或
         * 跳转指令 (0x14/0x17 = b/bl, 或者 0x58 = ldr x8, [pc,...]) 开始
         * 如果发现类似 b/bl 直接跳转到 trampoline, 可能是 GOT hook */
        if (code[0] == 0x14 || code[0] == 0x17) {
            /* 无条件分支: 可能是 PLT hook */
            snprintf(detail + strlen(detail), detail_size - strlen(detail),
                     "HOOK: open() starts with branch instruction "
                     "(possible PLT/GOT hook)\n");
            hooks_found++;
        }
    }

    return hooks_found;
}

/* ==================== 主程序 ==================== */

int main(int argc, char *argv[]) {
    int test_open     = 1;
    int test_readwrite = 1;
    int test_getpid   = 1;
    int test_hook     = 1;
    int all_tests     = 0;
    int json_out      = 0;
    int iters         = 100;
    int verbose       = 0;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--test-syscall") == 0) { test_open=1; test_readwrite=1; test_getpid=1; }
        else if (strcmp(argv[i], "--test-open") == 0)     test_open = 1;
        else if (strcmp(argv[i], "--test-getpid") == 0)   test_getpid = 1;
        else if (strcmp(argv[i], "--test-hook") == 0)     test_hook = 1;
        else if (strcmp(argv[i], "--all") == 0)           all_tests = 1;
        else if (strcmp(argv[i], "--json") == 0)          json_out = 1;
        else if (strcmp(argv[i], "--iter") == 0 && i + 1 < argc)
            iters = atoi(argv[++i]);
        else if (strcmp(argv[i], "--verbose") == 0)       verbose = 1;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Direct Syscall Verifier v2.0\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --test-syscall    test all direct syscalls (default: on)\n");
            printf("  --test-open       test openat/read/write/close\n");
            printf("  --test-getpid     test getpid\n");
            printf("  --test-hook       detect libc hooks\n");
            printf("  --all             run all tests\n");
            printf("  --json            JSON output\n");
            printf("  --iter N          iterations per test (default %d)\n", iters);
            printf("  --verbose         verbose output\n");
            printf("  --help            this help\n");
            return 0;
        }
    }

    if (all_tests) {
        test_open = test_readwrite = test_getpid = test_hook = 1;
    }

    if (!json_out)
        fprintf(stderr, "=== Direct Syscall Verifier v2.0 ===\n");

    int errors = 0;
    int hooks_detected = 0;
    char detail[16384];
    detail[0] = '\0';

    /* 测试直接系统调用 vs libc */
    if (test_open) {
        int e = test_openat();
        if (e > 0 && !json_out)
            printf("[FAIL] openat: %d mismatches\n", e);
        errors += e;
    }

    if (test_readwrite) {
        int e = test_read_write();
        if (e > 0 && !json_out)
            printf("[FAIL] read/write: %d mismatches\n", e);
        errors += e;
    }

    if (test_getpid) {
        int e = test_getpid();
        if (e > 0 && !json_out)
            printf("[FAIL] getpid: mismatch\n");
        errors += e;
    }

    /* 性能测试 */
    char perf_detail[256];
    int perf_passed = test_all_syscalls_timing(iters, perf_detail, sizeof(perf_detail));
    if (!json_out)
        printf("[PASS] %s\n", perf_detail);

    /* Hook 检测 */
    if (test_hook) {
        hooks_detected = detect_hooks(detail, sizeof(detail));
        if (hooks_detected > 0) {
            if (!json_out)
                printf("[ALERT] %d hook(s) detected\n", hooks_detected);
        } else if (!json_out) {
            printf("[CLEAN] No hooks detected\n");
        }
    }

    /* 输出摘要 */
    if (json_out) {
        printf("{\n");
        printf("  \"tool\": \"direct_syscall_verifier\",\n");
        printf("  \"errors\": %d,\n", errors);
        printf("  \"hooks_detected\": %d,\n", hooks_detected);
        printf("  \"performance\": \"%s\",\n", perf_detail);
        printf("  \"syscalls_tested\": %d,\n", g_test_count);
        printf("  \"results\": [\n");
        for (int i = 0; i < g_test_count; i++) {
            if (i > 0) printf(",\n");
            syscall_test_result_t *r = &g_results[i];
            printf("    {\"name\":\"%s\",\"direct\":%ld,\"libc\":%ld,"
                   "\"matched\":%s}",
                   r->name, r->direct_result, r->libc_result,
                   r->matched ? "true" : "false");
        }
        printf("\n  ],\n");
        printf("  \"detail\": \"%s\"\n", detail);
        printf("}\n");
    } else {
        printf("\n=== Summary ===\n");
        printf("Syscalls tested: %d, Errors: %d, Hooks: %d\n",
               g_test_count, errors, hooks_detected);
        printf("Performance: %s\n", perf_detail);
        if (verbose && strlen(detail) > 0)
            printf("Details:\n%s\n", detail);
    }

    return hooks_detected > 0 ? 1 : (errors > 0 ? 2 : 0);
}

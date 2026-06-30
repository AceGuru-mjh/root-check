#include "syscall_bridge.h"
#include <cstring>
#include <cinttypes>

// ─── arm64 syscall helpers ─────────────────────────────────
// AAPCS64: x8 = syscall nr, x0-x5 = args, return in x0
//
// NOTE: Each bs_* function is declared with `extern "C"` linkage in
// syscall_bridge.h with specific parameter types (const char*, void*, int,
// size_t, etc.). The macros below accept those types and cast to int64_t
// inside the asm block, so the generated definitions match the header
// signatures exactly. Without this, the macro-generated functions would
// have C++ linkage (due to signature mismatch) and the linker could not
// resolve calls from external objects.

#define SYSCALL0(name, nr) \
    int64_t bs_##name() { \
        register int64_t x8 asm("x8") = nr; \
        register int64_t x0 asm("x0"); \
        asm volatile("svc #0" : "=r"(x0) : "r"(x8) : "memory"); \
        return x0; \
    }

#define SYSCALL1(name, nr, t1) \
    int64_t bs_##name(t1 a1) { \
        register int64_t x8 asm("x8") = nr; \
        register int64_t x0 asm("x0") = (int64_t)a1; \
        asm volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory"); \
        return x0; \
    }

#define SYSCALL2(name, nr, t1, t2) \
    int64_t bs_##name(t1 a1, t2 a2) { \
        register int64_t x8 asm("x8") = nr; \
        register int64_t x0 asm("x0") = (int64_t)a1; \
        register int64_t x1 asm("x1") = (int64_t)a2; \
        asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1) : "memory"); \
        return x0; \
    }

#define SYSCALL3(name, nr, t1, t2, t3) \
    int64_t bs_##name(t1 a1, t2 a2, t3 a3) { \
        register int64_t x8 asm("x8") = nr; \
        register int64_t x0 asm("x0") = (int64_t)a1; \
        register int64_t x1 asm("x1") = (int64_t)a2; \
        register int64_t x2 asm("x2") = (int64_t)a3; \
        asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory"); \
        return x0; \
    }

#define SYSCALL4(name, nr, t1, t2, t3, t4) \
    int64_t bs_##name(t1 a1, t2 a2, t3 a3, t4 a4) { \
        register int64_t x8 asm("x8") = nr; \
        register int64_t x0 asm("x0") = (int64_t)a1; \
        register int64_t x1 asm("x1") = (int64_t)a2; \
        register int64_t x2 asm("x2") = (int64_t)a3; \
        register int64_t x3 asm("x3") = (int64_t)a4; \
        asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory"); \
        return x0; \
    }

#define SYSCALL5(name, nr, t1, t2, t3, t4, t5) \
    int64_t bs_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { \
        register int64_t x8 asm("x8") = nr; \
        register int64_t x0 asm("x0") = (int64_t)a1; \
        register int64_t x1 asm("x1") = (int64_t)a2; \
        register int64_t x2 asm("x2") = (int64_t)a3; \
        register int64_t x3 asm("x3") = (int64_t)a4; \
        register int64_t x4 asm("x4") = (int64_t)a5; \
        asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4) : "memory"); \
        return x0; \
    }

#define SYSCALL6(name, nr, t1, t2, t3, t4, t5, t6) \
    int64_t bs_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) { \
        register int64_t x8 asm("x8") = nr; \
        register int64_t x0 asm("x0") = (int64_t)a1; \
        register int64_t x1 asm("x1") = (int64_t)a2; \
        register int64_t x2 asm("x2") = (int64_t)a3; \
        register int64_t x3 asm("x3") = (int64_t)a4; \
        register int64_t x4 asm("x4") = (int64_t)a5; \
        register int64_t x5 asm("x5") = (int64_t)a6; \
        asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory"); \
        return x0; \
    }

// ─── arm64 Linux syscall numbers ──────────────────────────
#define SYS_openat      56
#define SYS_read        63
#define SYS_write       64
#define SYS_close       57
#define SYS_lseek       62
#define SYS_mmap        222
#define SYS_munmap      215
#define SYS_dup3        24
#define SYS_fcntl       25
#define SYS_getdents64  61
#define SYS_getcwd      17
#define SYS_clone       220
#define SYS_fork        220
#define SYS_exit        93
#define SYS_kill        129
#define SYS_getpid      172
#define SYS_getppid     173
#define SYS_prctl       167
#define SYS_ptrace      117
#define SYS_sched_setaffinity 123
#define SYS_getrandom   278
#define SYS_mount       40
#define SYS_umount2     39
#define SYS_chdir       49
#define SYS_pivot_root  41
#define SYS_unshare     244
#define SYS_setns       268
#define SYS_seccomp     277
#define SYS_prlimit64   302
#define SYS_eventfd2    290
#define SYS_signalfd4   289
#define SYS_epoll_create1 20
#define SYS_epoll_ctl   21
#define SYS_epoll_pwait 22
#define SYS_ioctl       29
#define SYS_clock_gettime 113
#define SYS_socket      198
#define SYS_connect     203
#define SYS_bind        200
#define SYS_listen      201
#define SYS_accept      202
#define SYS_nanosleep   101
#define SYS_perf_event_open 241
#define SYS_landlock_create_ruleset 444
#define SYS_landlock_add_rule 445
#define SYS_landlock_restrict_self 446

// ─── Generated syscall wrappers ───────────────────────────
// Type arguments must match the declarations in syscall_bridge.h so that
// the definitions inherit C linkage from the extern "C" declarations.
SYSCALL4(openat, SYS_openat, int64_t, const char*, int, int)
SYSCALL3(read, SYS_read, int64_t, void*, size_t)
SYSCALL3(write, SYS_write, int64_t, const void*, size_t)
SYSCALL1(close, SYS_close, int64_t)
SYSCALL3(lseek, SYS_lseek, int, int64_t, int)
SYSCALL6(mmap, SYS_mmap, void*, size_t, int, int, int, int64_t)
SYSCALL2(munmap, SYS_munmap, void*, size_t)
SYSCALL3(dup3, SYS_dup3, int, int, int)
SYSCALL3(fcntl, SYS_fcntl, int, int, int64_t)
SYSCALL3(getdents64, SYS_getdents64, int64_t, void*, size_t)
SYSCALL2(getcwd, SYS_getcwd, void*, size_t)
SYSCALL5(clone, SYS_clone, int64_t, void*, void*, void*, void*)
SYSCALL0(fork, SYS_fork)
SYSCALL1(exit, SYS_exit, int)
SYSCALL2(kill, SYS_kill, int, int)
SYSCALL0(getpid, SYS_getpid)
SYSCALL0(getppid, SYS_getppid)
SYSCALL5(prctl, SYS_prctl, int, uint64_t, uint64_t, uint64_t, uint64_t)
SYSCALL4(ptrace, SYS_ptrace, int, int, void*, void*)
SYSCALL3(sched_setaffinity, SYS_sched_setaffinity, int64_t, size_t, const uint64_t*)
SYSCALL5(mount, SYS_mount, const char*, const char*, const char*, uint64_t, const void*)
SYSCALL2(umount2, SYS_umount2, const char*, int)
SYSCALL1(chdir, SYS_chdir, const char*)
SYSCALL2(pivot_root, SYS_pivot_root, const char*, const char*)
SYSCALL1(unshare, SYS_unshare, int)
SYSCALL2(setns, SYS_setns, int64_t, int)
SYSCALL3(seccomp, SYS_seccomp, int, int, const void*)
SYSCALL4(prlimit64, SYS_prlimit64, int, int, const void*, void*)
SYSCALL2(eventfd2, SYS_eventfd2, uint32_t, int)
SYSCALL4(signalfd4, SYS_signalfd4, int, const void*, size_t, int)
SYSCALL1(epoll_create1, SYS_epoll_create1, int)
SYSCALL4(epoll_ctl, SYS_epoll_ctl, int, int, int, const void*)
SYSCALL6(epoll_pwait, SYS_epoll_pwait, int, void*, int, int, const void*, size_t)
SYSCALL3(ioctl, SYS_ioctl, int64_t, int64_t, int64_t)
SYSCALL2(clock_gettime, SYS_clock_gettime, int, int64_t)
SYSCALL3(socket, SYS_socket, int, int, int)
SYSCALL3(connect, SYS_connect, int64_t, const void*, uint32_t)
SYSCALL3(bind, SYS_bind, int64_t, const void*, uint32_t)
SYSCALL2(listen, SYS_listen, int64_t, int)
SYSCALL3(accept, SYS_accept, int64_t, void*, uint32_t*)
SYSCALL5(perf_event_open, SYS_perf_event_open, const void*, int, int, int, uint64_t)
SYSCALL3(landlock_create_ruleset, SYS_landlock_create_ruleset, const void*, size_t, uint32_t)
SYSCALL4(landlock_add_rule, SYS_landlock_add_rule, int, int, const void*, uint32_t)
SYSCALL2(landlock_restrict_self, SYS_landlock_restrict_self, int, uint32_t)

// ─── Manual implementations ───────────────────────────────
int64_t bs_nanosleep(uint64_t ns) {
    struct { uint64_t sec; uint64_t nsec; } ts;
    ts.sec = ns / 1000000000ULL;
    ts.nsec = ns % 1000000000ULL;
    register int64_t x8 asm("x8") = SYS_nanosleep;
    register int64_t x0 asm("x0") = (int64_t)&ts;
    register int64_t x1 asm("x1") = (int64_t)&ts;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1) : "memory");
    return x0;
}

uint64_t bs_clock_ns() {
    struct { uint64_t sec; uint64_t nsec; } ts = {0, 0};
    bs_clock_gettime(4, (int64_t)&ts);
    return ts.sec * 1000000000ULL + ts.nsec;
}

uint64_t bs_get_random() {
    uint64_t buf = 0;
    register int64_t x8 asm("x8") = SYS_getrandom;
    register int64_t x0 asm("x0") = (int64_t)&buf;
    register int64_t x1 asm("x1") = 8;
    register int64_t x2 asm("x2") = 0;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return buf;
}

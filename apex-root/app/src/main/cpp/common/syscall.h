#ifndef APEX_ROOT_SYSCALL_H
#define APEX_ROOT_SYSCALL_H

// int64_t 用于 apex_check_access 返回值；需包含 stdint
// faccessat 用于 arm32/x86_64 fallback
#include <stdint.h>
#include <unistd.h>

// For the portable libc fallback in apex_check_access() (only used when
// none of __aarch64__/__arm__/__x86_64__ is defined).
#if !defined(__aarch64__) && !defined(__arm__) && !defined(__x86_64__)
#include <unistd.h>
#endif

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif
#ifndef O_RDONLY
#define O_RDONLY 0
#endif
#ifndef O_WRONLY
#define O_WRONLY 1
#endif
#ifndef O_RDWR
#define O_RDWR 2
#endif
#ifndef O_CREAT
#define O_CREAT 0100
#endif
#ifndef O_DIRECTORY
#define O_DIRECTORY 0x10000
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif
#ifndef F_OK
#define F_OK 0
#endif
#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef MAP_SHARED
#define MAP_SHARED 0x01
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0x02
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef SIGUSR1
#define SIGUSR1 10
#endif
#ifndef SIGUSR2
#define SIGUSR2 12
#endif
#ifndef SIGSTOP
#define SIGSTOP 19
#endif
#ifndef __WALL
#define __WALL 0x40000000
#endif
#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif
#ifndef CLONE_NEWPID
#define CLONE_NEWPID 0x20000000
#endif
#ifndef CLONE_NEWNET
#define CLONE_NEWNET 0x40000000
#endif
#ifndef CLONE_NEWUTS
#define CLONE_NEWUTS 0x04000000
#endif
#ifndef CLONE_NEWIPC
#define CLONE_NEWIPC 0x08000000
#endif
#ifndef CLONE_NEWUSER
#define CLONE_NEWUSER 0x10000000
#endif
#ifndef CLONE_NEWCGROUP
#define CLONE_NEWCGROUP 0x02000000
#endif
#ifndef MS_BIND
#define MS_BIND 4096
#endif
#ifndef MS_PRIVATE
#define MS_PRIVATE (1<<18)
#endif
#ifndef MS_SLAVE
#define MS_SLAVE (1<<19)
#endif
#ifndef MS_REC
#define MS_REC 16384
#endif
#ifndef PIVOT_ROOT_NEW
#define PIVOT_ROOT_NEW 0
#endif

// ─────────────────────────────────────────────────────────────
// Per-arch Linux syscall numbers.
// arm32 numbers are from arch/arm/tools/syscall.tbl (EABI).
// arm64 numbers are from include/uapi/asm-generic/unistd.h.
// x86_64 numbers are from arch/x86/entry/syscalls/syscall_64.tbl.
// ─────────────────────────────────────────────────────────────
#if defined(__arm__)
// arm32 EABI (Aarch32). Syscall nr in r7, args in r0-r5, return in r0.
#define __NR_openat 322
#define __NR_read 3
#define __NR_write 4
#define __NR_close 6
#define __NR_access 33
#define __NR_getdents64 217
#define __NR_fork 2
#define __NR_kill 37
#define __NR_getpid 20
#define __NR_getppid 64
#define __NR_unshare 337
#define __NR_clone 120
#define __NR_mount 21
#define __NR_umount2 52
#define __NR_pivot_root 218
#define __NR_chdir 12
#define __NR_prctl 172
#define __NR_mmap2 192
#define __NR_munmap 11
#define __NR_seccomp 383
#define __NR_socket 281
#define __NR_connect 283
#define __NR_bind 282
#define __NR_listen 284
#define __NR_accept 285
#define __NR_dup3 358
#define __NR_fcntl 55
#define __NR_lseek 19
#define __NR_prlimit64 369
#define __NR_perf_event_open 364
#define __NR_ioctl 54
#define __NR_setns 375
#define __NR_rt_sigaction 174
#define __NR_nanosleep 162
#define __NR_clock_gettime 263
#define __NR_getrandom 384
#define __NR_epoll_create1 414
#define __NR_epoll_ctl 415
#define __NR_epoll_pwait 416
#define __NR_sched_setaffinity 122
#define __NR_pidfd_open 434
#define __NR_process_madvise 440
#define __NR_landlock_create_ruleset 444
#define __NR_landlock_add_rule 445
#define __NR_landlock_restrict_self 446
#define __NR_exit 1
#define __NR_getcwd 183
#define __NR_eventfd2 356
#define __NR_signalfd4 355
#ifndef __NR_ptrace
#define __NR_ptrace 26
#endif
#ifndef __NR_wait4
#define __NR_wait4 114
#endif
#ifndef __NR_mkdirat
#define __NR_mkdirat 323
#endif
#ifndef __NR_mmap
#define __NR_mmap 90
#endif
#elif defined(__x86_64__)
// x86_64. Syscall nr in rax, args in rdi,rsi,rdx,r10,r8,r9 (note: 4th is r10
// because the `syscall` instruction clobbers rcx and r11), return in rax.
#define __NR_openat 257
#define __NR_read 0
#define __NR_write 1
#define __NR_close 3
#define __NR_access 21
#define __NR_getdents64 217
#define __NR_fork 57
#define __NR_kill 62
#define __NR_getpid 39
#define __NR_getppid 110
#define __NR_unshare 272
#define __NR_clone 56
#define __NR_mount 165
#define __NR_umount2 39
#define __NR_pivot_root 155
#define __NR_chdir 80
#define __NR_prctl 157
#define __NR_mmap 9
#define __NR_munmap 11
#define __NR_seccomp 317
#define __NR_socket 41
#define __NR_connect 42
#define __NR_bind 49
#define __NR_listen 50
#define __NR_accept 43
#define __NR_dup3 292
#define __NR_fcntl 72
#define __NR_lseek 8
#define __NR_prlimit64 261
#define __NR_perf_event_open 298
#define __NR_ioctl 16
#define __NR_setns 308
#define __NR_rt_sigaction 13
#define __NR_nanosleep 35
#define __NR_clock_gettime 228
#define __NR_getrandom 318
#define __NR_epoll_create1 291
#define __NR_epoll_ctl 233
#define __NR_epoll_pwait 281
#define __NR_sched_setaffinity 203
#define __NR_pidfd_open 434
#define __NR_process_madvise 440
#define __NR_landlock_create_ruleset 444
#define __NR_landlock_add_rule 445
#define __NR_landlock_restrict_self 446
#define __NR_exit 60
#define __NR_getcwd 79
#define __NR_eventfd2 290
#define __NR_signalfd4 289
#ifndef __NR_ptrace
#define __NR_ptrace 101
#endif
#ifndef __NR_wait4
#define __NR_wait4 61
#endif
#ifndef __NR_mkdirat
#define __NR_mkdirat 258
#endif
#else
// arm64 (default). Syscall nr in x8, args in x0-x5, return in x0.
#define __NR_openat 56
#define __NR_read 63
#define __NR_write 64
#define __NR_close 57
#define __NR_access 48
#define __NR_getdents64 61
#define __NR_fork 220
#define __NR_kill 129
#define __NR_getpid 172
#define __NR_getppid 173
#define __NR_unshare 97
#define __NR_clone 220
#define __NR_mount 40
#define __NR_umount2 39
#define __NR_pivot_root 41
#define __NR_chdir 49
#define __NR_prctl 167
#define __NR_mmap 222
#define __NR_munmap 215
#define __NR_seccomp 277
#define __NR_socket 198
#define __NR_connect 203
#define __NR_bind 200
#define __NR_listen 201
#define __NR_accept 202
#define __NR_dup3 24
#define __NR_fcntl 25
#define __NR_lseek 62
#define __NR_prlimit64 261
#define __NR_perf_event_open 241
#define __NR_ioctl 29
#define __NR_setns 268
#define __NR_rt_sigaction 13
#define __NR_nanosleep 101
#define __NR_clock_gettime 113
#define __NR_getrandom 278
#define __NR_epoll_create1 20
#define __NR_epoll_ctl 21
#define __NR_epoll_pwait 22
#define __NR_sched_setaffinity 122
#define __NR_pidfd_open 434
#define __NR_process_madvise 440
#define __NR_landlock_create_ruleset 444
#define __NR_landlock_add_rule 445
#define __NR_landlock_restrict_self 446
#define __NR_exit 93
#define __NR_getcwd 17
#define __NR_eventfd2 290
#define __NR_signalfd4 289
#ifndef __NR_ptrace
#define __NR_ptrace 117
#endif
#ifndef __NR_wait4
#define __NR_wait4 260
#endif
#ifndef __NR_mkdirat
#define __NR_mkdirat 34
#endif
#endif

#ifndef SECCOMP_SET_MODE_FILTER
// SECCOMP_SET_MODE_FILTER is also declared as constexpr in sandbox_isolator.h
// Only define as macro if not already declared (e.g. when sandbox_isolator.h not included)
#endif
#ifndef SECCOMP_FILTER_FLAG_TSYNC
#define SECCOMP_FILTER_FLAG_TSYNC (1 << 0)
#endif
#ifndef SECCOMP_FILTER_FLAG_LOG
#define SECCOMP_FILTER_FLAG_LOG (1 << 1)
#endif
#ifndef SECCOMP_RET_KILL_PROCESS
// SECCOMP_RET_KILL_PROCESS declared as constexpr in sandbox_isolator.h
#endif
#ifndef SECCOMP_RET_ALLOW
// SECCOMP_RET_ALLOW declared as constexpr in sandbox_isolator.h
#endif

#define PR_SET_NO_NEW_PRIVS 38
#define PR_GET_NO_NEW_PRIVS 39
#define PR_SET_SECCOMP 22
#define PR_CAPBSET_READ 23
#define PR_CAPBSET_DROP 24

#define PERF_TYPE_SOFTWARE 1
#define PERF_COUNT_SW_DUMMY 9
#define PERF_FLAG_FD_CLOEXEC (1UL << 3)

#ifndef LANDLOCK_CREATE_RULESET_VERSION
// LANDLOCK_CREATE_RULESET_VERSION declared as constexpr in sandbox_isolator.h
#endif

#ifndef __NR_syscalls
#define __NR_syscalls 500
#endif

// ─────────────────────────────────────────────────────────────
// apex_check_access() — multi-arch faccessat() wrapper.
// arm64 has no access() syscall; must use faccessat (syscall 48) with
// (dirfd=AT_FDCWD, path, mode=F_OK, flags=0). arm32 also has faccessat at
// syscall 48 (EABI). x86_64 has faccessat at syscall 269.
// On any other arch, fall back to libc access().
// ─────────────────────────────────────────────────────────────
static inline int64_t apex_check_access(const char* path) {
#if defined(__aarch64__)
    // AAPCS64: x8 = syscall nr, x0-x5 = args, return in x0.
    register int64_t x8 asm("x8") = 48;          /* __NR_faccessat */
    register int64_t x0 asm("x0") = (int64_t)AT_FDCWD;
    register int64_t x1 asm("x1") = (int64_t)path;
    register int64_t x2 asm("x2") = (int64_t)F_OK;
    register int64_t x3 asm("x3") = 0;
    asm volatile("svc #0"
                 : "+r"(x0)
                 : "r"(x8), "r"(x1), "r"(x2), "r"(x3)
                 : "memory");
    return x0;
#elif defined(__arm__)
    // arm32: r7 is reserved in Thumb mode. Use libc faccessat() instead.
    // faccessat(AT_FDCWD, path, F_OK, 0) — 0 on success.
    return (int64_t)faccessat(AT_FDCWD, path, F_OK, 0);
#elif defined(__x86_64__)
    // x86_64: rax = syscall nr, args in rdi,rsi,rdx,r10,r8,r9.
    // Note: 4th arg uses r10 (not rcx) because `syscall` clobbers rcx & r11.
    register long rdi asm("rdi") = (long)AT_FDCWD;
    register const char* rsi asm("rsi") = path;
    register long rdx asm("rdx") = (long)F_OK;
    register long r10 asm("r10") = 0;
    long ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "0"(269L /* __NR_faccessat */),
                   "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10)
                 : "rcx", "r11", "memory");
    return (int64_t)ret;
#else
    // Portable fallback: libc access(path, F_OK). Returns 0 on success.
    return (int64_t)access(path, F_OK);
#endif
}

#endif

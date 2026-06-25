/*
 * syscall_arm64.S - Pure ARM64 assembly bare syscall wrappers
 *
 * Domain D: 底层裸系统汇编抽象层
 * NO libc dependency - all syscalls via svc #0
 * ARM64 calling convention: x0-x7 args, x8 syscall nr, x0 return
 *
 * Linux ARM64 syscall numbers from uapi/asm-generic/unistd.h
 * and arch-specific asm/unistd.h
 */

    .text
    .align 2

/* ================================================================
 * ARM64 Linux syscall number definitions
 * ================================================================ */
#define __NR_sched_setattr      274
#define __NR_sched_getattr      275
#define __NR_rt_sigprocmask     135
#define __NR_clone              220
#define __NR_fork                57

/* ================================================================
 * Generic bare syscall wrapper
 * long bare_syscall(long number, long a1, long a2, long a3,
 *                   long a4, long a5, long a6)
 *
 * C caller puts:  number=x0, a1=x1, a2=x2, a3=x3, a4=x4, a5=x5, a6=x6
 * Kernel expects: nr=x8,    a1=x0, a2=x1, a3=x2, a4=x3, a5=x4, a6=x5
 * ================================================================ */
    .global bare_syscall
    .type   bare_syscall, %function
bare_syscall:
    mov     x8,  x0
    mov     x0,  x1
    mov     x1,  x2
    mov     x2,  x3
    mov     x3,  x4
    mov     x4,  x5
    mov     x5,  x6
    svc     #0
    ret
    .size   bare_syscall, .-bare_syscall

/* ================================================================
 * SYS_openat (56)
 * int bare_openat(int dirfd, const char *pathname, int flags, int mode)
 * ================================================================ */
    .global bare_openat
    .type   bare_openat, %function
bare_openat:
    mov     x8, #56
    svc     #0
    ret
    .size   bare_openat, .-bare_openat

/* ================================================================
 * SYS_read (63)
 * ssize_t bare_read(int fd, void *buf, size_t count)
 * ================================================================ */
    .global bare_read
    .type   bare_read, %function
bare_read:
    mov     x8, #63
    svc     #0
    ret
    .size   bare_read, .-bare_read

/* ================================================================
 * SYS_write (64)
 * ssize_t bare_write(int fd, const void *buf, size_t count)
 * ================================================================ */
    .global bare_write
    .type   bare_write, %function
bare_write:
    mov     x8, #64
    svc     #0
    ret
    .size   bare_write, .-bare_write

/* ================================================================
 * SYS_close (57)
 * int bare_close(int fd)
 * ================================================================ */
    .global bare_close
    .type   bare_close, %function
bare_close:
    mov     x8, #57
    svc     #0
    ret
    .size   bare_close, .-bare_close

/* ================================================================
 * SYS_getdents64 (61)
 * ssize_t bare_getdents64(int fd, struct linux_dirent64 *dirp, size_t count)
 * ================================================================ */
    .global bare_getdents64
    .type   bare_getdents64, %function
bare_getdents64:
    mov     x8, #61
    svc     #0
    ret
    .size   bare_getdents64, .-bare_getdents64

/* ================================================================
 * SYS_lseek (62)
 * off_t bare_lseek(int fd, off_t offset, int whence)
 * ================================================================ */
    .global bare_lseek
    .type   bare_lseek, %function
bare_lseek:
    mov     x8, #62
    svc     #0
    ret
    .size   bare_lseek, .-bare_lseek

/* ================================================================
 * SYS_mmap (222)
 * void* bare_mmap(void *addr, size_t length, int prot,
 *                 int flags, int fd, off_t offset)
 * ================================================================ */
    .global bare_mmap
    .type   bare_mmap, %function
bare_mmap:
    mov     x8, #222
    svc     #0
    ret
    .size   bare_mmap, .-bare_mmap

/* ================================================================
 * SYS_munmap (215)
 * int bare_munmap(void *addr, size_t length)
 * ================================================================ */
    .global bare_munmap
    .type   bare_munmap, %function
bare_munmap:
    mov     x8, #215
    svc     #0
    ret
    .size   bare_munmap, .-bare_munmap

/* ================================================================
 * SYS_truncate (46)
 * int bare_truncate(const char *path, off_t length)
 * ================================================================ */
    .global bare_truncate
    .type   bare_truncate, %function
bare_truncate:
    mov     x8, #46
    svc     #0
    ret
    .size   bare_truncate, .-bare_truncate

/* ================================================================
 * SYS_newfstatat / SYS_stat (79)
 * int bare_stat(const char *pathname, struct kernel_stat *statbuf)
 * Implemented via newfstatat(AT_FDCWD, path, buf, 0)
 * ================================================================ */
    .global bare_stat
    .type   bare_stat, %function
bare_stat:
    /* x0=pathname, x1=statbuf; need newfstatat(AT_FDCWD, path, buf, 0) */
    mov     x3,  x0          /* x3 = pathname (arg3 to kernel) */
    mov     x4,  x1          /* x4 = statbuf   (arg4 to kernel) */
    mov     x0, #-100        /* x0 = AT_FDCWD  (arg1 to kernel) */
    mov     x1,  x3          /* x1 = pathname  (arg2 to kernel) */
    mov     x2,  x4          /* x2 = statbuf   (arg3 to kernel) */
    mov     x3,  #0          /* x3 = flags=0   (arg4 to kernel) */
    mov     x8, #79          /* SYS_newfstatat */
    svc     #0
    ret
    .size   bare_stat, .-bare_stat

/* ================================================================
 * SYS_fstat (80)
 * int bare_fstat(int fd, struct kernel_stat *statbuf)
 * Implemented via newfstatat(fd, "", buf, AT_EMPTY_PATH)
 * ================================================================ */
    .global bare_fstat
    .type   bare_fstat, %function
bare_fstat:
    /* x0=fd, x1=statbuf */
    mov     x3,  x0          /* x3 = fd        -> arg1 */
    mov     x4,  x1          /* x4 = statbuf   -> arg3 */
    mov     x0,  x3          /* x0 = fd */
    adr     x1, .Lempty_str  /* x1 = "" */
    mov     x2,  x4          /* x2 = statbuf */
    mov     x3,  #0x1000     /* AT_EMPTY_PATH */
    mov     x8,  #79         /* SYS_newfstatat */
    svc     #0
    ret
    .size   bare_fstat, .-bare_fstat

.Lempty_str:
    .byte   0
    .align  2

/* ================================================================
 * SYS_readlinkat (78)
 * ssize_t bare_readlinkat(int dirfd, const char *pathname,
 *                         char *buf, size_t bufsiz)
 * ================================================================ */
    .global bare_readlinkat
    .type   bare_readlinkat, %function
bare_readlinkat:
    mov     x8, #78
    svc     #0
    ret
    .size   bare_readlinkat, .-bare_readlinkat

/* ================================================================
 * SYS_sched_setaffinity (122)
 * int bare_sched_setaffinity(pid_t pid, size_t cpusetsize,
 *                            const void *mask)
 * ================================================================ */
    .global bare_sched_setaffinity
    .type   bare_sched_setaffinity, %function
bare_sched_setaffinity:
    mov     x8, #122
    svc     #0
    ret
    .size   bare_sched_setaffinity, .-bare_sched_setaffinity

/* ================================================================
 * SYS_clock_gettime (113)
 * int bare_clock_gettime(int clk_id, struct kernel_timespec *tp)
 * ================================================================ */
    .global bare_clock_gettime
    .type   bare_clock_gettime, %function
bare_clock_gettime:
    mov     x8, #113
    svc     #0
    ret
    .size   bare_clock_gettime, .-bare_clock_gettime

/* ================================================================
 * SYS_faccessat (48) - used as bare_access
 * int bare_access(const char *pathname, int mode)
 * Implemented via faccessat(AT_FDCWD, path, mode, 0)
 * ================================================================ */
    .global bare_access
    .type   bare_access, %function
bare_access:
    mov     x2,  x0          /* save mode -> x2 */
    mov     x3,  x1          /* save mode -> x3 (arg3) */
    mov     x1,  x0          /* x1 = pathname (arg2) */
    mov     x0, #-100        /* x0 = AT_FDCWD (arg1) */
    mov     x2,  x3          /* x2 = mode (arg3) */
    mov     x3,  #0          /* x3 = flags (arg4) */
    mov     x8,  #48         /* SYS_faccessat */
    svc     #0
    ret
    .size   bare_access, .-bare_access

/* ================================================================
 * SYS_getpid (172)
 * ================================================================ */
    .global bare_getpid
    .type   bare_getpid, %function
bare_getpid:
    mov     x8, #172
    svc     #0
    ret
    .size   bare_getpid, .-bare_getpid

/* ================================================================
 * SYS_gettid (178)
 * ================================================================ */
    .global bare_gettid
    .type   bare_gettid, %function
bare_gettid:
    mov     x8, #178
    svc     #0
    ret
    .size   bare_gettid, .-bare_gettid

/* ================================================================
 * SYS_fstatat (79) - raw newfstatat for direct callers
 * int bare_fstatat(int dirfd, const char *path,
 *                  struct kernel_stat *statbuf, int flags)
 * ================================================================ */
    .global bare_fstatat
    .type   bare_fstatat, %function
bare_fstatat:
    mov     x8, #79
    svc     #0
    ret
    .size   bare_fstatat, .-bare_fstatat

/* ================================================================
 * SYS_clone (220) - fork support
 * long bare_fork(void)
 * ================================================================ */
    .global bare_fork
    .type   bare_fork, %function
bare_fork:
    mov     x8, #220
    svc     #0
    ret
    .size   bare_fork, .-bare_fork

/* ================================================================
 * SYS_exit (93)
 * void bare_exit(int status)
 * ================================================================ */
    .global bare_exit
    .type   bare_exit, %function
bare_exit:
    mov     x8, #93
    svc     #0
    ret
    .size   bare_exit, .-bare_exit

/* ================================================================
 * SYS_sched_yield (124)
 * int bare_sched_yield(void)
 * ================================================================ */
    .global bare_sched_yield
    .type   bare_sched_yield, %function
bare_sched_yield:
    mov     x8, #124
    svc     #0
    ret
    .size   bare_sched_yield, .-bare_sched_yield

/* ================================================================
 * Stack canary for -fstack-protector (if enabled)
 * ================================================================ */
#if defined(__ANDROID__) || defined(__linux__)
    .section .note.gnu.property, "a"
    .balign 8
    .long   4
    .long   0x10
    .long   5
    .asciz  "GNU"
    .long   0xc0000000
    .long   4
    .long   0
    .previous
#endif

/* Mark stack as non-executable */
    .section .note.GNU-stack, "", %progbits

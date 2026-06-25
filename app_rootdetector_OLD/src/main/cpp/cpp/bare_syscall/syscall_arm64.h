/*
 * syscall_arm64.h - C declarations for ARM64 bare assembly syscall wrappers
 *
 * Domain D: 底层裸系统汇编抽象层
 * All functions implemented in syscall_arm64.S using svc #0
 * NO libc dependency whatsoever
 */

#ifndef SYSCALL_ARM64_H
#define SYSCALL_ARM64_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * Kernel structure definitions (avoid libc header dependency)
 * ================================================================ */

/* ARM64 Linux kernel stat structure (from uapi/asm-generic/stat.h) */
struct kernel_stat {
    uint64_t    st_dev;
    uint64_t    st_ino;
    uint32_t    st_mode;
    uint32_t    st_nlink;
    uint32_t    st_uid;
    uint32_t    st_gid;
    uint64_t    st_rdev;
    int64_t     st_size;
    int32_t     st_blksize;
    int32_t     st_pad2;
    int64_t     st_blocks;
    int64_t     st_atime;
    uint64_t    st_atime_nsec;
    int64_t     st_mtime;
    uint64_t    st_mtime_nsec;
    int64_t     st_ctime;
    uint64_t    st_ctime_nsec;
    uint32_t    __unused4;
    uint32_t    __unused5;
};

/* Kernel timespec for clock_gettime */
struct kernel_timespec {
    int64_t     tv_sec;
    long        tv_nsec;
};

/* linux_dirent64 for getdents64 */
struct linux_dirent64 {
    uint64_t        d_ino;
    int64_t         d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char            d_name[];
};

/* ================================================================
 * ARM64 syscall number constants
 * ================================================================ */
#define SYS_OPENAT              56
#define SYS_CLOSE               57
#define SYS_LSEEK               62
#define SYS_READ                63
#define SYS_WRITE               64
#define SYS_GETDENTS64          61
#define SYS_FACCESSAT           48
#define SYS_TRUNCATE            46
#define SYS_NEWFSTATAT          79
#define SYS_FSTAT               80
#define SYS_READLINKAT          78
#define SYS_MMAP                222
#define SYS_MUNMAP              215
#define SYS_SCHED_SETAFFINITY   122
#define SYS_CLOCK_GETTIME       113
#define SYS_GETPID              172
#define SYS_GETTID              178

/* ================================================================
 * Open flags (from uapi/asm-generic/fcntl.h)
 * ================================================================ */
#define BARE_O_RDONLY           0x0000
#define BARE_O_WRONLY           0x0001
#define BARE_O_RDWR             0x0002
#define BARE_O_CREAT            0x0040
#define BARE_O_EXCL             0x0080
#define BARE_O_TRUNC            0x0200
#define BARE_O_APPEND           0x0400
#define BARE_O_NONBLOCK         0x0800
#define BARE_O_CLOEXEC          0x80000
#define BARE_O_DIRECTORY        0x10000

/* AT_FDCWD special value */
#define BARE_AT_FDCWD           (-100)
#define BARE_AT_EMPTY_PATH      0x1000
#define BARE_AT_SYMLINK_NOFOLLOW 0x100

/* F_OK / R_OK / W_OK / X_OK */
#define BARE_F_OK               0
#define BARE_R_OK               4
#define BARE_W_OK               2
#define BARE_X_OK               1

/* mmap prot flags */
#define BARE_PROT_NONE          0x0
#define BARE_PROT_READ          0x1
#define BARE_PROT_WRITE         0x2
#define BARE_PROT_EXEC          0x4

/* mmap flags */
#define BARE_MAP_SHARED         0x01
#define BARE_MAP_PRIVATE        0x02
#define BARE_MAP_FIXED          0x10
#define BARE_MAP_ANONYMOUS      0x20
#define BARE_MAP_ANON           BARE_MAP_ANONYMOUS

/* lseek whence */
#define BARE_SEEK_SET           0
#define BARE_SEEK_CUR           1
#define BARE_SEEK_END           2

/* clock IDs */
#define BARE_CLOCK_MONOTONIC        1
#define BARE_CLOCK_MONOTONIC_RAW    4
#define BARE_CLOCK_BOOTTIME         7

/* dirent d_type values */
#define BARE_DT_UNKNOWN         0
#define BARE_DT_FIFO            1
#define BARE_DT_CHR             2
#define BARE_DT_DIR             4
#define BARE_DT_BLK             6
#define BARE_DT_REG             8
#define BARE_DT_LNK             10
#define BARE_DT_SOCK            12

/* Error codes (negative returns from syscalls) */
#define BARE_EPERM              1
#define BARE_ENOENT             2
#define BARE_ESRCH              3
#define BARE_EINTR              4
#define BARE_EIO                5
#define BARE_ENXIO              6
#define BARE_E2BIG              7
#define BARE_ENOEXEC            8
#define BARE_EBADF              9
#define BARE_ECHILD             10
#define BARE_EAGAIN             11
#define BARE_ENOMEM             12
#define BARE_EACCES             13
#define BARE_EFAULT             14
#define BARE_EBUSY              16
#define BARE_EEXIST             17
#define BARE_ENOTDIR            20
#define BARE_EISDIR             21
#define BARE_EINVAL             22
#define BARE_ENFILE             23
#define BARE_EMFILE             24
#define BARE_ENOSPC             28
#define BARE_ERANGE             34

/* MAP_FAILED equivalent */
#define BARE_MAP_FAILED         ((void *)-1)

/* ================================================================
 * Generic syscall wrapper
 * ================================================================ */
long bare_syscall(long number, ...);

/* ================================================================
 * Individual bare syscall wrappers (assembly implemented)
 * All return negative errno on failure, unless noted otherwise.
 * ================================================================ */

/* File descriptor operations */
long bare_openat(int dirfd, const char *pathname, int flags, int mode);
long bare_close(int fd);
long bare_lseek(int fd, long offset, int whence);

/* Read/Write */
long bare_read(int fd, void *buf, size_t count);
long bare_write(int fd, const void *buf, size_t count);

/* Directory enumeration */
long bare_getdents64(int fd, void *dirp, size_t count);

/* Memory mapping */
long bare_mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);
long bare_munmap(void *addr, size_t length);

/* File status */
long bare_stat(const char *pathname, struct kernel_stat *statbuf);
long bare_fstat(int fd, struct kernel_stat *statbuf);
long bare_fstatat(int dirfd, const char *path, struct kernel_stat *statbuf, int flags);

/* Path operations */
long bare_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
long bare_access(const char *pathname, int mode);
long bare_truncate(const char *path, long length);

/* Scheduling */
long bare_sched_setaffinity(int pid, size_t cpusetsize, const void *mask);

/* Time */
long bare_clock_gettime(int clk_id, struct kernel_timespec *tp);

/* Process info */
int bare_getpid(void);
int bare_gettid(void);

/* ================================================================
 * Inline helpers
 * ================================================================ */

/* Check if a syscall return value indicates error (negative, in -1..-4095) */
static inline int bare_is_error(long ret) {
    return (ret < 0 && ret >= -4095);
}

/* Convert syscall return to errno (positive) */
static inline int bare_get_errno(long ret) {
    return (ret < 0) ? (int)(-ret) : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* SYSCALL_ARM64_H */

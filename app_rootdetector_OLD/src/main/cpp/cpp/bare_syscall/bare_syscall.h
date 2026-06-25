#ifndef BARE_SYSCALL_H
#define BARE_SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== ARM64 直接系统调用 ========== */
long bare_syscall(long number, ...);
long bare_openat(int dirfd, const char *pathname, int flags, mode_t mode);
long bare_read(int fd, void *buf, size_t count);
long bare_write(int fd, const void *buf, size_t count);
long bare_close(int fd);
long bare_getdents64(int fd, void *dirp, size_t count);
long bare_fstatat(int dirfd, const char *path, void *statbuf, int flags);
long bare_readlinkat(int dirfd, const char *path, char *buf, size_t bufsize);
long bare_access(const char *pathname, int mode);
long bare_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
long bare_munmap(void *addr, size_t length);
pid_t bare_getpid(void);
pid_t bare_gettid(void);

/* ========== CPU 控制 ========== */
int bare_bind_cpu(int cpu_id);
int bare_get_cpu_count(void);
void bare_cache_flush(void *addr, size_t len);
uint64_t bare_rdtsc(void);

/* ========== 裸文件 I/O（纯 syscall，无 libc 包装）========== */
int bare_open(const char *path, int flags, mode_t mode);
ssize_t bare_read_full(int fd, void *buf, size_t count);
ssize_t bare_write_full(int fd, const void *buf, size_t count);
int bare_exists(const char *path);
long bare_lseek(int fd, long offset, int whence);

/* ========== 进程控制（纯 syscall） ========== */
long bare_fork(void);
void bare_exit(int status);
int bare_sched_yield(void);

/* ========== 裸内存操作 ========== */
void *bare_memcpy(void *dest, const void *src, size_t n);
int bare_memcmp(const void *s1, const void *s2, size_t n);
void *bare_memset(void *s, int c, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* BARE_SYSCALL_H */
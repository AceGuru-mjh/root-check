// ─── Portable libc-based syscall implementations ─────────
// 用于 armeabi-v7a (arm32) 和 x86_64 架构。
// arm64 有独立的 syscall_arm64.cpp（使用裸 svc 指令绕过 libc hook），
// 其他架构使用 bionic libc 的标准 POSIX 封装，功能等价但不绕过 hook。
//
// 何时使用此文件：编译目标非 __aarch64__ 时（即 arm32 / x86_64 / i386 等）。
#if !defined(__aarch64__)

#include "syscall_bridge.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/epoll.h>
#include <sys/random.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// ─── File operations ──────────────────────────────────────
int64_t bs_openat(int64_t dirfd, const char* path, int flags, int mode) {
    return (int64_t)openat((int)dirfd, path, flags, (mode_t)mode);
}
int64_t bs_read(int64_t fd, void* buf, size_t count) {
    return (int64_t)read((int)fd, buf, count);
}
int64_t bs_write(int64_t fd, const void* buf, size_t count) {
    return (int64_t)write((int)fd, buf, count);
}
int64_t bs_close(int64_t fd) {
    return (int64_t)close((int)fd);
}
int64_t bs_mmap(void* addr, size_t length, int prot, int flags, int fd, int64_t offset) {
    void* ret = mmap(addr, length, prot, flags, (int)fd, (off_t)offset);
    return ret == MAP_FAILED ? -(int64_t)errno : (int64_t)ret;
}
int64_t bs_munmap(void* addr, size_t length) {
    return (int64_t)munmap(addr, length);
}

// ─── Directory operations ─────────────────────────────────
int64_t bs_getdents64(int64_t fd, void* dirp, size_t count) {
    // syscall(SYS_getdents64, ...) — 直接走 libc syscall 封装
    return (int64_t)syscall(SYS_getdents64, (int)fd, dirp, count);
}

// ─── Process operations ───────────────────────────────────
int64_t bs_fork() {
    return (int64_t)fork();
}
int64_t bs_exit(int status) {
    _exit(status);
    return 0;
}
int64_t bs_kill(int pid, int sig) {
    return (int64_t)kill(pid, sig);
}
int64_t bs_getpid() {
    return (int64_t)getpid();
}
int64_t bs_getppid() {
    return (int64_t)getppid();
}

// ─── Debug ────────────────────────────────────────────────
int64_t bs_ptrace(int request, int pid, void* addr, void* data) {
    // 修复：直接用 syscall 避免 enum __ptrace_request 转换问题
    return (int64_t)syscall(SYS_ptrace, (long)request, (long)pid, addr, data);
}

// ─── Scheduling ───────────────────────────────────────────
int64_t bs_sched_setaffinity(int64_t pid, size_t cpusetsize, const uint64_t* mask) {
    return (int64_t)sched_setaffinity((pid_t)pid, cpusetsize, (const cpu_set_t*)mask);
}

// ─── Socket ───────────────────────────────────────────────
int64_t bs_socket(int domain, int type, int protocol) {
    return (int64_t)socket(domain, type, protocol);
}
int64_t bs_connect(int64_t sockfd, const void* addr, uint32_t addrlen) {
    return (int64_t)connect((int)sockfd, (const struct sockaddr*)addr, (socklen_t)addrlen);
}
int64_t bs_bind(int64_t sockfd, const void* addr, uint32_t addrlen) {
    return (int64_t)bind((int)sockfd, (const struct sockaddr*)addr, (socklen_t)addrlen);
}
int64_t bs_listen(int64_t sockfd, int backlog) {
    return (int64_t)listen((int)sockfd, backlog);
}
int64_t bs_accept(int64_t sockfd, void* addr, uint32_t* addrlen) {
    socklen_t len = addrlen ? *addrlen : 0;
    int ret = accept((int)sockfd, (struct sockaddr*)addr, &len);
    if (addrlen) *addrlen = len;
    return (int64_t)ret;
}

// ─── Namespace ────────────────────────────────────────────
int64_t bs_unshare(int flags) {
    return (int64_t)unshare(flags);
}
int64_t bs_setns(int64_t fd, int nstype) {
    return (int64_t)setns((int)fd, nstype);
}

// ─── Process ──────────────────────────────────────────────
int64_t bs_clone(int64_t flags, void* stack, void* parent_tid, void* tls, void* child_tid) {
    // clone 的 libc 签名复杂，直接走 syscall
    return (int64_t)syscall(SYS_clone, (long)flags, stack, parent_tid, tls, child_tid);
}
int64_t bs_getcwd(void* buf, size_t size) {
    char* ret = getcwd((char*)buf, size);
    return ret ? (int64_t)strlen((char*)buf) : -(int64_t)errno;
}

// ─── Security ─────────────────────────────────────────────
int64_t bs_seccomp(int op, int flags, const void* prog) {
    return (int64_t)syscall(SYS_seccomp, op, flags, prog);
}
int64_t bs_prctl(int op, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    return (int64_t)prctl(op, (unsigned long)a2, (unsigned long)a3, (unsigned long)a4, (unsigned long)a5);
}

// ─── Landlock LSM ─────────────────────────────────────────
int64_t bs_landlock_create_ruleset(const void* attr, size_t size, uint32_t flags) {
#ifdef SYS_landlock_create_ruleset
    return (int64_t)syscall(SYS_landlock_create_ruleset, attr, size, flags);
#else
    errno = ENOSYS; return -1;
#endif
}
int64_t bs_landlock_add_rule(int ruleset_fd, int type, const void* attr, uint32_t flags) {
#ifdef SYS_landlock_add_rule
    return (int64_t)syscall(SYS_landlock_add_rule, ruleset_fd, type, attr, flags);
#else
    errno = ENOSYS; return -1;
#endif
}
int64_t bs_landlock_restrict_self(int ruleset_fd, uint32_t flags) {
#ifdef SYS_landlock_restrict_self
    return (int64_t)syscall(SYS_landlock_restrict_self, ruleset_fd, flags);
#else
    errno = ENOSYS; return -1;
#endif
}

// ─── Mount ────────────────────────────────────────────────
int64_t bs_mount(const char* source, const char* target, const char* fstype, uint64_t flags, const void* data) {
    // 修复：使用 syscall 避免某些 NDK 版本 mount 函数声明不可见
    return (int64_t)syscall(SYS_mount, source, target, fstype, (long)flags, data);
}
int64_t bs_umount2(const char* target, int flags) {
    return (int64_t)syscall(SYS_umount2, target, flags);
}
int64_t bs_chdir(const char* path) {
    return (int64_t)chdir(path);
}
int64_t bs_pivot_root(const char* new_root, const char* put_old) {
#ifdef SYS_pivot_root
    return (int64_t)syscall(SYS_pivot_root, new_root, put_old);
#else
    errno = ENOSYS; return -1;
#endif
}

// ─── FD operations ────────────────────────────────────────
int64_t bs_dup3(int oldfd, int newfd, int flags) {
    return (int64_t)dup3(oldfd, newfd, flags);
}
int64_t bs_fcntl(int fd, int cmd, int64_t arg) {
    return (int64_t)fcntl(fd, cmd, arg);
}
int64_t bs_lseek(int fd, int64_t offset, int whence) {
    return (int64_t)lseek(fd, (off_t)offset, whence);
}

// ─── Event monitoring ─────────────────────────────────────
int64_t bs_eventfd2(uint32_t initval, int flags) {
#ifdef SYS_eventfd2
    return (int64_t)syscall(SYS_eventfd2, initval, flags);
#else
    errno = ENOSYS; return -1;
#endif
}
int64_t bs_signalfd4(int fd, const void* mask, size_t masksize, int flags) {
#ifdef SYS_signalfd4
    return (int64_t)syscall(SYS_signalfd4, fd, mask, masksize, flags);
#else
    errno = ENOSYS; return -1;
#endif
}
int64_t bs_epoll_create1(int flags) {
    return (int64_t)syscall(SYS_epoll_create1, flags);
}
int64_t bs_epoll_ctl(int epfd, int op, int fd, const void* event) {
    return (int64_t)syscall(SYS_epoll_ctl, epfd, op, fd, event);
}
int64_t bs_epoll_pwait(int epfd, void* events, int maxevents, int timeout, const void* sigmask, size_t sigsetsize) {
#ifdef SYS_epoll_pwait
    return (int64_t)syscall(SYS_epoll_pwait, epfd, events, maxevents, timeout, sigmask, sigsetsize);
#else
    return (int64_t)epoll_pwait(epfd, (struct epoll_event*)events, maxevents, timeout, (const sigset_t*)sigmask);
#endif
}

// ─── Resource ─────────────────────────────────────────────
int64_t bs_prlimit64(int pid, int resource, const void* new_rlim, void* old_rlim) {
#ifdef SYS_prlimit64
    return (int64_t)syscall(SYS_prlimit64, pid, resource, new_rlim, old_rlim);
#else
    errno = ENOSYS; return -1;
#endif
}

// ─── PMU ──────────────────────────────────────────────────
int64_t bs_perf_event_open(const void* attr, int pid, int cpu, int group_fd, uint64_t flags) {
    return (int64_t)syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

// ─── IOCTL ────────────────────────────────────────────────
int64_t bs_ioctl(int64_t fd, int64_t request, int64_t arg) {
    return (int64_t)ioctl((int)fd, (unsigned long)request, (void*)arg);
}

// ─── Time ─────────────────────────────────────────────────
int64_t bs_clock_gettime(int clk_id, int64_t tp) {
    return (int64_t)clock_gettime(clk_id, (struct timespec*)(uintptr_t)tp);
}

// ─── Utility ──────────────────────────────────────────────
int64_t bs_nanosleep(uint64_t ns) {
    struct timespec ts;
    ts.tv_sec = (time_t)(ns / 1000000000ULL);
    ts.tv_nsec = (long)(ns % 1000000000ULL);
    return (int64_t)nanosleep(&ts, nullptr);
}

uint64_t bs_clock_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

uint64_t bs_get_random() {
    uint64_t buf = 0;
    // 修复：使用 syscall(SYS_getrandom, ...) 跨架构可用
    syscall(SYS_getrandom, &buf, sizeof(buf), 0);
    return buf;
}

#endif  // !defined(__aarch64__)

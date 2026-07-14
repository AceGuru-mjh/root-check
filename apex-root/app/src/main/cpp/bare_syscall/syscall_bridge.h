#ifndef APEX_ROOT_SYSCALL_BRIDGE_H
#define APEX_ROOT_SYSCALL_BRIDGE_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// dirent64 type for getdents64
struct apex_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

// These are only defined if not building with bionic/libc system headers.
// The plugins that need pid_t/uid_t should use int32_t/uint32_t directly.

// ─── File operations ──────────────────────────────────────
int64_t bs_openat(int64_t dirfd, const char* path, int flags, int mode);
int64_t bs_read(int64_t fd, void* buf, size_t count);
int64_t bs_write(int64_t fd, const void* buf, size_t count);
int64_t bs_close(int64_t fd);
int64_t bs_mmap(void* addr, size_t length, int prot, int flags, int fd, int64_t offset);
int64_t bs_munmap(void* addr, size_t length);

// ─── Directory operations ─────────────────────────────────
int64_t bs_getdents64(int64_t fd, void* dirp, size_t count);

// ─── Process operations ───────────────────────────────────
int64_t bs_fork();
int64_t bs_exit(int status);
int64_t bs_kill(int pid, int sig);
int64_t bs_getpid();
int64_t bs_getppid();

// ─── Debug ────────────────────────────────────────────────
int64_t bs_ptrace(int request, int pid, void* addr, void* data);

// ─── Scheduling ───────────────────────────────────────────
int64_t bs_sched_setaffinity(int64_t pid, size_t cpusetsize, const uint64_t* mask);

// ─── Socket ───────────────────────────────────────────────
int64_t bs_socket(int domain, int type, int protocol);
int64_t bs_connect(int64_t sockfd, const void* addr, uint32_t addrlen);
int64_t bs_bind(int64_t sockfd, const void* addr, uint32_t addrlen);
int64_t bs_listen(int64_t sockfd, int backlog);
int64_t bs_accept(int64_t sockfd, void* addr, uint32_t* addrlen);

// ─── Namespace ────────────────────────────────────────────
int64_t bs_unshare(int flags);
int64_t bs_setns(int64_t fd, int nstype);

// ─── Process ──────────────────────────────────────────────
int64_t bs_clone(int64_t flags, void* stack, void* parent_tid, void* tls, void* child_tid);
int64_t bs_getcwd(void* buf, size_t size);

// ─── Security ─────────────────────────────────────────────
int64_t bs_seccomp(int op, int flags, const void* prog);
int64_t bs_prctl(int op, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5);

// ─── Landlock LSM ─────────────────────────────────────────
int64_t bs_landlock_create_ruleset(const void* attr, size_t size, uint32_t flags);
int64_t bs_landlock_add_rule(int ruleset_fd, int type, const void* attr, uint32_t flags);
int64_t bs_landlock_restrict_self(int ruleset_fd, uint32_t flags);

// ─── Mount ────────────────────────────────────────────────
int64_t bs_mount(const char* source, const char* target, const char* fstype, uint64_t flags, const void* data);
int64_t bs_umount2(const char* target, int flags);
int64_t bs_chdir(const char* path);
int64_t bs_pivot_root(const char* new_root, const char* put_old);

// ─── FD operations ────────────────────────────────────────
int64_t bs_dup3(int oldfd, int newfd, int flags);
int64_t bs_fcntl(int fd, int cmd, int64_t arg);
int64_t bs_lseek(int fd, int64_t offset, int whence);

// ─── Event monitoring ─────────────────────────────────────
int64_t bs_eventfd2(uint32_t initval, int flags);
int64_t bs_signalfd4(int fd, const void* mask, size_t masksize, int flags);
int64_t bs_epoll_create1(int flags);
int64_t bs_epoll_ctl(int epfd, int op, int fd, const void* event);
int64_t bs_epoll_pwait(int epfd, void* events, int maxevents, int timeout, const void* sigmask, size_t sigsetsize);

// ─── Resource ─────────────────────────────────────────────
int64_t bs_prlimit64(int pid, int resource, const void* new_rlim, void* old_rlim);

// ─── PMU ──────────────────────────────────────────────────
int64_t bs_perf_event_open(const void* attr, int pid, int cpu, int group_fd, uint64_t flags);

// ─── IOCTL ────────────────────────────────────────────────
int64_t bs_ioctl(int64_t fd, int64_t request, int64_t arg);

// ─── Time ─────────────────────────────────────────────────
int64_t bs_clock_gettime(int clk_id, int64_t tp);

// ─── Utility ──────────────────────────────────────────────
int64_t bs_nanosleep(uint64_t ns);
uint64_t bs_clock_ns();
uint64_t bs_get_random();

#ifdef __cplusplus
}
#endif

#endif

#ifndef APEX_ROOT_SYSCALL_H
#define APEX_ROOT_SYSCALL_H

#define AT_FDCWD (-100)
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_DIRECTORY 0x10000
#define O_CLOEXEC 02000000
#define F_OK 0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define SIGKILL 9
#define CLONE_NEWNS 0x00020000
#define CLONE_NEWPID 0x02000000
#define CLONE_NEWNET 0x40000000
#define CLONE_NEWUTS 0x04000000
#define CLONE_NEWIPC 0x08000000
#define CLONE_NEWUSER 0x10000000
#define CLONE_NEWCGROUP 0x02000000
#define MS_BIND 4096
#define MS_PRIVATE (1<<18)
#define MS_SLAVE (1<<19)
#define MS_REC 16384
#define PIVOT_ROOT_NEW 0

#ifdef __arm__
#define __NR_openat 322
#define __NR_read 63
#define __NR_write 64
#define __NR_close 57
#define __NR_access 48
#define __NR_getdents64 217
#define __NR_fork 220
#define __NR_kill 137
#define __NR_getpid 172
#define __NR_unshare 397
#define __NR_clone 220
#define __NR_mount 231
#define __NR_umount2 232
#define __NR_pivot_root 416
#define __NR_chdir 179
#define __NR_prctl 172
#define __NR_mmap 222
#define __NR_munmap 215
#define __NR_seccomp 383
#define __NR_socket 326
#define __NR_connect 328
#define __NR_bind 329
#define __NR_listen 330
#define __NR_accept 334
#define __NR_dup3 356
#define __NR_fcntl 55
#define __NR_lseek 62
#define __NR_prlimit64 404
#define __NR_perf_event_open 444
#define __NR_ioctl 54
#define __NR_setns 386
#define __NR_rt_sigaction 174
#define __NR_nanosleep 162
#define __NR_clock_gettime 263
#define __NR_getrandom 384
#define __NR_epoll_create1 413
#define __NR_epoll_ctl 414
#define __NR_epoll_pwait 415
#define __NR_sched_setaffinity 345
#define __NR_pidfd_open 434
#define __NR_process_madvise 440
#define __NR_landlock_create_ruleset 444
#define __NR_landlock_add_rule 445
#define __NR_landlock_restrict_self 446
#else
#define __NR_openat 56
#define __NR_read 63
#define __NR_write 64
#define __NR_close 57
#define __NR_access 48
#define __NR_getdents64 217
#define __NR_fork 57
#define __NR_kill 130
#define __NR_getpid 172
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
#define __NR_setns 308
#define __NR_rt_sigaction 13
#define __NR_nanosleep 35
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
#endif

#define SECCOMP_SET_MODE_FILTER 1
#define SECCOMP_FILTER_FLAG_TSYNC (1 << 0)
#define SECCOMP_FILTER_FLAG_LOG (1 << 1)
#define SECCOMP_RET_KILL_PROCESS 0x80000000U
#define SECCOMP_RET_ALLOW 0x7fff0000U

#define PR_SET_NO_NEW_PRIVS 38
#define PR_GET_NO_NEW_PRIVS 39
#define PR_SET_SECCOMP 22
#define PR_CAPBSET_READ 23
#define PR_CAPBSET_DROP 24

#define PERF_TYPE_SOFTWARE 1
#define PERF_COUNT_SW_DUMMY 9
#define PERF_FLAG_FD_CLOEXEC (1UL << 3)

#define LANDLOCK_CREATE_RULESET_VERSION 1

#ifndef __NR_syscalls
#define __NR_syscalls 500
#endif

#endif

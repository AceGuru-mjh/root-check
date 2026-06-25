.syntax unified
.arch armv7-a
.fpu softvfp
.arm

.text
.align 2

#define SYS_OPENAT 322
#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_CLOSE 57
#define SYS_GETDENTS64 61
#define SYS_LSEEK 62
#define SYS_MMAP 192
#define SYS_MUNMAP 91
#define SYS_FSTATAT 79
#define SYS_READLINKAT 78
#define SYS_SCHED_SETAFFINITY 123
#define SYS_CLOCK_GETTIME 113
#define SYS_FACCESSAT 307
#define SYS_GETPID 172
#define SYS_GETTID 178
#define SYS_CLONE 120
#define SYS_EXIT 1
#define SYS_SCHED_YIELD 158

.global bare_syscall_arm
.type bare_syscall_arm, %function
bare_syscall_arm:
    push {r4, r5, r6, r7}
    mov r7, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3
    ldr r3, [sp, #16]
    ldr r4, [sp, #20]
    ldr r5, [sp, #24]
    ldr r6, [sp, #28]
    svc #0
    pop {r4, r5, r6, r7}
    bx lr
.size bare_syscall_arm, .-bare_syscall_arm

.global bare_openat_arm
.type bare_openat_arm, %function
bare_openat_arm:
    push {r7}
    mov r7, #SYS_OPENAT
    svc #0
    pop {r7}
    bx lr
.size bare_openat_arm, .-bare_openat_arm

.global bare_read_arm
.type bare_read_arm, %function
bare_read_arm:
    push {r7}
    mov r7, #SYS_READ
    svc #0
    pop {r7}
    bx lr
.size bare_read_arm, .-bare_read_arm

.global bare_write_arm
.type bare_write_arm, %function
bare_write_arm:
    push {r7}
    mov r7, #SYS_WRITE
    svc #0
    pop {r7}
    bx lr
.size bare_write_arm, .-bare_write_arm

.global bare_close_arm
.type bare_close_arm, %function
bare_close_arm:
    push {r7}
    mov r7, #SYS_CLOSE
    svc #0
    pop {r7}
    bx lr
.size bare_close_arm, .-bare_close_arm

.global bare_getpid_arm
.type bare_getpid_arm, %function
bare_getpid_arm:
    push {r7}
    mov r7, #SYS_GETPID
    svc #0
    pop {r7}
    bx lr
.size bare_getpid_arm, .-bare_getpid_arm

.global bare_gettid_arm
.type bare_gettid_arm, %function
bare_gettid_arm:
    push {r7}
    mov r7, #SYS_GETTID
    svc #0
    pop {r7}
    bx lr
.size bare_gettid_arm, .-bare_gettid_arm

.global bare_fork_arm
.type bare_fork_arm, %function
bare_fork_arm:
    push {r7}
    mov r7, #SYS_CLONE
    mov r0, #0
    mov r1, #0
    svc #0
    pop {r7}
    bx lr
.size bare_fork_arm, .-bare_fork_arm

.global bare_exit_arm
.type bare_exit_arm, %function
bare_exit_arm:
    push {r7}
    mov r7, #SYS_EXIT
    svc #0
    pop {r7}
    bx lr
.size bare_exit_arm, .-bare_exit_arm

.global bare_sched_yield_arm
.type bare_sched_yield_arm, %function
bare_sched_yield_arm:
    push {r7}
    mov r7, #SYS_SCHED_YIELD
    svc #0
    pop {r7}
    bx lr
.size bare_sched_yield_arm, .-bare_sched_yield_arm

.section .note.GNU-stack, "", %progbits

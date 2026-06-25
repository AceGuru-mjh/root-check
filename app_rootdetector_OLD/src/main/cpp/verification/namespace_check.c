/*
 * ====================================================================
 * 多进程命名空间对比检测验证程序 v2.0
 *
 * 编译:
 *   aarch64-linux-android29-clang namespace_check.c -o namespace_check
 *
 * 用法:
 *   ./namespace_check [选项]
 *
 * 选项:
 *   --compare-mount    对比 /proc/self/mountinfo (默认开启)
 *   --compare-fd       对比 /proc/self/fd 目录内容
 *   --compare-environ  对比 /proc/self/environ 环境变量
 *   --compare-status   对比 /proc/self/status 进程状态
 *   --dual-guard       启动双进程守护检测
 *   --json             JSON 输出
 *   --output-dir DIR   临时文件目录 (默认 /data/local/tmp)
 *   --verbose          详细输出
 *
 * 返回码: 0=无异常, 1=检测到命名空间隔离, 2=检测到挂载隐藏
 *
 * 核心原理:
 *   Magisk/KernelSU/APatch 通过 mount namespace 隔离隐藏 Root 痕迹。
 *   子进程 fork 后在新 namespace 中看到的 /proc 与父进程不同。
 *   本程序通过 fork 子进程读取关键系统文件，对比父子进程差异
 *   来检测是否存在命名空间隔离导致的隐藏。
 *
 * 扩展检测:
 *   1. /proc/self/mountinfo - 挂载信息对比 (核心指标)
 *   2. /proc/self/fd        - 文件描述符对比
 *   3. /proc/self/environ   - 环境变量对比 (检测Zygisk注入)
 *   4. /proc/self/status    - 进程状态对比
 *
 * 双进程守护检测:
 *   两个子进程互相监控，防止检测进程被 Root 工具杀死。
 *   任一子进程若检测到对方被终止，或对方被挂入隔离 namespace，
 *   则视为检测到攻击。
 * ====================================================================
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sched.h>

/* ==================== 配置 ==================== */
#define MAX_FILE_SIZE     (256 * 1024)
#define OUTPUT_DIR        "/data/local/tmp"
#define SHM_NAME          "/namespace_guard"
#define GUARD_TIMEOUT_MS  5000

/* ==================== 全局状态 ==================== */
static volatile int g_running = 1;

/* ==================== 文件读取工具 ==================== */

static char *read_file(const char *path, size_t *out_len) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return NULL;

    char *buf = (char *)malloc(MAX_FILE_SIZE);
    if (!buf) { close(fd); return NULL; }

    ssize_t n = read(fd, buf, MAX_FILE_SIZE - 1);
    close(fd);
    if (n <= 0) { free(buf); return NULL; }

    buf[n] = '\0';
    if (out_len) *out_len = (size_t)n;
    return buf;
}

static int read_dir_entries(const char *path, char *out, size_t out_size) {
    DIR *dir = opendir(path);
    if (!dir) {
        snprintf(out, out_size, "[unreadable: %s]", strerror(errno));
        return -1;
    }

    size_t pos = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && pos < out_size - 64) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        pos += (size_t)snprintf(out + pos, out_size - pos, "%s/", entry->d_name);
    }
    closedir(dir);
    out[pos] = '\0';
    return 0;
}

/* ==================== 核心对比函数 ==================== */

/* 对比挂载信息 */
static int compare_mountinfo(const char *tmp_dir, char *detail, size_t detail_size) {
    /* 父进程读取 */
    size_t parent_len;
    char *parent_data = read_file("/proc/self/mountinfo", &parent_len);
    if (!parent_data) {
        snprintf(detail, detail_size, "Cannot read parent mountinfo");
        return -1;
    }

    /* 写入临时文件供子进程读取 */
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s/ns_parent_mount_%d", tmp_dir, getpid());
    int wfd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (wfd < 0) { free(parent_data); return -1; }
    write(wfd, parent_data, parent_len);
    close(wfd);
    free(parent_data);

    /* fork 子进程 */
    pid_t pid = fork();
    if (pid < 0) {
        unlink(tmp_path);
        return -1;
    }

    if (pid == 0) {
        /* 子进程: 读取自己的 mountinfo 写入另一个临时文件 */
        size_t child_len;
        char *child_data = read_file("/proc/self/mountinfo", &child_len);
        if (child_data) {
            char child_path[256];
            snprintf(child_path, sizeof(child_path), "%s/ns_child_mount_%d", tmp_dir, getpid());
            int cfd = open(child_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (cfd >= 0) {
                write(cfd, child_data, child_len);
                close(cfd);
            }
            free(child_data);
        }
        _exit(0);
    }

    /* 父进程: 等待子进程完成并比较 */
    int status;
    waitpid(pid, &status, 0);

    char child_path[256];
    snprintf(child_path, sizeof(child_path), "%s/ns_child_mount_%d", tmp_dir, getpid());

    size_t child_data_len;
    char *child_data = read_file(child_path, &child_data_len);
    unlink(tmp_path);
    unlink(child_path);

    if (!child_data) {
        snprintf(detail, detail_size, "Cannot read child mountinfo");
        return -1;
    }

    /* 重新读取父进程 mountinfo (确保更新) */
    char *parent_data2 = read_file("/proc/self/mountinfo", &parent_len);
    if (!parent_data2) {
        free(child_data);
        snprintf(detail, detail_size, "Cannot re-read parent mountinfo");
        return -1;
    }

    int diff_count = 0;
    if (strcmp(parent_data2, child_data) != 0) {
        /* 逐行对比以定位差异 */
        char *parent_save, *child_save;
        char *parent_line = strtok_r(parent_data2, "\n", &parent_save);
        char *child_line  = strtok_r(child_data, "\n", &child_save);
        int line_no = 1;

        while (parent_line || child_line) {
            if (!parent_line && child_line) {
                snprintf(detail + strlen(detail), detail_size - strlen(detail),
                         "Mount diff L%d: [child only] %s\n", line_no, child_line);
                diff_count++;
            } else if (parent_line && !child_line) {
                snprintf(detail + strlen(detail), detail_size - strlen(detail),
                         "Mount diff L%d: [parent only] %s\n", line_no, parent_line);
                diff_count++;
            } else if (strcmp(parent_line, child_line) != 0) {
                snprintf(detail + strlen(detail), detail_size - strlen(detail),
                         "Mount diff L%d:\n  parent: %s\n  child:  %s\n",
                         line_no, parent_line, child_line);
                diff_count++;
            }
            parent_line = strtok_r(NULL, "\n", &parent_save);
            child_line  = strtok_r(NULL, "\n", &child_save);
            line_no++;
        }
    }

    free(parent_data2);
    free(child_data);
    return diff_count;
}

/* 对比 /proc/self/fd 目录 */
static int compare_fd(const char *tmp_dir, char *detail, size_t detail_size) {
    char parent_fd[8192];
    if (read_dir_entries("/proc/self/fd", parent_fd, sizeof(parent_fd)) != 0) {
        snprintf(detail, detail_size, "Cannot read parent fd");
        return -1;
    }

    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s/ns_parent_fd_%d", tmp_dir, getpid());
    int wfd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (wfd < 0) return -1;
    write(wfd, parent_fd, strlen(parent_fd));
    close(wfd);

    pid_t pid = fork();
    if (pid < 0) { unlink(tmp_path); return -1; }

    if (pid == 0) {
        char child_fd[8192];
        read_dir_entries("/proc/self/fd", child_fd, sizeof(child_fd));
        char child_path[256];
        snprintf(child_path, sizeof(child_path), "%s/ns_child_fd_%d", tmp_dir, getpid());
        int cfd = open(child_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (cfd >= 0) { write(cfd, child_fd, strlen(child_fd)); close(cfd); }
        _exit(0);
    }

    waitpid(pid, NULL, 0);
    char child_path[256];
    snprintf(child_path, sizeof(child_path), "%s/ns_child_fd_%d", tmp_dir, getpid());

    char child_fd[8192];
    int cfd = open(child_path, O_RDONLY);
    ssize_t n = cfd >= 0 ? read(cfd, child_fd, sizeof(child_fd) - 1) : -1;
    if (cfd >= 0) close(cfd);
    unlink(tmp_path);
    unlink(child_path);

    if (n <= 0) { snprintf(detail, detail_size, "Cannot read child fd"); return -1; }
    child_fd[n] = '\0';

    if (strcmp(parent_fd, child_fd) != 0) {
        snprintf(detail + strlen(detail), detail_size - strlen(detail),
                 "FD diff:\n  parent: %s\n  child:  %s\n", parent_fd, child_fd);
        return 1;
    }
    return 0;
}

/* 对比环境变量 */
static int compare_environ(const char *tmp_dir, char *detail, size_t detail_size) {
    size_t parent_len;
    char *parent_data = read_file("/proc/self/environ", &parent_len);
    if (!parent_data) return -1;

    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s/ns_parent_env_%d", tmp_dir, getpid());
    int wfd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (wfd < 0) { free(parent_data); return -1; }
    write(wfd, parent_data, parent_len);
    close(wfd);
    free(parent_data);

    pid_t pid = fork();
    if (pid < 0) { unlink(tmp_path); return -1; }

    if (pid == 0) {
        size_t child_len;
        char *child_data = read_file("/proc/self/environ", &child_len);
        if (child_data) {
            char child_path[256];
            snprintf(child_path, sizeof(child_path), "%s/ns_child_env_%d", tmp_dir, getpid());
            int cfd = open(child_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (cfd >= 0) { write(cfd, child_data, child_len); close(cfd); }
            free(child_data);
        }
        _exit(0);
    }

    waitpid(pid, NULL, 0);
    char child_path[256];
    snprintf(child_path, sizeof(child_path), "%s/ns_child_env_%d", tmp_dir, getpid());

    size_t child_len;
    char *child_data = read_file(child_path, &child_len);
    unlink(tmp_path);
    unlink(child_path);

    if (!child_data) return -1;

    /* 按 \0 分割对比每个环境变量 */
    int diff_count = 0;
    char *parent_cur = read_file("/proc/self/environ", &parent_len);
    if (!parent_cur) { free(child_data); return -1; }

    char *p = parent_cur;
    char *c = child_data;
    size_t p_remain = parent_len;
    size_t c_remain = child_len;

    while (p_remain > 0 && c_remain > 0) {
        size_t p_len = strnlen(p, p_remain);
        size_t c_len = strnlen(c, c_remain);

        if (p_len != c_len || memcmp(p, c, p_len) != 0) {
            snprintf(detail + strlen(detail), detail_size - strlen(detail),
                     "Environ diff:\n  parent: %.*s\n  child:  %.*s\n",
                     (int)p_len, p, (int)c_len, c);
            diff_count++;
        }
        p += p_len + 1;
        c += c_len + 1;
        if (p_remain > p_len + 1) p_remain -= p_len + 1; else break;
        if (c_remain > c_len + 1) c_remain -= c_len + 1; else break;
    }

    free(parent_cur);
    free(child_data);
    return diff_count;
}

/* 对比进程状态 */
static int compare_status(const char *tmp_dir, char *detail, size_t detail_size) {
    /* State/TracerPid/Gid 等关键字段 */
    size_t parent_len;
    char *parent_data = read_file("/proc/self/status", &parent_len);
    if (!parent_data) return -1;

    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s/ns_parent_st_%d", tmp_dir, getpid());
    int wfd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (wfd < 0) { free(parent_data); return -1; }
    write(wfd, parent_data, parent_len);
    close(wfd);
    free(parent_data);

    pid_t pid = fork();
    if (pid < 0) { unlink(tmp_path); return -1; }

    if (pid == 0) {
        size_t child_len;
        char *child_data = read_file("/proc/self/status", &child_len);
        if (child_data) {
            char child_path[256];
            snprintf(child_path, sizeof(child_path), "%s/ns_child_st_%d", tmp_dir, getpid());
            int cfd = open(child_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (cfd >= 0) { write(cfd, child_data, child_len); close(cfd); }
            free(child_data);
        }
        _exit(0);
    }

    waitpid(pid, NULL, 0);
    char child_path[256];
    snprintf(child_path, sizeof(child_path), "%s/ns_child_st_%d", tmp_dir, getpid());

    size_t child_len;
    char *child_data = read_file(child_path, &child_len);
    unlink(tmp_path);
    unlink(child_path);

    if (!child_data) return -1;

    char *parent_cur = read_file("/proc/self/status", &parent_len);
    if (!parent_cur) { free(child_data); return -1; }

    int diff_count = 0;
    const char *key_fields[] = {"State:", "TracerPid:", "Uid:", "Gid:",
                                "Groups:", "CapEff:", "CapPrm:", "Seccomp:", NULL};

    for (const char **k = key_fields; *k; k++) {
        char *p_line = strstr(parent_cur, *k);
        char *c_line = strstr(child_data, *k);
        if (p_line && c_line) {
            char *p_end = strchr(p_line, '\n');
            char *c_end = strchr(c_line, '\n');
            if (p_end && c_end) {
                size_t p_len = (size_t)(p_end - p_line);
                size_t c_len = (size_t)(c_end - c_line);
                if (p_len != c_len || memcmp(p_line, c_line, p_len) != 0) {
                    snprintf(detail + strlen(detail), detail_size - strlen(detail),
                             "Status diff: %.*s\n  parent: %.*s\n  child:  %.*s\n",
                             (int)(c_end - c_line), c_line,
                             (int)p_len, p_line, (int)c_len, c_line);
                    diff_count++;
                }
            }
        }
    }

    free(parent_cur);
    free(child_data);
    return diff_count;
}

/* ==================== 双进程守护检测 ==================== */

typedef struct {
    pid_t        pid1;
    pid_t        pid2;
    int          detected;
    volatile int *alive1;
    volatile int *alive2;
} guard_state_t;

static void guardian_alarm(int sig) {
    (void)sig;
    g_running = 0;
}

static int dual_guardian_detect(void) {
    /* 共享内存: 两个进程互相报告存活状态 */
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd < 0) {
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0600);
        if (shm_fd < 0) return -1;
    }
    ftruncate(shm_fd, 4096);

    volatile int *shared = (volatile int *)mmap(
        NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) { close(shm_fd); return -1; }

    shared[0] = 0; /* guard1 心跳 */
    shared[1] = 0; /* guard2 心跳 */
    shared[2] = 0; /* 检测计数 */
    close(shm_fd);

    signal(SIGALRM, guardian_alarm);

    pid_t pid1 = fork();
    if (pid1 < 0) { munmap((void *)shared, 4096); return -1; }

    if (pid1 == 0) {
        /* Guard 1: 监控 Guard 2 */
        int my_id = 1;
        int detected = 0;
        alarm(GUARD_TIMEOUT_MS / 1000 + 1);

        while (g_running) {
            shared[0] = shared[0] + 1;  /* 报告存活 */
            usleep(100000); /* 100ms */

            if (shared[2] > 0) {  /* 任何一方检测到异常 */
                detected = 1;
                break;
            }
        }

        if (detected) {
            fprintf(stderr, "[GUARD1] Namespace isolation detected\n");
            _exit(3);
        }
        _exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        kill(pid1, SIGKILL);
        waitpid(pid1, NULL, 0);
        munmap((void *)shared, 4096);
        return -1;
    }

    if (pid2 == 0) {
        /* Guard 2: 监控 Guard 1 */
        int my_id = 2;
        int detected = 0;
        alarm(GUARD_TIMEOUT_MS / 1000 + 1);

        while (g_running) {
            shared[1] = shared[1] + 1;
            usleep(100000);

            /* 检查对方的命名空间是否与自己不同 */
            pid_t ppid = getppid();
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/ns/mnt", (int)ppid);
            struct stat st1, st2;
            if (stat(path, &st1) == 0 && stat("/proc/self/ns/mnt", &st2) == 0) {
                if (st1.st_ino != st2.st_ino) {
                    shared[2] = 1;  /* 通知检测到隔离 */
                    detected = 1;
                    break;
                }
            }
            if (shared[2] > 0) { detected = 1; break; }
        }

        if (detected) {
            fprintf(stderr, "[GUARD2] Namespace isolation detected\n");
            _exit(3);
        }
        _exit(0);
    }

    /* 父进程: 等待一段时间后检查守护进程状态 */
    usleep(GUARD_TIMEOUT_MS * 1000);

    int status1, status2;
    waitpid(pid1, &status1, WNOHANG);
    waitpid(pid2, &status2, WNOHANG);

    int detected = 0;
    if (WIFEXITED(status1) && WEXITSTATUS(status1) == 3) detected = 1;
    if (WIFEXITED(status2) && WEXITSTATUS(status2) == 3) detected = 1;

    /* 检查守护进程是否被杀死 */
    int guard1_dead = (kill(pid1, 0) != 0);
    int guard2_dead = (kill(pid2, 0) != 0);
    if (guard1_dead || guard2_dead) {
        fprintf(stderr, "[GUARD] Guardian process was killed!\n");
        detected = 1;
    }

    /* 清理 */
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    munmap((void *)shared, 4096);
    shm_unlink(SHM_NAME);

    return detected;
}

/* ==================== 主程序 ==================== */

int main(int argc, char *argv[]) {
    int do_mount   = 1;
    int do_fd      = 1;
    int do_environ = 1;
    int do_status  = 1;
    int do_guard   = 1;
    int json_out   = 0;
    int verbose    = 0;
    const char *output_dir = OUTPUT_DIR;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--compare-mount") == 0)   do_mount   = 1;
        else if (strcmp(argv[i], "--no-mount") == 0)        do_mount   = 0;
        else if (strcmp(argv[i], "--compare-fd") == 0)      do_fd      = 1;
        else if (strcmp(argv[i], "--no-fd") == 0)           do_fd      = 0;
        else if (strcmp(argv[i], "--compare-environ") == 0) do_environ = 1;
        else if (strcmp(argv[i], "--no-environ") == 0)      do_environ = 0;
        else if (strcmp(argv[i], "--compare-status") == 0)  do_status  = 1;
        else if (strcmp(argv[i], "--no-status") == 0)       do_status  = 0;
        else if (strcmp(argv[i], "--dual-guard") == 0)      do_guard   = 1;
        else if (strcmp(argv[i], "--no-guard") == 0)        do_guard   = 0;
        else if (strcmp(argv[i], "--json") == 0)            json_out   = 1;
        else if (strcmp(argv[i], "--verbose") == 0)         verbose    = 1;
        else if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc)
            output_dir = argv[++i];
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Multi-Process Namespace Check v2.0\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --compare-mount   compare mountinfo (default: on)\n");
            printf("  --no-mount        disable mountinfo comparison\n");
            printf("  --compare-fd      compare fd (default: on)\n");
            printf("  --compare-environ compare environ (default: on)\n");
            printf("  --compare-status  compare status (default: on)\n");
            printf("  --dual-guard      dual-process guardian (default: on)\n");
            printf("  --json            JSON output\n");
            printf("  --output-dir DIR  temp directory (default /data/local/tmp)\n");
            printf("  --verbose         verbose output\n");
            printf("  --help            this help\n");
            return 0;
        }
    }

    if (!json_out)
        fprintf(stderr, "=== Multi-Process Namespace Check v2.0 ===\n");

    int total_score = 0;
    char detail[16384];
    detail[0] = '\0';

    /* 确保输出目录存在 */
    mkdir(output_dir, 0700);

    /* 1. 挂载信息对比 */
    if (do_mount) {
        if (!json_out)
            fprintf(stderr, "[CHECK] Comparing mountinfo...\n");
        int n = compare_mountinfo(output_dir, detail, sizeof(detail));
        if (n > 0) {
            total_score += n;
            if (!json_out) printf(">>> Mount namespace: %d differences found\n", n);
        } else if (!json_out) {
            printf(">>> Mount namespace: clean\n");
        }
    }

    /* 2. FD 对比 */
    if (do_fd) {
        if (!json_out)
            fprintf(stderr, "[CHECK] Comparing fd...\n");
        int n = compare_fd(output_dir, detail, sizeof(detail));
        if (n > 0) {
            total_score += n;
            if (!json_out) printf(">>> FD entries: %d differences found\n", n);
        } else if (!json_out) {
            printf(">>> FD entries: clean\n");
        }
    }

    /* 3. 环境变量对比 */
    if (do_environ) {
        if (!json_out)
            fprintf(stderr, "[CHECK] Comparing environ...\n");
        int n = compare_environ(output_dir, detail, sizeof(detail));
        if (n > 0) {
            total_score += n;
            if (!json_out) printf(">>> Environ: %d differences found\n", n);
        } else if (!json_out) {
            printf(">>> Environ: clean\n");
        }
    }

    /* 4. 进程状态对比 */
    if (do_status) {
        if (!json_out)
            fprintf(stderr, "[CHECK] Comparing status...\n");
        int n = compare_status(output_dir, detail, sizeof(detail));
        if (n > 0) {
            total_score += n;
            if (!json_out) printf(">>> Status: %d differences found\n", n);
        } else if (!json_out) {
            printf(">>> Status: clean\n");
        }
    }

    /* 5. 双进程守护检测 */
    if (do_guard) {
        if (!json_out)
            fprintf(stderr, "[CHECK] Dual-process guardian...\n");
        int n = dual_guardian_detect();
        if (n > 0) {
            total_score += 10;
            if (!json_out)
                printf(">>> Dual guardian: namespace isolation detected!\n");
        } else if (!json_out) {
            printf(">>> Dual guardian: clean\n");
        }
    }

    /* 输出 */
    if (json_out) {
        printf("{\n");
        printf("  \"tool\": \"namespace_check\",\n");
        printf("  \"detected\": %s,\n", total_score > 0 ? "true" : "false");
        printf("  \"total_score\": %d,\n", total_score);
        printf("  \"detail\": \"%s\"\n", detail);
        printf("}\n");
    } else {
        if (total_score > 0) {
            printf("\n=== NAMESPACE ISOLATION DETECTED (score=%d) ===\n", total_score);
            if (verbose) printf("Details:\n%s\n", detail);
        } else {
            printf("\n=== No namespace isolation detected ===\n");
        }
    }

    return total_score > 0 ? 1 : 0;
}

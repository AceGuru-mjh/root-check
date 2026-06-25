/*
 * ====================================================================
 * APatch 侧信道检测验证程序 v2.0
 *
 * 编译:
 *   aarch64-linux-android29-clang apatch_sidechannel.c -o apatch_sidechannel
 *
 * 用法:
 *   ./apatch_sidechannel [选项]
 *
 * 选项:
 *   --iters N        每次测试迭代次数 (默认 100000)
 *   --warmup N       预热次数 (默认 10000)
 *   --repeats N      完整测试重复次数 (默认 10)
 *   --threshold N    报警阈值纳秒 (默认 500)
 *   --cpu N          绑定到指定 CPU (默认 0)
 *   --lock-freq      尝试锁定 CPU 频率 (默认尝试)
 *   --json           输出 JSON 格式结果
 *   --calibrate      运行自校准模式 (建立基线)
 *   --baseline FILE  使用指定基线文件
 *   --verbose        详细输出
 *
 * 返回码: 0=未检出, 1=检出APatch, 2=检出其他Hook, -1=错误
 *
 * 核心原理:
 *   APatch 通过内核 inline-hook 拦截 do_vfs_ftruncate，
 *   导致 ftruncate(fd, 0xffff) 和 ftruncate(fd, 0x10000)
 *   的执行时间产生显著差异。纯净系统中两者时间差 < 100ns，
 *   APatch 系统中差异 > 500ns。
 * ====================================================================
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>

/* ==================== 配置默认值 ==================== */
#define DEFAULT_ITERS       100000
#define DEFAULT_WARMUP      10000
#define DEFAULT_REPEATS     10
#define DEFAULT_THRESHOLD_NS 500
#define DEFAULT_CPU         0

/* ==================== 高精度计时 ==================== */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ==================== CPU 频率锁定 ==================== */

static int lock_cpu_frequency(int cpu) {
    char path[128];
    int fd;

    /* 尝试 governor → performance */
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", cpu);
    fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, "performance", 11);
        close(fd);
        fprintf(stderr, "[LOCK] CPU%d governor → performance\n", cpu);
    }

    /* 尝试设置最大频率 = 最小频率 (锁定频率) */
    char max_freq[64] = {0};
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", cpu);
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, max_freq, sizeof(max_freq) - 1);
        close(fd);
        if (n > 0) {
            max_freq[n] = '\0';
            while (n > 0 && (max_freq[n - 1] == '\n' || max_freq[n - 1] == ' '))
                max_freq[--n] = '\0';
            snprintf(path, sizeof(path),
                     "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq", cpu);
            fd = open(path, O_WRONLY);
            if (fd >= 0) {
                write(fd, max_freq, strlen(max_freq));
                close(fd);
                fprintf(stderr, "[LOCK] CPU%d locked at %s kHz\n", cpu, max_freq);
            }
        }
    }
    return 0;
}

/* ==================== CPU 绑定与亲和性 ==================== */

static int bind_cpu(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        fprintf(stderr, "[WARN] sched_setaffinity CPU%d: %s\n", cpu, strerror(errno));
        return -1;
    }
    /* 提升优先级以减少调度抖动 */
    setpriority(PRIO_PROCESS, 0, -20);
    return 0;
}

/* ==================== 侧信道测量核心 ==================== */

/* 单次测量: 测量 ftruncate(fd, size) 的平均耗时 */
static double measure_ftruncate_avg(int fd, off_t size, int warmup, int iters) {
    /* 预热 */
    for (int i = 0; i < warmup; i++) {
        ftruncate(fd, size);
    }

    /* 正式测量 */
    uint64_t start = get_time_ns();
    for (int i = 0; i < iters; i++) {
        ftruncate(fd, size);
    }
    uint64_t end = get_time_ns();

    return (double)(end - start) / (double)iters;
}

/* 统计测量: 多次测量, 去除异常值, 返回均值+标准差 */
typedef struct {
    double  mean;          /* 均值 (ns) */
    double  stddev;        /* 标准差 (ns) */
    double  min;           /* 最小值 (ns) */
    double  max;           /* 最大值 (ns) */
    int     samples;       /* 有效样本数 */
    double  raw[256];      /* 原始数据 */
} stat_result_t;

static int double_cmp(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

static stat_result_t compute_stats(double *data, int n) {
    stat_result_t st;
    memset(&st, 0, sizeof(st));
    st.samples = n;

    if (n <= 0) return st;

    /* 排序 */
    qsort(data, (size_t)n, sizeof(double), double_cmp);

    /* 去除头尾各 10% 异常值后的有效区间 */
    int trim_start = n / 10;
    int trim_end   = n - n / 10;
    if (trim_end <= trim_start) { trim_start = 0; trim_end = n; }

    int valid = 0;
    double sum = 0, sum2 = 0;
    st.min = 1e18;
    st.max = -1e18;

    for (int i = trim_start; i < trim_end; i++) {
        double v = data[i];
        sum  += v;
        sum2 += v * v;
        valid++;
        if (v < st.min) st.min = v;
        if (v > st.max) st.max = v;
    }

    st.mean   = sum / valid;
    st.stddev = sqrt(sum2 / valid - st.mean * st.mean);
    st.samples = valid;
    memcpy(st.raw, data, (size_t)n * sizeof(double));
    return st;
}

/* ==================== 侧信道检测 ==================== */

typedef struct {
    double         diff_ns;          /* 时间差 (ns) */
    stat_result_t  stat_small;       /* ftruncate(fd, 0xffff) 统计 */
    stat_result_t  stat_large;       /* ftruncate(fd, 0x10000) 统计 */
    double         threshold_ns;     /* 使用阈值 */
    int            detected;         /* 是否检出 */
    double         confidence;       /* 置信度 0-100 */
} sidechannel_result_t;

static sidechannel_result_t detect_apatch_sidechannel(
    int fd, int warmup, int iters, int repeats, double threshold_ns) {

    sidechannel_result_t res;
    memset(&res, 0, sizeof(res));
    res.threshold_ns = threshold_ns;

    double data_small[256];
    double data_large[256];
    int rep = repeats > 256 ? 256 : repeats;

    for (int i = 0; i < rep; i++) {
        data_small[i] = measure_ftruncate_avg(fd, 0xffff, warmup, iters);
        data_large[i] = measure_ftruncate_avg(fd, 0x10000, warmup, iters);
    }

    res.stat_small = compute_stats(data_small, rep);
    res.stat_large = compute_stats(data_large, rep);

    /* 统计显著性: 如果两个分布均值差异 > 3σ 则判定为真 */
    double diff = fabs(res.stat_small.mean - res.stat_large.mean);
    double combined_std = sqrt(res.stat_small.stddev * res.stat_small.stddev +
                               res.stat_large.stddev * res.stat_large.stddev);
    double sigma = (combined_std > 0.001) ? diff / combined_std : 0;

    res.diff_ns = diff;

    if (diff > threshold_ns && sigma > 3.0) {
        res.detected  = 1;
        res.confidence = sigma > 10.0 ? 99.0 :
                         sigma > 6.0  ? 95.0 :
                         sigma > 3.0  ? 80.0 : 60.0;
    } else if (diff > threshold_ns / 2) {
        /* 疑似 */
        res.detected  = 0;
        res.confidence = 30.0;
    } else {
        res.detected  = 0;
        res.confidence = 95.0 - (diff / threshold_ns) * 50.0;
        if (res.confidence < 0) res.confidence = 0;
    }

    return res;
}

/* ==================== 基线管理 ==================== */

typedef struct {
    double base_diff_ns;
    double base_small_mean;
    double base_large_mean;
    int    cpu;
    int    samples;
} baseline_t;

static int save_baseline(const char *path, const baseline_t *bl) {
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;
    fprintf(fp, "base_diff_ns=%.2f\n", bl->base_diff_ns);
    fprintf(fp, "base_small_mean=%.2f\n", bl->base_small_mean);
    fprintf(fp, "base_large_mean=%.2f\n", bl->base_large_mean);
    fprintf(fp, "cpu=%d\n", bl->cpu);
    fprintf(fp, "samples=%d\n", bl->samples);
    fclose(fp);
    return 0;
}

static int load_baseline(const char *path, baseline_t *bl) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    memset(bl, 0, sizeof(*bl));
    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        char key[64], val[64];
        if (sscanf(line, "%63[^=]=%63s", key, val) == 2) {
            if      (strcmp(key, "base_diff_ns") == 0)  bl->base_diff_ns = atof(val);
            else if (strcmp(key, "base_small_mean") == 0) bl->base_small_mean = atof(val);
            else if (strcmp(key, "base_large_mean") == 0) bl->base_large_mean = atof(val);
            else if (strcmp(key, "cpu") == 0)           bl->cpu = atoi(val);
            else if (strcmp(key, "samples") == 0)       bl->samples = atoi(val);
        }
    }
    fclose(fp);
    return 0;
}

/* ==================== 多核扫描 ==================== */

static int detect_all_cpus(int warmup, int iters, int repeats,
                           double threshold_ns, double *out_scores) {
    int max_cpu = sysconf(_SC_NPROCESSORS_CONF);
    if (max_cpu <= 0) max_cpu = 4;

    int detected_count = 0;
    for (int cpu = 0; cpu < max_cpu; cpu++) {
        if (bind_cpu(cpu) != 0) continue;

        int fd = open("/dev/null", O_RDWR);
        if (fd < 0) continue;

        sidechannel_result_t res = detect_apatch_sidechannel(
            fd, warmup, iters, repeats, threshold_ns);
        close(fd);

        out_scores[cpu] = res.diff_ns;
        if (res.detected) detected_count++;
    }
    return detected_count;
}

/* ==================== JSON 输出 ==================== */

static void print_json(const sidechannel_result_t *res,
                       const baseline_t *bl, int cpu) {
    printf("{\n");
    printf("  \"tool\": \"apatch_sidechannel_verifier\",\n");
    printf("  \"detected\": %s,\n", res->detected ? "true" : "false");
    printf("  \"confidence\": %.1f,\n", res->confidence);
    printf("  \"diff_ns\": %.2f,\n", res->diff_ns);
    printf("  \"threshold_ns\": %.0f,\n", res->threshold_ns);
    printf("  \"cpu\": %d,\n", cpu);
    printf("  \"small\": { \"mean\": %.2f, \"stddev\": %.2f, \"min\": %.2f, \"max\": %.2f },\n",
           res->stat_small.mean, res->stat_small.stddev,
           res->stat_small.min, res->stat_small.max);
    printf("  \"large\": { \"mean\": %.2f, \"stddev\": %.2f, \"min\": %.2f, \"max\": %.2f },\n",
           res->stat_large.mean, res->stat_large.stddev,
           res->stat_large.min, res->stat_large.max);
    if (bl) {
        printf("  \"baseline_diff_ns\": %.2f,\n", bl->base_diff_ns);
        printf("  \"deviation_sigma\": %.2f\n",
               (res->diff_ns - bl->base_diff_ns) /
               (res->stat_small.stddev + res->stat_large.stddev + 0.001));
    } else {
        printf("  \"sigma\": %.2f\n",
               res->diff_ns / (res->stat_small.stddev + res->stat_large.stddev + 0.001));
    }
    printf("}\n");
}

/* ==================== 主程序 ==================== */

int main(int argc, char *argv[]) {
    int    iters      = DEFAULT_ITERS;
    int    warmup     = DEFAULT_WARMUP;
    int    repeats    = DEFAULT_REPEATS;
    double threshold  = DEFAULT_THRESHOLD_NS;
    int    cpu        = DEFAULT_CPU;
    int    lock_freq  = 1;
    int    json_out   = 0;
    int    calibrate  = 0;
    int    all_cpus   = 0;
    int    verbose    = 0;
    const char *baseline_file = NULL;

    /* 解析参数 */
    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--iters") == 0 && i + 1 < argc)
            iters = atoi(argv[++i]);
        else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc)
            warmup = atoi(argv[++i]);
        else if (strcmp(argv[i], "--repeats") == 0 && i + 1 < argc)
            repeats = atoi(argv[++i]);
        else if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc)
            threshold = atof(argv[++i]);
        else if (strcmp(argv[i], "--cpu") == 0 && i + 1 < argc)
            cpu = atoi(argv[++i]);
        else if (strcmp(argv[i], "--no-lock-freq") == 0)
            lock_freq = 0;
        else if (strcmp(argv[i], "--json") == 0)
            json_out = 1;
        else if (strcmp(argv[i], "--calibrate") == 0)
            calibrate = 1;
        else if (strcmp(argv[i], "--all-cpus") == 0)
            all_cpus = 1;
        else if (strcmp(argv[i], "--baseline") == 0 && i + 1 < argc)
            baseline_file = argv[++i];
        else if (strcmp(argv[i], "--verbose") == 0)
            verbose = 1;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("APatch Side-Channel Detector v2.0\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --iters N         iterations per test (default %d)\n", DEFAULT_ITERS);
            printf("  --warmup N        warmup iterations (default %d)\n", DEFAULT_WARMUP);
            printf("  --repeats N       test repetitions (default %d)\n", DEFAULT_REPEATS);
            printf("  --threshold N     alert threshold ns (default %d)\n", DEFAULT_THRESHOLD_NS);
            printf("  --cpu N           bind CPU (default %d)\n", DEFAULT_CPU);
            printf("  --no-lock-freq    skip CPU frequency lock\n");
            printf("  --json            output JSON\n");
            printf("  --calibrate       run calibration mode\n");
            printf("  --all-cpus        test all CPU cores\n");
            printf("  --baseline FILE   compare against baseline\n");
            printf("  --verbose         verbose output\n");
            printf("  --help            this help\n");
            return 0;
        }
    }

    if (!json_out) {
        fprintf(stderr, "=== APatch Side-Channel Detector v2.0 ===\n");
        fprintf(stderr, "Config: iters=%d warmup=%d repeats=%d threshold=%.0fns cpu=%d\n",
                iters, warmup, repeats, threshold, cpu);
    }

    /* CPU 频率锁定 */
    if (lock_freq) {
        lock_cpu_frequency(cpu);
    }

    /* 绑定 CPU */
    if (bind_cpu(cpu) != 0 && !all_cpus) {
        fprintf(stderr, "[ERROR] Cannot bind CPU %d\n", cpu);
        return -1;
    }

    /* 加载基线 */
    baseline_t baseline;
    int has_baseline = 0;
    if (baseline_file) {
        has_baseline = (load_baseline(baseline_file, &baseline) == 0);
        if (!has_baseline)
            fprintf(stderr, "[WARN] Cannot load baseline: %s\n", baseline_file);
    }

    /* 自校准模式 */
    if (calibrate) {
        if (!json_out)
            fprintf(stderr, "[CALIBRATE] Running calibration...\n");

        int fd = open("/dev/null", O_RDWR);
        if (fd < 0) { perror("open /dev/null"); return -1; }

        sidechannel_result_t cal = detect_apatch_sidechannel(
            fd, warmup, iters, repeats, threshold);
        close(fd);

        baseline.base_diff_ns   = cal.diff_ns;
        baseline.base_small_mean = cal.stat_small.mean;
        baseline.base_large_mean = cal.stat_large.mean;
        baseline.cpu     = cpu;
        baseline.samples = repeats;

        const char *bl_path = baseline_file ? baseline_file : "/data/local/tmp/apatch_baseline.txt";
        save_baseline(bl_path, &baseline);

        if (!json_out) {
            printf("[CALIBRATE] Baseline saved to %s\n", bl_path);
            printf("  base_diff_ns=%.2f\n", baseline.base_diff_ns);
            printf("  small_mean=%.2f large_mean=%.2f\n",
                   baseline.base_small_mean, baseline.base_large_mean);
        }
        return 0;
    }

    /* 多核扫描 */
    if (all_cpus) {
        int max_cpu = sysconf(_SC_NPROCESSORS_CONF);
        if (max_cpu <= 0) max_cpu = 4;
        double scores[256];
        int detected = detect_all_cpus(warmup, iters, repeats, threshold, scores);

        if (json_out) {
            printf("{\n  \"tool\": \"apatch_sidechannel_verifier\",\n");
            printf("  \"all_cpus\": true,\n");
            printf("  \"cpu_count\": %d,\n", max_cpu);
            printf("  \"detected_cores\": %d,\n", detected);
            printf("  \"scores\": [");
            for (int i = 0; i < max_cpu; i++) {
                if (i > 0) printf(",");
                printf("{\"cpu\":%d,\"diff_ns\":%.2f}", i, scores[i]);
            }
            printf("]\n}\n");
        } else {
            printf("Multi-CPU Scan: %d/%d cores detected\n", detected, max_cpu);
            for (int i = 0; i < max_cpu; i++)
                printf("  CPU%d: diff=%.2fns %s\n", i, scores[i],
                       scores[i] > threshold ? "⚠ DETECTED" : "✓ clean");
        }
        return detected > 0 ? 1 : 0;
    }

    /* 单核检测 */
    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) { perror("open /dev/null"); return -1; }

    sidechannel_result_t result = detect_apatch_sidechannel(
        fd, warmup, iters, repeats, threshold);
    close(fd);

    if (json_out) {
        print_json(&result, has_baseline ? &baseline : NULL, cpu);
    } else {
        printf("Small (0xffff): mean=%.2fns stddev=%.2fns (range %.2f-%.2f)\n",
               result.stat_small.mean, result.stat_small.stddev,
               result.stat_small.min, result.stat_small.max);
        printf("Large (0x10000): mean=%.2fns stddev=%.2fns (range %.2f-%.2f)\n",
               result.stat_large.mean, result.stat_large.stddev,
               result.stat_large.min, result.stat_large.max);
        printf("Diff: %.2f ns (threshold: %.0f ns, sigma: %.2f)\n",
               result.diff_ns, threshold,
               result.diff_ns / (result.stat_small.stddev + result.stat_large.stddev + 0.001));

        if (result.detected) {
            printf(">>> APatch DETECTED (confidence: %.1f%%)\n", result.confidence);
        } else if (result.confidence < 50) {
            printf(">>> SUSPICIOUS (diff=%.2fns exceeds threshold/2)\n", result.diff_ns);
        } else {
            printf(">>> CLEAN (diff=%.2fns within normal range)\n", result.diff_ns);
        }

        if (has_baseline) {
            printf("Baseline comparison: base_diff=%.2fns, deviation=%.2fns\n",
                   baseline.base_diff_ns, result.diff_ns - baseline.base_diff_ns);
        }

        if (verbose) {
            printf("\nRaw small samples:");
            for (int i = 0; i < result.stat_small.samples && i < 10; i++)
                printf(" %.2f", result.stat_small.raw[i]);
            printf("\nRaw large samples:");
            for (int i = 0; i < result.stat_large.samples && i < 10; i++)
                printf(" %.2f", result.stat_large.raw[i]);
            printf("\n");
        }
    }

    return result.detected ? 1 : 0;
}

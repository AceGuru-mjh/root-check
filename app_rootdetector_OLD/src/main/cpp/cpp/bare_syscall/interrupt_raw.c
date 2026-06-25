#include "interrupt_raw.h"
#include "bare_syscall.h"
#include "raw_file_io.h"
#include "syscall_arm64.h"
#include <string.h>

static int parse_irq_line(const char *line, irq_stat_t *stat) {
    if (!line || !stat) return -1;

    const char *p = line;

    while (*p == ' ') p++;

    int irq = 0;
    int has_irq = 0;
    while (*p >= '0' && *p <= '9') {
        irq = irq * 10 + (*p - '0');
        has_irq = 1;
        p++;
    }

    if (!has_irq) return -1;
    stat->irq_number = irq;

    while (*p == ' ' || *p == ':') p++;

    uint64_t count = 0;
    while (*p >= '0' && *p <= '9') {
        count = count * 10 + (*p - '0');
        p++;
        while (*p == ' ') p++;
    }
    stat->interrupt_count = count;

    int name_idx = 0;
    while (*p && *p != '\n' && name_idx < 63) {
        stat->name[name_idx++] = *p;
        p++;
    }
    stat->name[name_idx] = '\0';

    stat->is_suspicious = 0;
    return 0;
}

int interrupt_raw_read(irq_snapshot_t *snapshot) {
    if (!snapshot) return INT_RAW_ERR_PARAM;

    char *buf = NULL;
    size_t buf_size = 0;

    int ret = read_proc_node("/proc/interrupts", &buf, &buf_size);
    if (ret != RAW_IO_OK) return INT_RAW_ERR_ACCESS;

    snapshot->source_count = 0;
    snapshot->total_interrupts = 0;
    snapshot->total_since_boot = 0;

    const char *line = buf;
    int line_num = 0;

    while (line && *line) {
        if (line_num > 0) {
            irq_stat_t stat;
            memset(&stat, 0, sizeof(stat));
            if (parse_irq_line(line, &stat) == 0 &&
                snapshot->source_count < INT_RAW_MAX_SOURCES) {
                snapshot->stats[snapshot->source_count] = stat;
                snapshot->total_interrupts += stat.interrupt_count;
                snapshot->source_count++;
            }
        }

        const char *nl = strchr(line, '\n');
        if (!nl) break;
        line = nl + 1;
        line_num++;
    }

    raw_free_file(buf, buf_size);
    return INT_RAW_OK;
}

int interrupt_raw_diff(const irq_snapshot_t *before, const irq_snapshot_t *after) {
    if (!before || !after) return INT_RAW_ERR_PARAM;

    int anomaly_count = 0;

    for (int i = 0; i < after->source_count; i++) {
        for (int j = 0; j < before->source_count; j++) {
            if (after->stats[i].irq_number == before->stats[j].irq_number) {
                uint64_t diff = after->stats[i].interrupt_count -
                                before->stats[j].interrupt_count;
                if (diff > 1000000) {
                    after->stats[i].is_suspicious = 1;
                    anomaly_count++;
                }
                break;
            }
        }
    }

    return anomaly_count;
}

int interrupt_raw_detect_anomaly(const irq_snapshot_t *current) {
    if (!current) return INT_RAW_ERR_PARAM;

    int anomalies = 0;

    for (int i = 0; i < current->source_count; i++) {
        const char *name = current->stats[i].name;

        if (strstr(name, "timer") && current->stats[i].interrupt_count > 100000000ULL) {
            continue;
        }

        if (current->stats[i].interrupt_count > 50000000ULL) {
            if (strstr(name, "syscall") || strstr(name, "hooks") ||
                strstr(name, "gate")) {
                current->stats[i].is_suspicious = 1;
                anomalies++;
            }
        }

        if (current->stats[i].interrupt_count == 0 &&
            current->stats[i].irq_number > 0 &&
            current->stats[i].irq_number < 200) {
            if (strlen(current->stats[i].name) > 0) {
                current->stats[i].is_suspicious = 1;
                anomalies++;
            }
        }
    }

    return anomalies;
}

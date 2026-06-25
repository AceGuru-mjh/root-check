/*
 * ====================================================================
 * plugin_interface.h — 七层检测插件标准接口 (v2.0)
 *
 * 此为顶层规范文件，所有检测插件必须遵守此接口。
 *
 * ▎架构原则
 * ───────────
 * 1. 每层检测为独立 .so，微内核通过 dlopen 动态加载。
 * 2. 每个 .so 仅导出 register_plugin() 符号 (→ plugin_info_t*)。
 * 3. 插件禁止直接调用 libc 标准函数，需通过域 D 汇编 syscall。
 * 4. 所有输出写入隔离 mmap 缓冲区，互不污染。
 * 5. 结果即时 SHA3-256 哈希，送域 C 守护进程验签。
 *
 * ▎插件生命周期
 * ─────────────
 *   dlopen → register_plugin() → info->init(rule_data) → info->detect(ctx)
 *       → info->cleanup() → dlclose
 *
 * ▎安全约束
 * ─────────────
 * - 不得使用可 Hook 的 libc API (fopen / stat / access / popen 等)
 * - 必须通过 bare_syscall.h 中的裸系统调用操作文件、进程、内存
 * - 敏感字符串需 xor 编码，禁止明文字符串 (如 "/su", "magisk")
 * - 每次检测必须全量执行，不得缓存
 * - 插入随机延迟和 CPU 迁移防侧信道分析
 * ====================================================================
 */
#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================
 * 第 1 节 — 常量与枚举定义
 * ================================================================== */

#define PLUGIN_NAME_MAX     64
#define PLUGIN_VERSION_MAX  16
#define PLUGIN_DESC_MAX     128
#define DETAIL_BUF_SIZE     4096
#define PLUGIN_HASH_LEN     64               /* SHA3-512 输出长度（架构C级可信验签） */

/* 七层 ID 常量 */
#define LAYER_ID_PROPERTY       1    /* L1: 原始属性裸读 */
#define LAYER_ID_ART            2    /* L2: ART 虚拟机注入检测 */
#define LAYER_ID_MEMORY         3    /* L3: 进程 & 内存指纹 */
#define LAYER_ID_MOUNT          4    /* L4: 命名空间挂载对比 */
#define LAYER_ID_SIDECHANNEL    5    /* L5: 侧信道时延分析 */
#define LAYER_ID_KERNEL         6    /* L6: 内核完整性探针 */
#define LAYER_ID_TEE            7    /* L7: TEE TrustZone 旁路 */

/* 检测级别 — 决定本次任务执行哪些插件层 */
typedef enum {
    DETECT_LEVEL_QUICK    = 1,    /* L1+L3+L4, <1s */
    DETECT_LEVEL_STANDARD = 2,    /* L1-L5,    <5s */
    DETECT_LEVEL_DEEP     = 3,    /* L1-L7,    <15s */
    DETECT_LEVEL_FORENSIC = 4     /* L1-L7 + 额外验证, <60s */
} detect_level_t;

/* 规则类型 — 微内核下发的加密规则条目 */
typedef enum {
    RULE_TYPE_MEMORY_SIG       = 1,  /* 内存签名 */
    RULE_TYPE_KERNEL_HASH      = 2,  /* 内核函数哈希 */
    RULE_TYPE_MOUNT_FEATURE    = 3,  /* 挂载特征 */
    RULE_TYPE_SU_PATH          = 4,  /* su 路径 */
    RULE_TYPE_LATENCY_THRESH   = 5,  /* 系统调用延迟阈值 */
    RULE_TYPE_PROPERTY_PATTERN = 6,  /* 属性特征模式 */
    RULE_TYPE_TEE_NODE         = 7   /* TEE 设备节点标识 */
} rule_type_t;

/* ==================================================================
 * 第 2 节 — 核心数据结构
 * ================================================================== */

/* 规则条目（变长，尾部跟 data 字段） */
typedef struct {
    uint32_t rule_type;
    uint32_t data_len;
    uint8_t  data[];
} rule_entry_t;

/* 插件检测上下文 — execute 时由微内核传入 */
typedef struct {
    const void  *rule_data;        /* 解密后的加密规则基址 */
    size_t       rule_data_size;
    uint8_t     *work_buffer;      /* 隔离 mmap 缓冲区 */
    size_t       work_buffer_size;
    uint64_t     task_random_salt; /* 每次任务唯一盐值 */
    int          cpu_core;         /* 当前绑定 CPU */
} plugin_context_t;

/* 插件输出结构 — 写入隔离缓冲区，微内核读出并哈希 */
typedef struct {
    uint32_t layer_id;             /* 层 ID (1-7) */
    uint8_t  detected;             /* 0=未检出, 1=检出 */
    uint32_t confidence;           /* 置信度 0-100 */
    uint32_t risk_score;           /* 威胁分 0-100 */
    uint64_t cost_ns;              /* 执行耗时纳秒 */
    char     detail[DETAIL_BUF_SIZE]; /* 检测详情 UTF-8 */
    uint8_t  hash[PLUGIN_HASH_LEN];   /* 输出 SHA3-256 (由微内核验算) */
} plugin_result_t;

/* ==================================================================
 * 第 3 节 — 插件接口结构
 * ================================================================== */

typedef struct {
    uint32_t    layer_id;
    const char *name;               /* 短名称: "l1_property" */
    const char *version;            /* 语义化版本: "1.0.0" */
    const char *description;        /* 自然语言描述 */

    /* ── 生命周期 ────────────────────────── */
    int  (*init)(const void *rule_data, size_t rule_data_size);
    int  (*detect)(const plugin_context_t *ctx, plugin_result_t *out);
    void (*cleanup)(void);
} plugin_info_t;

/* ==================================================================
 * 第 4 节 — 微内核→插件通信类型
 * ================================================================== */

#define MAX_LAYERS 7

/* 扫描任务（由微内核解析 IPC 消息后构造） */
typedef struct {
    uint32_t      task_id;
    detect_level_t level;
    uint32_t      plugin_mask;       /* 位掩码，选执行层 */
    uint64_t      deadline_ns;
    uint8_t       session_key[32];
    uint64_t      random_salt;
} scan_task_t;

/* 全局安全报告（域C签名后推送给域A） */
typedef struct {
    uint32_t         report_id;
    uint32_t         task_id;
    uint64_t         timestamp_ns;        /* CLOCK_MONOTONIC_RAW 纳秒 */
    uint32_t         layer_count;
    uint32_t         root_detected;       /* 0/1 最终判定 */
    uint32_t         risk_score;          /* 综合风险分 0-100 */
    uint32_t         tamper_detected;     /* 域C扫描发现篡改 */
    uint32_t         tamper_source;       /* 0=无, 1=域A, 2=域B, 3=域C */
    uint32_t         reserved[2];
    plugin_result_t  layer_results[MAX_LAYERS];
    uint8_t          aggregated_hash[PLUGIN_HASH_LEN]; /* 全层SHA3-512聚合 */
    uint8_t          daemon_signature[64]; /* 域C设备唯一密钥签名 */
} global_secure_report_t;

/* ==================================================================
 * 第 5 节 — 插件注册宏
 *
 * 每个 .so 必须且只能调用 PLUGIN_REGISTER 一次。
 * 宏展开为:
 *   1. 一个 static plugin_info_t 实例
 *   2. 一个 register_plugin() 函数，返回该实例指针
 *   3. __attribute__((constructor)) 自动调用
 *
 * 微内核通过 dlsym(handle, "register_plugin") 获取入口。
 * ================================================================== */

/* 低层级：手动注册（适合需要自定义时机的情况） */
__attribute__((visibility("default")))
plugin_info_t *register_plugin(void);

/* 标准宏：自动注册（推荐所有插件使用） */
#define PLUGIN_REGISTER(id, short_name, ver, desc, init_fn, detect_fn, cleanup_fn) \
    static plugin_info_t __plugin_instance = {                                    \
        .layer_id    = (id),                                                      \
        .name        = (short_name),                                              \
        .version     = (ver),                                                     \
        .description = (desc),                                                    \
        .init        = (init_fn),                                                 \
        .detect      = (detect_fn),                                               \
        .cleanup     = (cleanup_fn)                                               \
    };                                                                            \
    __attribute__((used, visibility("default")))                                  \
    plugin_info_t *register_plugin(void) {                                        \
        return &__plugin_instance;                                                \
    }

/* 快速宏：适用于只实现 detect() 的简单插件 */
#define PLUGIN_SIMPLE(id, short_name, ver, desc, detect_fn)                       \
    static int  __plugin_##id##_init(const void *d, size_t s) { (void)d; (void)s; return 0; } \
    static void __plugin_##id##_cleanup(void) {}                                  \
    PLUGIN_REGISTER(id, short_name, ver, desc,                                    \
                    __plugin_##id##_init, detect_fn, __plugin_##id##_cleanup)

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_INTERFACE_H */
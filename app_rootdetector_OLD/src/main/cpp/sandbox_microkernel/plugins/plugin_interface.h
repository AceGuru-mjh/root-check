/*
 * plugin_interface.h — 域B微内核七层检测插件标准接口
 *
 * 所有检测插件（L1~L7）必须实现此接口，微内核调度器通过 dlsym
 * 动态加载调用。每层插件完全独立编译为 .so，单模块崩溃不影响整体。
 *
 * 约束：
 *   - 禁止调用任何 libc 标准函数，必须使用域D汇编 syscall 封装
 *   - 禁止使用全局/静态变量（破坏内存隔离）
 *   - 所有动态分配内存通过域D memory_raw 接口
 *   - 输出结果统一通过 plugin_result_t 返回，每层结果独立哈希签名
 */
#ifndef ROOTGUARD_PLUGIN_INTERFACE_H
#define ROOTGUARD_PLUGIN_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * 版本 & 标识
 * ==================================================================== */
#define PLUGIN_INTERFACE_VERSION    2

/* 最大层名称长度 */
#define PLUGIN_NAME_MAX             64

/* 最大详情描述长度 */
#define PLUGIN_DETAIL_MAX           2048

/* 哈希长度（SHA3-512） */
#define PLUGIN_HASH_LEN             64

/* 内存特征匹配的最大数量 */
#define PLUGIN_MAX_MATCHES          64

/* 每层最大子检测项数 */
#define PLUGIN_MAX_CHECKS           128

/* ====================================================================
 * 检测结果状态码
 * ==================================================================== */
typedef enum {
    PLUGIN_OK                = 0,    /* 检测正常完成，未发现风险 */
    PLUGIN_DETECTED          = 1,    /* 检测到风险 */
    PLUGIN_SUSPICIOUS        = 2,    /* 疑似风险（需交叉验证） */
    PLUGIN_SKIPPED           = 3,    /* 跳过（白名单过滤/条件不满足） */
    PLUGIN_ERROR             = -1,   /* 插件内部错误 */
    PLUGIN_ERR_MEMORY        = -2,   /* 内存分配失败 */
    PLUGIN_ERR_TIMEOUT       = -3,   /* 执行超时 */
    PLUGIN_ERR_TAMPER        = -4,   /* 检测到自身被篡改 */
    PLUGIN_ERR_NOT_IMPLEMENTED = -5 /* 功能未实现 */
} plugin_status_t;

/* ====================================================================
 * 检测结果（单层插件输出）
 * 每个插件返回此结构，微内核调度器对其进行密码学哈希签名
 * ==================================================================== */
typedef struct {
    /* 层标识符（1~7） */
    uint32_t        layer_id;

    /* 检测状态 */
    plugin_status_t status;

    /* 置信度 [0, 100]，0=无风险，100=确定风险 */
    uint32_t        confidence;

    /* 风险评分 [0, 1000]，供 UI 聚合展示 */
    uint32_t        risk_score;

    /* 检测耗时（纳秒，由微内核计时注入） */
    uint64_t        elapsed_ns;

    /* 匹配到的特征数量 */
    uint32_t        match_count;

    /* 详情描述（UTF-8，PLUGIN_DETAIL_MAX 字节以内） */
    char            detail[PLUGIN_DETAIL_MAX];

    /* 匹配特征列表 */
    struct {
        /* 特征标识（如 Magisk 内存特征 "magiskd:xyz"） */
        char        fingerprint_id[64];

        /* 匹配偏移/地址（/proc/self/maps 行号或内存偏移） */
        uint64_t    match_offset;

        /* 匹配长度 */
        uint32_t    match_len;

        /* 严重性标记 */
        uint32_t    severity;
    } matches[PLUGIN_MAX_MATCHES];

    /* 本层结果的 SHA3-512 哈希（由微内核在结果返回后填充，插件无需计算） */
    /* 用于域C守护进程验签时校验完整性 */
    uint8_t         result_hash[PLUGIN_HASH_LEN];
} plugin_result_t;

/* ====================================================================
 * 单次检测项定义（用于插件内部组织子检测逻辑）
 * 每层插件可注册多个子检测项，微内核会随机打乱顺序执行
 * ==================================================================== */
typedef enum {
    CHECK_TYPE_MEMORY_SCAN     = 0,  /* 内存扫描 /proc/self/maps */
    CHECK_TYPE_FILE_SCAN       = 1,  /* 文件系统遍历 */
    CHECK_TYPE_SYSCALL_TIMING  = 2,  /* 系统调用时序分析 */
    CHECK_TYPE_MOUNT_COMPARE   = 3,  /* 挂载命名空间对比 */
    CHECK_TYPE_KERNEL_PROBE    = 4,  /* 内核完整性探针 */
    CHECK_TYPE_TEE_ACCESS      = 5,  /* TEE TrustZone 原始操作 */
    CHECK_TYPE_PROP_READ       = 6,  /* 属性裸读 /dev/__properties__ */
    CHECK_TYPE_ART_SCAN        = 7,  /* ART 虚拟机注入检测 */
    CHECK_TYPE_PROCESS_WALK    = 8,  /* 进程列表遍历 getdents64 */
    CHECK_TYPE_CPU_BIND        = 9,  /* CPU 绑定/调频 */
    CHECK_TYPE_HASH_VERIFY     = 10, /* 哈希校验类 */
    CHECK_TYPE_CONTRAST        = 11, /* 差分对比类 */
    CHECK_TYPE_CUSTOM          = 255 /* 自定义检测类型 */
} plugin_check_type_t;

typedef struct {
    uint32_t            check_id;       /* 子检测项唯一 ID */
    plugin_check_type_t check_type;     /* 检测类型 */
    char                check_name[64]; /* 子检测项名称 */
    uint32_t            weight;         /* 权重（用于全局评分聚合） */
    uint64_t            timeout_ns;     /* 单项超时（纳秒） */
} plugin_check_def_t;

/* ====================================================================
 * 插件配置选项（微内核调度器注入）
 * ==================================================================== */
typedef struct {
    /* 本次扫描级别（0=快速,1=常规,2=深度,3=取证） */
    uint32_t    scan_level;

    /* 设备随机盐值（每次扫描不同，用于防重放） */
    uint64_t    random_salt;

    /* CPU核心绑定掩码（微内核已绑定，插件可通过 cpu_control 确认） */
    uint64_t    cpu_affinity_mask;

    /* 需要跳过的子检测项位掩码 */
    uint64_t    skip_checks_mask;

    /* 是否启用详细调试输出 */
    uint32_t    verbose;

    /* 保留扩展 */
    uint32_t    reserved[4];
} plugin_config_t;

/* ====================================================================
 * 插件引用计数 / 生命周期状态
 * ==================================================================== */
typedef enum {
    PLUGIN_STATE_UNLOADED   = 0,
    PLUGIN_STATE_LOADED     = 1,
    PLUGIN_STATE_INITIALIZED = 2,
    PLUGIN_STATE_EXECUTING  = 3,
    PLUGIN_STATE_COMPLETED  = 4,
    PLUGIN_STATE_ERROR      = 5
} plugin_state_t;

/* ====================================================================
 * 插件描述符（每个插件必须导出一个同名符号）
 * 命名规则：rg_plugin_{l1|l2|...|l7}_descriptor
 * 示例：rg_plugin_l1_descriptor, rg_plugin_l5_descriptor
 * ==================================================================== */
typedef struct {
    /* 插件接口版本（必须等于 PLUGIN_INTERFACE_VERSION） */
    uint32_t    interface_version;

    /* 层 ID（1~7，对应 L1~L7） */
    uint32_t    layer_id;

    /* 插件名称（如 "L1_PropertyRawReader"） */
    char        name[PLUGIN_NAME_MAX];

    /* 简短描述 */
    char        description[128];

    /* 版本字符串（如 "1.2.0"） */
    char        version[16];

    /* 插件作者/组织 */
    char        author[64];

    /* 注册的子检测项数量 */
    uint32_t    check_count;

    /* 指向子检测项定义数组的指针 */
    const plugin_check_def_t *check_defs;

    /* ==============================================================
     * 生命周期回调函数指针
     * ============================================================== */

    /*
     * init: 插件初始化
     * 在插件首次加载时调用，用于分配资源、预热数据。
     * 若返回非 0，微内核将卸载此插件并标记为不可用。
     * 参数：
     *   config  - 微内核注入的配置（含随机盐值、扫描等级等）
     * 返回：
     *   0 = 成功，非 0 = 失败
     */
    int (*init)(const plugin_config_t *config);

    /*
     * execute: 执行一次检测
     * 每次扫描任务时调用。微内核会在调用前绑定随机CPU核心，
     * 分配独立内存池，注入纳秒级噪声。
     * 参数：
     *   config  - 当前扫描配置（含任务唯一盐值等）
     *   result  - 输出缓冲区，插件写入检测结果
     * 返回：
     *   plugin_status_t
     */
    plugin_status_t (*execute)(const plugin_config_t *config,
                               plugin_result_t *result);

    /*
     * cleanup: 插件清理
     * 在插件被卸载（或进程退出）时调用，用于释放资源。
     * 即使是紧急卸载，微内核也会保证调用此函数。
     */
    void (*cleanup)(void);

    /*
     * self_check: 内存自校验回调
     * 由域C守护进程周期性调用，校验插件代码段完整性。
     * 插件应返回自身关键段的哈希供守护进程比对。
     * 如果检测到篡改，应返回 PLUGIN_ERR_TAMPER。
     * 参数：
     *   hash_out  - 输出自身代码段哈希（64字节）
     * 返回：
     *   0 = 完整，非 0 = 检测到篡改
     */
    int (*self_check)(uint8_t hash_out[PLUGIN_HASH_LEN]);

    /* 插件当前状态（由微内核维护，插件不应修改） */
    plugin_state_t state;

    /* 保留扩展 */
    uint64_t    reserved[4];
} plugin_descriptor_t;

/* ====================================================================
 * 插件辅助宏（简化插件创建）
 * ==================================================================== */

/* 声明插件描述符（每个插件 .c 文件中使用一次） */
#define DECLARE_PLUGIN_DESCRIPTOR(name) \
    plugin_descriptor_t name = { \
        .interface_version = PLUGIN_INTERFACE_VERSION, \
        .layer_id         = 0, \
        .name             = #name, \
        .description      = "", \
        .version          = "1.0.0", \
        .author           = "RootGuard", \
        .check_count      = 0, \
        .check_defs       = NULL, \
        .init             = NULL, \
        .execute          = NULL, \
        .cleanup          = NULL, \
        .self_check       = NULL, \
        .state            = PLUGIN_STATE_UNLOADED \
    }

/* 便捷初始化宏 */
#define PLUGIN_INIT_OK      0
#define PLUGIN_INIT_FAIL    -1

/* ====================================================================
 * 微内核调度器导出函数（插件可调用）
 *
 * 以下函数由 sandbox_main / micro_kernel.c 定义并导出符号，
 * 插件可通过 dlsym(RTLD_DEFAULT, ...) 或链接时直接调用。
 * ==================================================================== */

/* 获取高精度纳秒时间（CLOCK_MONOTONIC_RAW） */
uint64_t plugin_get_time_ns(void);

/* 生成随机数（基于硬件随机数生成器或混合随机池） */
uint64_t plugin_get_random(void);

/* 内存安全分配/释放（从微内核隔离池分配） */
void *plugin_malloc(size_t size);
void  plugin_free(void *ptr);

/* 日志输出（通过域D裸syscall写入） */
void plugin_log(const char *tag, const char *fmt, ...);

/* 注册自定义子检测项（可在 init 中动态添加） */
int plugin_register_check(plugin_check_def_t *check_def);

#ifdef __cplusplus
}
#endif

#endif /* ROOTGUARD_PLUGIN_INTERFACE_H */

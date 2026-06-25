/*
 * ipc_protocol.h - RootGuard 全域加密 IPC 协议定义（域A↔B↔C）
 *
 * 三进程物理隔离架构通信协议：
 *   域A（前端可信交互域/UI进程）  -> 域B（沙箱检测执行域）: 单向加密任务派发
 *   域B（沙箱检测执行域）          -> 域C（可信安全守护域）: 双向加密管道，逐层结果推送验签
 *   域C（可信安全守护域）          -> 域A（前端可信交互域）: 守护进程签名的完整安全报告
 *
 * 安全策略：
 *   - AES-256-GCM 加密，每条消息独立 Nonce
 *   - 64bit 单调递增序列号防重放攻击
 *   - 硬件随机盐时间戳（CLOCK_MONOTONIC_RAW）防时间篡改
 *   - 域C 使用设备唯一密钥对全部检测结果签名，无签名即伪造
 *   - 禁止 Binder 通信（Binder 易被 LSPosed Hook）
 */

#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 常量定义 ===================== */

/* 协议魔数：0x52470000 ("RG\0\0") */
#define IPC_MAGIC                   ((uint32_t)0x52470000U)

/* 协议版本 */
#define IPC_PROTOCOL_VERSION        2

/* 时间戳有效期：5 秒（纳秒） */
#define IPC_TIMESTAMP_VALID_WINDOW_NS  (5ULL * 1000ULL * 1000ULL * 1000ULL)

/* 最大加密载荷大小：64 KB */
#define IPC_MAX_PAYLOAD_SIZE        (64U * 1024U)

/* 最大明文缓冲区大小（含对齐） */
#define IPC_MAX_PLAINTEXT_SIZE      (64U * 1024U)

/* AAD / Tag / Nonce 长度（AES-256-GCM） */
#define IPC_AAD_LEN                 16
#define IPC_TAG_LEN                 16
#define IPC_NONCE_LEN               12

/* 详情字符串最大长度 */
#define IPC_DETAIL_MAX_LEN          2048

/* 哈希签名长度（SHA3-512，密码学可信验签） */
#define IPC_HASH_LEN                64

/* 域C守护进程签名长度（SHA3-512 + 设备唯一密钥派生签名） */
#define IPC_DAEMON_SIGNATURE_LEN    64

/* 最大检测层数 */
#define IPC_MAX_LAYERS              32

/* Socket 路径（仅域A/B/C各自本地 Socket） */
#define IPC_SOCKET_PATH_SANDBOX     "/data/data/com.rootguard/socket/sandbox_ipc"
#define IPC_SOCKET_PATH_DAEMON      "/data/data/com.rootguard/socket/daemon_ipc"

/* ===================== 错误码 ===================== */

typedef enum {
    IPC_OK                      =  0,
    IPC_ERR_INVALID_ARG         = -1,
    IPC_ERR_BUFFER_TOO_SMALL    = -2,
    IPC_ERR_BAD_MAGIC           = -3,
    IPC_ERR_BAD_VERSION         = -4,
    IPC_ERR_DESERIALIZE         = -5,
    IPC_ERR_CRYPTO_ENCRYPT      = -6,
    IPC_ERR_CRYPTO_DECRYPT      = -7,
    IPC_ERR_TAG_MISMATCH        = -8,
    IPC_ERR_REPLAY_SEQ_OLD      = -9,
    IPC_ERR_REPLAY_SEQ_DUP      = -10,
    IPC_ERR_TIMESTAMP_EXPIRED   = -11,
    IPC_ERR_PAYLOAD_TOO_LARGE   = -12,
    IPC_ERR_SOCKET_CREATE       = -20,
    IPC_ERR_SOCKET_BIND         = -21,
    IPC_ERR_SOCKET_LISTEN       = -22,
    IPC_ERR_SOCKET_ACCEPT       = -23,
    IPC_ERR_SOCKET_CONNECT      = -24,
    IPC_ERR_SOCKET_SEND         = -25,
    IPC_ERR_SOCKET_RECV         = -26,
    IPC_ERR_SOCKET_CLOSED       = -27,
    IPC_ERR_INTERNAL            = -99
} ipc_status_t;

/* ===================== 消息类型 ===================== */

/*
 * 三域消息流向：
 *   A→B : IPC_MSG_SCAN_TASK         域A→域B 任务派发
 *   B→C : IPC_MSG_LAYER_RESULT      域B→域C 单层检测结果实时推送
 *   B→C : IPC_MSG_SIGN_REQUEST      域B→域C 请求全局签名
 *   C→B : IPC_MSG_SIGN_RESPONSE     域C→域B 返回签名结果
 *   C→A : IPC_MSG_SIGNED_REPORT     域C→域A 签名报告下发
 *   ALL : IPC_MSG_SECURITY_ALERT    任意方向 安全告警
 *   ALL : IPC_MSG_HEARTBEAT         双向 心跳保活
 *   C→A : IPC_MSG_DAEMON_STATUS     域C→域A 守护进程状态通知
 */
typedef enum {
    IPC_MSG_NONE            = 0,
    IPC_MSG_SCAN_TASK       = 1,   /* A -> B : 扫描任务派发 */
    IPC_MSG_LAYER_RESULT    = 2,   /* B -> C : 单层检测结果实时推送 */
    IPC_MSG_SIGN_REQUEST    = 3,   /* B -> C : 请求 daemon 全局签名 */
    IPC_MSG_SIGN_RESPONSE   = 4,   /* C -> B : daemon 签名结果 */
    IPC_MSG_SIGNED_REPORT   = 5,   /* C -> A : 全域签名报告下发 */
    IPC_MSG_SECURITY_ALERT  = 6,   /* 任意方向 : 安全告警 */
    IPC_MSG_HEARTBEAT       = 7,   /* 双向 : 心跳保活 */
    IPC_MSG_DAEMON_STATUS   = 8,   /* C -> A : 守护进程状态通知 */

    IPC_MSG_TYPE_MAX
} ipc_msg_type_t;

/* ===================== 消息头 ===================== */

/*
 * 所有 IPC 消息的通用头部（48 + 16 + 16 = 80 字节）。
 * 传输格式：[header][encrypted_payload]
 * 其中 encrypted_payload 为 AES-GCM 加密后的密文，tag 单独放在 header.tag。
 */
typedef struct {
    uint32_t magic;          /* 必须等于 IPC_MAGIC */
    uint32_t msg_type;       /* ipc_msg_type_t */
    uint64_t sequence_num;   /* 64 位单调递增序列号（防重放） */
    uint64_t timestamp;      /* 硬件随机盐时间戳（纳秒，自开机） */
    uint32_t payload_len;    /* 加密后载荷长度（字节） */
    uint8_t  aad[IPC_AAD_LEN];   /* 附加认证数据 */
    uint8_t  tag[IPC_TAG_LEN];   /* AES-GCM 认证标签 */
} __attribute__((packed)) ipc_message_header_t;

/* 完整消息容器（用于内存传递，非序列化格式） */
typedef struct {
    ipc_message_header_t header;
    uint8_t  *payload;           /* 明文载荷（反序列化后）或密文（序列化后） */
    size_t    payload_len;
    int       is_encrypted;      /* 1 = payload 为密文；0 = 明文 */
} ipc_message_t;

/* ===================== ScanTask 载荷 ===================== */

/* 检测等级 */
typedef enum {
    IPC_SCAN_LEVEL_QUICK      = 0,   /* 快速扫描 */
    IPC_SCAN_LEVEL_NORMAL     = 1,   /* 常规扫描 */
    IPC_SCAN_LEVEL_DEEP       = 2,   /* 深度扫描 */
    IPC_SCAN_LEVEL_FORENSIC   = 3    /* 取证扫描 */
} ipc_scan_level_t;

/* 插件白名单位掩码（每一位对应一个检测插件） */
#define IPC_PLUGIN_APATCH           (1U << 0)
#define IPC_PLUGIN_ART_HOOK         (1U << 1)
#define IPC_PLUGIN_FILE_PATH        (1U << 2)
#define IPC_PLUGIN_KERNEL_MOD       (1U << 3)
#define IPC_PLUGIN_MOUNT            (1U << 4)
#define IPC_PLUGIN_MEMORY           (1U << 5)
#define IPC_PLUGIN_MEMORY_FP        (1U << 6)
#define IPC_PLUGIN_PROCESS          (1U << 7)
#define IPC_PLUGIN_PROPERTY         (1U << 8)
#define IPC_PLUGIN_SELINUX          (1U << 9)
#define IPC_PLUGIN_SYSCALL_TIMING   (1U << 10)
#define IPC_PLUGIN_SELF_PROTECT     (1U << 11)
#define IPC_PLUGIN_ALL              (0xFFFFFFFFU)

/*
 * ScanTask（域A→域B 扫描任务派发）
 * 每次扫描任务携带独立随机盐值，防止重放攻击
 */
typedef struct {
    uint32_t detection_level;       /* ipc_scan_level_t */
    uint32_t plugin_whitelist;      /* 插件白名单位掩码（bit0= L1, bit1=L2, ...） */
    uint64_t random_salt;           /* 硬件随机盐（每次任务唯一，用于派生会话密钥） */
    uint64_t task_deadline_ns;      /* 任务截止时间戳（纳秒） */
    uint32_t flags;                 /* 保留扩展位 */
    uint32_t cpu_affinity_hint;     /* CPU 亲和建议（0=微内核自动选择） */
} __attribute__((packed)) ipc_scan_task_t;

/* ===================== TrustedLayerResult（域B→域C 单层检测结果） ===================== */

/*
 * 每层检测输出的原始数据结构。
 * 域B沙箱完成单层检测后立即加密推送至域C守护进程。
 * 域C收到后先验签哈希完整性，再汇聚签名。
 */
typedef struct {
    uint32_t layer_id;              /* 检测层 ID（0=L1 属性, 1=L2 ART, ..., 6=L7 TEE） */
    uint32_t detected;              /* 0 = 未检出；1 = 检出风险 */
    uint32_t detail_offset;         /* 详情字符串在载荷后的偏移（字节） */
    uint32_t detail_len;            /* 详情字符串长度（含结尾 \0） */
    uint8_t  hash[IPC_HASH_LEN];    /* 本层结果 SHA3-512 哈希（域B计算，域C验签） */
    uint64_t cost_ns;               /* 检测耗时（纳秒，含噪声注入值） */
    uint32_t confidence;            /* 置信度 [0, 100] */
    uint32_t risk_score;            /* 风险评分 [0, 1000] */
    uint32_t match_count;           /* 匹配特征数量 */
    uint32_t cpu_core_id;           /* 执行时的 CPU 核心编号 */
    /* 紧跟其后：
     *   1. detail_len 字节详情 UTF-8 字符串
     *   2. match_count * 64 字节特征 ID 列表
     */
} __attribute__((packed)) ipc_layer_result_t;

/* ===================== SignRequest / SignResponse ===================== */

/*
 * SignRequest（域B→域C 请求全局签名）
 * 所有 layer 结果完成后，域B聚合哈希请求域C签名
 */
typedef struct {
    uint64_t report_id;             /* 本次报告 ID（域A下发 ScanTask 时生成） */
    uint32_t layer_count;           /* 待签名 layer 数量 */
    uint32_t payload_total_len;     /* 紧随其后的待签名数据总字节数 */
    uint8_t  aggregated_hash[IPC_HASH_LEN]; /* 所有 layer 结果的 SHA3-512 聚合哈希 */
    uint64_t task_random_salt;      /* 对应 ScanTask 的随机盐值（交叉验证） */
    /* 紧随其后是待签名的原始字节（ipc_layer_result_t 序列化数组） */
} __attribute__((packed)) ipc_sign_request_t;

/*
 * SignResponse（域C→域B 签名响应）
 * 域C使用设备唯一密钥对 aggregated_hash 签名
 */
typedef struct {
    uint64_t report_id;             /* 对应请求的 report ID */
    uint32_t status;                /* 0 = 成功；非 0 = 失败 */
    uint32_t signature_len;         /* 实际签名长度（<= IPC_DAEMON_SIGNATURE_LEN） */
    uint8_t  signature[IPC_DAEMON_SIGNATURE_LEN]; /* 域C设备唯一密钥签名 */
    uint64_t signed_at_ns;          /* 签名时 CLOCK_MONOTONIC_RAW 时间戳 */
    uint8_t  signer_key_id[32];     /* 签名密钥标识（用于域A验证签名来源） */
} __attribute__((packed)) ipc_sign_response_t;

/* ===================== GlobalSecureReport（域C→域A 全域签名报告） ===================== */

/* 单层聚合摘要（写入报告） */
typedef struct {
    uint32_t layer_id;
    uint32_t detected;              /* 0/1 */
    uint32_t confidence;            /* 0-100 */
    uint32_t risk_score;            /* 0-1000 */
    uint32_t detail_offset;         /* 在 report.detail_blob 中的偏移 */
    uint32_t detail_len;
    uint8_t  hash[IPC_HASH_LEN];    /* 该层结果哈希（与域B原始计算一致） */
} __attribute__((packed)) ipc_layer_summary_t;

/*
 * GlobalSecureReport（域C→域A 全量签名报告）
 *
 * 这是域A（UI进程）唯一能接收的数据类型。
 * 域A收到后必须先验证 daemon_signature，无签名或签名不匹配直接判定伪造。
 * 报告包含：全域风险总判定 + 逐层摘要 + 风险评分 + 设备信息
 */
typedef struct {
    uint64_t report_id;                     /* 与 ScanTask 的 report_id 一致 */
    uint64_t scan_requested_at_ns;          /* 域A请求扫描的时间戳 */
    uint64_t generated_at_ns;               /* 报告生成时间戳 */
    uint32_t detection_level;               /* 对应 ScanTask 的扫描等级 */
    uint32_t layer_count;                   /* 实际执行的 layer 数量 */
    uint32_t root_detected;                 /* 0=未检测到Root, 1=检测到Root */
    uint32_t risk_score;                    /* 0-100 聚合风险分 */
    uint32_t detail_blob_len;               /* 后续 detail blob 长度 */
    uint32_t tamper_detected;               /* 0=完整, 1=检测到篡改（域C扫描发现） */
    uint32_t tamper_source;                 /* 篡改来源（0=无, 1=域A, 2=域B, 3=域C自身） */
    uint64_t daemon_uptime_ns;              /* 域C守护进程运行时长 */
    uint8_t  daemon_signature[IPC_DAEMON_SIGNATURE_LEN]; /* 域C设备唯一密钥签名 */
    uint8_t  aggregated_hash[IPC_HASH_LEN]; /* 全部 layer 的 SHA3-512 聚合哈希 */
    /* 紧随其后：
     *   ipc_layer_summary_t   summaries[layer_count]
     *   uint8_t               detail_blob[detail_blob_len] (UTF-8, \0 分隔)
     */
} __attribute__((packed)) ipc_global_secure_report_t;

/* ===================== SecurityAlert ===================== */

/*
 * 安全告警类型（全覆盖架构风险场景）
 */
typedef enum {
    IPC_ALERT_NONE                  = 0,
    IPC_ALERT_CRYPTO_FAILURE        = 1,   /* 解密/校验失败 */
    IPC_ALERT_REPLAY_DETECTED       = 2,   /* 重放攻击 */
    IPC_ALERT_TIMESTAMP_DRIFT       = 3,   /* 时间戳漂移过大 */
    IPC_ALERT_SOCKET_TAMPER         = 4,   /* Socket 文件被篡改 */
    IPC_ALERT_UNAUTHORIZED_MSG      = 5,   /* 未授权消息类型 */
    IPC_ALERT_PROCESS_INJECT        = 6,   /* 疑似进程注入（Zygisk/LSPosed） */
    IPC_ALERT_SELF_TAMPER           = 7,   /* 内存自校验失败 */
    IPC_ALERT_MEMORY_PATCH_DETECTED = 8,   /* 域C扫描发现域A/B内存段被patch */
    IPC_ALERT_PTRACE_ATTACH         = 9,   /* 检测到 ptrace 附着 */
    IPC_ALERT_DEBUG_PORT            = 10,  /* 调试端口扫描告警 */
    IPC_ALERT_PLUGIN_CRASH          = 11,  /* 检测插件崩溃 */
    IPC_ALERT_NAMESPACE_TAMPER      = 12,  /* 命名空间被篡改 */
    IPC_ALERT_KERNEL_HOOK           = 13,  /* 内核syscall表被Hook */
    IPC_ALERT_TEE_INTEGRITY_FAIL    = 14,  /* TEE分区完整性校验失败 */
    IPC_ALERT_SIGNATURE_MISMATCH    = 15,  /* 报告签名不匹配 */
    IPC_ALERT_DAEMON_DOWN           = 16   /* 域C守护进程异常离线 */
} ipc_alert_type_t;

typedef enum {
    IPC_SEVERITY_INFO      = 0,
    IPC_SEVERITY_WARN      = 1,
    IPC_SEVERITY_CRITICAL  = 2,
    IPC_SEVERITY_FATAL     = 3
} ipc_severity_t;

/* SecurityAlert 载荷 */
typedef struct {
    uint32_t alert_type;        /* ipc_alert_type_t */
    uint32_t severity;          /* ipc_severity_t */
    uint64_t occurred_at_ns;    /* 发生时间戳 */
    uint32_t source_pid;        /* 来源进程 PID */
    uint32_t detail_len;        /* 详情字符串长度 */
    uint64_t related_seq;       /* 关联的消息序列号（如适用） */
    /* 紧随其后是 detail_len 字节的详情字符串 */
} __attribute__((packed)) ipc_security_alert_t;

/* ===================== Heartbeat ===================== */

/* 心跳状态标志位 */
#define IPC_HEARTBEAT_STATE_ALIVE       (1U << 0)   /* 进程存活 */
#define IPC_HEARTBEAT_STATE_BUSY        (1U << 1)   /* 正在执行检测 */
#define IPC_HEARTBEAT_STATE_TAMPERED    (1U << 2)   /* 检测到自身篡改 */
#define IPC_HEARTBEAT_STATE_DEGRADED    (1U << 3)   /* 降级运行（部分插件不可用） */

typedef struct {
    uint64_t sender_pid;            /* 发送方 PID */
    uint64_t tick_ns;               /* 发送方 CLOCK_MONOTONIC_RAW */
    uint32_t state;                 /* 状态标志位 */
    uint32_t uptime_sec;            /* 已运行秒数 */
    uint64_t mem_usage;             /* 当前内存占用（字节） */
    uint32_t plugin_count;          /* 已加载插件数 */
    uint32_t last_alert_seq;        /* 最近一次告警的序列号 */
} __attribute__((packed)) ipc_heartbeat_t;

/* ===================== DaemonStatus（域C→域A 守护进程状态） ===================== */

/*
 * 域C守护进程主动推送给域A的状态通知
 * 用于告知 UI 当前安全环境概览（独立于检测报告）
 */
typedef enum {
    DAEMON_STATUS_RUNNING       = 0,
    DAEMON_STATUS_DEGRADED      = 1,   /* 部分功能受限 */
    DAEMON_STATUS_TAMPERED      = 2,   /* 检测到自身篡改（高危） */
    DAEMON_STATUS_STOPPED       = 3    /* 已停止（不应出现） */
} ipc_daemon_status_t;

typedef struct {
    uint32_t        status;                 /* ipc_daemon_status_t */
    uint32_t        overall_risk_level;     /* 0=安全 1=可疑 2=高危 3=严重 */
    uint64_t        daemon_uptime_ns;       /* 守护进程持续运行时长 */
    uint32_t        running_checks_count;   /* 正在执行的校验任务数 */
    uint32_t        last_full_scan_result;  /* 最近一次完整扫描结果 */
    uint64_t        last_full_scan_at_ns;   /* 最近一次完整扫描时间 */
    uint32_t        tamper_count;           /* 累计检测到篡改次数 */
    uint32_t        alert_count;            /* 累计告警次数 */
    uint8_t         daemon_signature[IPC_DAEMON_SIGNATURE_LEN]; /* 域C自签名 */
} __attribute__((packed)) ipc_daemon_status_t;

/* ===================== 序列化 API（ipc_protocol.c） ===================== */

/*
 * serialize_message:
 *   将 ipc_message_t（明文）序列化为可发送的字节流。
 *   输出 buffer 布局：[ipc_message_header_t][ciphertext]
 *   调用前需保证 payload 已加密（is_encrypted=1）或无需加密（payload==NULL）。
 *
 * 参数：
 *   msg        - 输入消息
 *   out_buf    - 输出缓冲区
 *   out_size   - 缓冲区容量
 *   out_len    - 实际写入字节数
 */
ipc_status_t serialize_message(const ipc_message_t *msg,
                               uint8_t *out_buf,
                               size_t out_size,
                               size_t *out_len);

/*
 * deserialize_message:
 *   从字节流中解析消息头，并将密文载荷拷贝到 msg->payload。
 *   返回的 msg->is_encrypted = 1，调用方需自行解密。
 *   注意：本函数会校验 magic，但不会解密或校验序列号/时间戳。
 */
ipc_status_t deserialize_message(const uint8_t *in_buf,
                                 size_t in_len,
                                 ipc_message_t *msg);

/*
 * validate_sequence:
 *   防重放校验：期望 seq > last_seen_seq。
 *   last_seen_seq 由调用方维护（每个对端连接一个）。
 */
ipc_status_t validate_sequence(uint64_t seq, uint64_t *last_seen_seq);

/*
 * validate_timestamp:
 *   时间戳校验：|now_ns - msg_ts| <= IPC_TIMESTAMP_VALID_WINDOW_NS。
 *   now_ns 通常来自 CLOCK_MONOTONIC 或硬件计数器。
 */
ipc_status_t validate_timestamp(uint64_t msg_ts, uint64_t now_ns);

/*
 * build_aad:
 *   根据消息头字段构造 AAD（16 字节）。
 *   AAD = msg_type(4) || seq_low(4) || seq_high(4) || ts_low(4)  等。
 */
void build_aad(const ipc_message_header_t *hdr, uint8_t out_aad[IPC_AAD_LEN]);

/*
 * message_init / message_free:
 *   消息容器初始化/清理。
 */
void message_init(ipc_message_t *msg);
void message_free(ipc_message_t *msg);

/* ===================== 全域签名验证 API ===================== */

/*
 * verify_daemon_signature:
 *   验证域C守护进程签名的完整性。
 *   域A（UI）收到报告后调用此函数校验签名。
 *   签名使用设备唯一密钥派生，无签名或签名失败直接判定报告伪造。
 *
 * 参数：
 *   data       - 被签名的原始数据
 *   data_len   - 数据长度
 *   signature  - 域C签名（64字节）
 *   device_key - 设备唯一密钥（32字节，域A预置）
 *
 * 返回：
 *   0 = 签名有效，-1 = 签名无效/数据被篡改
 */
int verify_daemon_signature(const uint8_t *data,
                            size_t data_len,
                            const uint8_t signature[IPC_DAEMON_SIGNATURE_LEN],
                            const uint8_t device_key[32]);

/*
 * verify_report_integrity:
 *   一站式报告完整性校验。
 *   域A收到 GlobalSecureReport 后调用，内部：
 *   1. 验证 daemon_signature
 *   2. 验证 aggregated_hash 与各 layer 哈希一致
 *   3. 若无合法签名，设置 tamper_detected=1
 *
 * 参数：
 *   report     - 待校验报告
 *   device_key - 设备唯一密钥
 *
 * 返回：
 *   0 = 完整可信，-1 = 报告被篡改
 */
int verify_report_integrity(const ipc_global_secure_report_t *report,
                            const uint8_t device_key[32]);

#ifdef __cplusplus
}
#endif

#endif /* IPC_PROTOCOL_H */

#ifndef ROOTGUARD_IPC_PROTOCOL_H
#define ROOTGUARD_IPC_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================
 * 全域加密 IPC 协议定义
 *
 * 三域通信拓扑：
 *   域 A (UI)  ──单向任务下发──▶  域 B (沙箱)  ◀──双向加密管道──▶  域 C (守护进程)
 *       │                                                                │
 *       └─────────────────── 签名报告推送 ────────────────────────────────┘
 *
 * 安全策略：
 *   - 所有消息 AES-256-GCM 加密，16 字节认证标签
 *   - 64 位单调递增序列号，防重放
 *   - 64 位硬件随机盐时间戳，防定时攻击
 *   - 消息体 SHA3-256 哈希链，防篡改
 *
 * 序列化格式：
 *   [magic(4) | version(2) | msg_type(2) | seq(8) | ts(8) | flags(4) |
 *    payload_len(4) | nonce(12) | tag(16) | encrypted_payload]
 * ================================================================== */

/* ===================== 协议常量 ===================== */
#define IPC_MAGIC                   0x52470001U           /* "RG\0x01" */
#define IPC_PROTOCOL_VERSION        2
#define IPC_MAX_PAYLOAD_SIZE        (64U * 1024U)
#define IPC_AAD_LEN                 16
#define IPC_TAG_LEN                 16
#define IPC_NONCE_LEN               12
#define IPC_HASH_LEN                32
#define IPC_SIG_LEN                 64
#define IPC_MAX_LAYERS              32
#define IPC_TIMESTAMP_WINDOW_NS     (5ULL * 1000000000ULL) /* 5 秒 */

/* Socket 路径（安全目录，0700 权限） */
#define IPC_SOCKET_PATH_SANDBOX     "/data/data/com.rootguard/cache/sandbox_ipc"
#define IPC_SOCKET_PATH_DAEMON      "/data/data/com.rootguard/cache/daemon_ipc"

/* ===================== 错误码 ===================== */
typedef enum {
    IPC_OK                      = 0,
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
    IPC_ERR_SESSION_EXPIRED     = -28,
    IPC_ERR_INTERNAL            = -99
} ipc_status_t;

/* ===================== 消息类型 ===================== */
typedef enum {
    IPC_MSG_NONE            = 0,
    IPC_MSG_SCAN_TASK       = 1,   /* A → B: 下发扫描任务 */
    IPC_MSG_LAYER_RESULT    = 2,   /* B → C: 单层原始检测结果推送验签 */
    IPC_MSG_SIGN_REQUEST    = 3,   /* B → C: 请求全局报告签名 */
    IPC_MSG_SIGN_RESPONSE   = 4,   /* C → B: 签名后的全局报告 */
    IPC_MSG_SECURITY_ALERT  = 5,   /* 任意方向: 安全告警 */
    IPC_MSG_HEARTBEAT       = 6,   /* 双向保活 */
    IPC_MSG_DAEMON_STATUS   = 7,   /* C → A: 守护进程状态推送 */
    IPC_MSG_SHUTDOWN        = 8,   /* A → C: 优雅关闭 */
    IPC_MSG_TRIPLE_ARBITER  = 9,   /* A → C: 启动三副本仲裁 */
    IPC_MSG_TRIPLE_ARBITER_RESULT = 10, /* C → A: 仲裁结果 */
    IPC_MSG_LOG_WRITE       = 11,  /* A/B → C: 写入日志 */
    IPC_MSG_KERNEL_SNAPSHOT = 12,  /* C → A: 内核快照 */
    IPC_MSG_TYPE_MAX
} ipc_msg_type_t;

/* ===================== 消息头（线格式 56 字节） ===================== */
typedef struct {
    uint32_t magic;                 /* IPC_MAGIC */
    uint16_t version;               /* IPC_PROTOCOL_VERSION */
    uint16_t msg_type;              /* ipc_msg_type_t */
    uint64_t sequence_num;          /* 单调递增序列号 */
    uint64_t timestamp_ns;          /* CLOCK_MONOTONIC_RAW 时间戳 */
    uint32_t flags;                 /* 扩展标志位 */
    uint32_t payload_len;           /* 加密后载荷长度 */
    uint8_t  nonce[IPC_NONCE_LEN];  /* AES-GCM nonce */
    uint8_t  tag[IPC_TAG_LEN];      /* AES-GCM 认证标签 */
} __attribute__((packed)) ipc_header_t;

/* 内存消息容器 */
typedef struct {
    ipc_header_t header;
    uint8_t     *payload;
    size_t       payload_len;
    int          is_encrypted;
} ipc_message_t;

/* ===================== 检测等级 ===================== */
typedef enum {
    IPC_SCAN_LEVEL_QUICK    = 0,   /* L1+L3+L4, <1s */
    IPC_SCAN_LEVEL_NORMAL   = 1,   /* L1-L5, <5s */
    IPC_SCAN_LEVEL_DEEP     = 2,   /* L1-L7, <15s */
    IPC_SCAN_LEVEL_FORENSIC = 3    /* L1-L7 + 额外验证, <60s */
} ipc_scan_level_t;

/* ===================== 插件白名单位掩码 ===================== */
#define IPC_PLUGIN_PROPERTY         (1U << 0)
#define IPC_PLUGIN_ART              (1U << 1)
#define IPC_PLUGIN_MEMORY           (1U << 2)
#define IPC_PLUGIN_MOUNT            (1U << 3)
#define IPC_PLUGIN_SIDECHANNEL      (1U << 4)
#define IPC_PLUGIN_KERNEL           (1U << 5)
#define IPC_PLUGIN_TEE              (1U << 6)
#define IPC_PLUGIN_ALL              (0x7FU)

/* ===================== ScanTask 载荷（A → B） ===================== */
typedef struct {
    uint32_t detection_level;       /* ipc_scan_level_t */
    uint32_t plugin_whitelist;      /* 位掩码 */
    uint64_t random_salt;           /* 每次任务唯一盐值 */
    uint64_t task_deadline_ns;      /* 截止时间戳 */
    uint32_t flags;
    uint8_t  session_key[IPC_HASH_LEN];  /* 临时会话密钥 */
} __attribute__((packed)) ipc_scan_task_t;

/* ===================== LayerResult 载荷（B → C） ===================== */
/*
 * 序列化格式: [layer_result_t] 紧接 [detail_string]
 * detail_string 为 UTF-8 明文，\0 结尾
 */
typedef struct {
    uint32_t layer_id;              /* 对应插件位索引 */
    uint32_t detected;              /* 0/1 */
    uint32_t confidence;            /* 置信度 0-100 */
    uint64_t cost_ns;               /* 检测耗时纳秒 */
    uint32_t detail_offset;         /* detail_string 在本结构后的偏移 */
    uint32_t detail_len;            /* detail_string 长度(含\0) */
    uint8_t  hash[IPC_HASH_LEN];    /* 本层输出 SHA3-256 */
    uint32_t reserved[4];
} __attribute__((packed)) ipc_layer_result_t;

/* ===================== SignRequest 载荷（B → C） ===================== */
typedef struct {
    uint64_t report_id;
    uint32_t layer_count;
    uint32_t payload_total_len;     /* 后续待签名数据总字节 */
    uint8_t  aggregated_hash[IPC_HASH_LEN]; /* 各层聚合哈希 */
} __attribute__((packed)) ipc_sign_request_t;

/* ===================== SignResponse 载荷（C → B/A） ===================== */
typedef struct {
    uint64_t report_id;
    uint32_t status;                /* 0=成功 */
    uint32_t sig_len;
    uint8_t  signature[IPC_SIG_LEN]; /* Ed25519 或 HMAC-SHA3 */
    uint64_t signed_at_ns;
} __attribute__((packed)) ipc_sign_response_t;

/* ===================== 全局安全报告（C 签名后推送给 A） ===================== */
typedef struct {
    uint64_t report_id;
    uint64_t generated_at_ns;
    uint32_t detection_level;
    uint32_t layer_count;
    uint32_t root_detected;         /* 0/1 总判定 */
    uint32_t risk_score;            /* 0-100 综合风险分 */
    uint32_t detail_blob_len;
    uint8_t  daemon_signature[IPC_SIG_LEN];
    uint8_t  aggregated_hash[IPC_HASH_LEN];
    /* 紧随其后:
     *   ipc_layer_result_t[layer_count]
     *   detail_blob (UTF-8 \0 分隔拼接)
     */
} __attribute__((packed)) ipc_global_secure_report_t;

/* ===================== 安全告警 ===================== */
typedef enum {
    IPC_ALERT_NONE              = 0,
    IPC_ALERT_CRYPTO_FAILURE    = 1,
    IPC_ALERT_REPLAY_DETECTED   = 2,
    IPC_ALERT_TIMESTAMP_DRIFT   = 3,
    IPC_ALERT_SOCKET_TAMPER     = 4,
    IPC_ALERT_UNAUTHORIZED_MSG  = 5,
    IPC_ALERT_PROCESS_INJECT    = 6,
    IPC_ALERT_SELF_TAMPER       = 7,
    IPC_ALERT_MEMORY_PATCHED    = 8,
    IPC_ALERT_HOOK_DETECTED     = 9
} ipc_alert_type_t;

typedef enum {
    IPC_SEVERITY_INFO      = 0,
    IPC_SEVERITY_WARN      = 1,
    IPC_SEVERITY_CRITICAL  = 2,
    IPC_SEVERITY_FATAL     = 3
} ipc_severity_t;

typedef struct {
    uint32_t alert_type;
    uint32_t severity;
    uint64_t occurred_at_ns;
    uint32_t source_pid;
    uint32_t detail_len;
    uint64_t related_seq;
} __attribute__((packed)) ipc_security_alert_t;

/* ===================== Heartbeat ===================== */
typedef struct {
    uint64_t sender_pid;
    uint64_t tick_ns;
    uint32_t state;                 /* 发送方状态字 */
    uint32_t reserved;
} __attribute__((packed)) ipc_heartbeat_t;

/* ===================== 序列化 API ===================== */
ipc_status_t ipc_serialize(const ipc_message_t *msg,
                           uint8_t *out, size_t out_size, size_t *out_len);
ipc_status_t ipc_deserialize(const uint8_t *in, size_t in_len,
                             ipc_message_t *msg);

ipc_status_t ipc_validate_sequence(uint64_t seq, uint64_t *last_seen);
ipc_status_t ipc_validate_timestamp(uint64_t msg_ts, uint64_t now_ns);
void         ipc_build_aad(const ipc_header_t *hdr, uint8_t aad[IPC_AAD_LEN]);

void ipc_message_init(ipc_message_t *msg);
void ipc_message_free(ipc_message_t *msg);

/* ===================== 会话管理 ===================== */
typedef struct {
    int       sock_fd;
    char      socket_path[108];
    uint8_t   session_key[IPC_HASH_LEN];
    uint64_t  last_seq;
    uint32_t  timeouts;
} ipc_session_t;

int  ipc_session_init(ipc_session_t *sess, const char *path);
void ipc_session_close(ipc_session_t *sess);
int  ipc_session_send(ipc_session_t *sess, ipc_msg_type_t type,
                      const void *payload, uint32_t len);
int  ipc_session_recv(ipc_session_t *sess, ipc_message_t *msg);
int  ipc_handshake_server(int fd, ipc_session_t *sess);
int  ipc_handshake_client(int fd, ipc_session_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* ROOTGUARD_IPC_PROTOCOL_H */
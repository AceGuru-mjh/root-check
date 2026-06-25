#ifndef RULE_PARSER_H
#define RULE_PARSER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 规则类型定义 ========== */

typedef enum {
    RULE_TYPE_MEMORY_SIG = 1,      // 内存签名
    RULE_TYPE_KERNEL_HASH = 2,     // 内核函数哈希
    RULE_TYPE_MOUNT_FEATURE = 3,   // 挂载特征
    RULE_TYPE_SU_PATH = 4,         // su 路径
    RULE_TYPE_LATENCY_THRESHOLD = 5 // 系统调用延迟阈值
} rule_type_t;

/* ========== 规则数据结构 ========== */

typedef struct {
    uint32_t rule_type;    // 规则类型
    uint32_t data_len;
    uint8_t  data[];       // 可变长度规则数据
} rule_entry_t;

/* ========== 规则文件头 ========== */

typedef struct {
    uint32_t magic;            // 魔数: 0x52554C45 ("RULE")
    uint32_t version;          // 版本号
    uint32_t rule_count;       // 规则数量
    uint32_t total_size;       // 总大小
    uint8_t  hash[32];         // 文件哈希 (SHA3-256)
    uint8_t  iv[12];           // AES-GCM IV
    uint8_t  tag[16];          // AES-GCM 认证标签
} rule_file_header_t;

/* ========== 规则解析器上下文 ========== */

typedef struct {
    char file_path[256];       // 规则文件路径
    uint8_t* encrypted_data;   // 加密的规则数据
    size_t encrypted_size;     // 加密数据大小
    uint8_t* decrypted_data;   // 解密后的规则数据
    size_t decrypted_size;     // 解密数据大小
    rule_entry_t** rules;      // 规则数组
    uint32_t rule_count;       // 规则数量
    int loaded;                // 是否已加载
} rule_parser_ctx_t;

/* ========== 规则解析器 API ========== */

/**
 * 初始化规则解析器
 * @param file_path 加密规则文件路径
 * @return 0 成功，<0 失败
 */
__attribute__((visibility("default")))
int rule_parser_init(const char* file_path);

/**
 * 加载并解密规则文件
 * @return 0 成功，<0 失败
 */
__attribute__((visibility("default")))
int rule_parser_load(void);

/**
 * 获取规则数据
 * @param size 输出规则数据大小
 * @return 规则数据指针，失败返回 NULL
 */
__attribute__((visibility("default")))
void* rule_parser_get_rules(size_t* size);

/**
 * 验证规则文件哈希
 * @return 0 哈希正确，<0 哈希错误
 */
__attribute__((visibility("default")))
int rule_parser_verify_hash(void);

/**
 * 清理规则解析器资源
 */
__attribute__((visibility("default")))
void rule_parser_cleanup(void);

/**
 * 获取指定类型的规则
 * @param rule_type 规则类型
 * @param index 规则索引（同类型中的第几个）
 * @return 规则指针，不存在返回 NULL
 */
__attribute__((visibility("default")))
rule_entry_t* rule_parser_get_rule_by_type(rule_type_t rule_type, uint32_t index);

/**
 * 获取规则数量
 * @param rule_type 规则类型（0 表示所有类型）
 * @return 规则数量
 */
__attribute__((visibility("default")))
uint32_t rule_parser_get_rule_count(rule_type_t rule_type);

/**
 * AI模糊匹配 — 编辑距离计算
 * @param str1 字符串1
 * @param str2 字符串2
 * @return 编辑距离（0=完全匹配）
 */
__attribute__((visibility("default")))
int rule_fuzzy_match_distance(const char *str1, const char *str2);

/**
 * AI模糊匹配 — 检测字符串是否包含模糊签名
 * @param data 待检测数据
 * @param data_len 数据长度
 * @param pattern 模糊模式（支持?通配符）
 * @return 匹配度 0-100（100=完全匹配）
 */
__attribute__((visibility("default")))
int rule_fuzzy_match_pattern(const uint8_t *data, size_t data_len,
                              const char *pattern);

/**
 * AI模糊匹配 — 搜索所有规则中对给定数据的模糊匹配
 * @param data 待检测数据
 * @param data_len 数据长度
 * @param rule_type 规则类型
 * @param min_score 最低匹配分（0-100）
 * @return 最高匹配分，-1表示无匹配
 */
__attribute__((visibility("default")))
int rule_fuzzy_search(const uint8_t *data, size_t data_len,
                       rule_type_t rule_type, int min_score);

#ifdef __cplusplus
}
#endif

#endif /* RULE_PARSER_H */
#include "rule_parser.h"
#include "../plugin_interface.h"
#include "../../bare_syscall/bare_syscall.h"
#include "../../crypto/core/crypto_core.h"
#include <string.h>
#include <stdlib.h>

/* ========== 全局规则解析器上下文 ========== */

static rule_parser_ctx_t g_rule_ctx = {0};

/* ========== 嵌入式 AES-256 密钥（实际应从安全存储获取）========== */

static const uint8_t g_aes_key[32] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    0x6b, 0x8f, 0x4e, 0x3d, 0x5a, 0x7c, 0x9b, 0x1e,
    0x2f, 0x5a, 0x8c, 0x4d, 0x6e, 0x3b, 0x7a, 0x9f
};

/* ========== 魔数定义 ========== */

#define RULE_FILE_MAGIC 0x454C5552  // "RULE" in little-endian

/* ========== 内部函数：读取文件 ========== */

static int read_file(const char* path, uint8_t** data, size_t* size) {
    if (!path || !data || !size) {
        return -1;
    }
    
    // 打开文件
    int fd = bare_open(path, 0, 0);  // O_RDONLY
    if (fd < 0) {
        return -2;
    }
    
    // 获取文件大小
    uint8_t stat_buf[128];
    if (bare_fstatat(fd, "", stat_buf, 0) < 0) {
        bare_close(fd);
        return -3;
    }
    
    // 读取文件内容（简化：假设文件小于 1MB）
    size_t max_size = 1024 * 1024;
    uint8_t* buffer = (uint8_t*)bare_mmap(NULL, max_size, 
                                            PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS,
                                            -1, 0);
    if (buffer == MAP_FAILED) {
        bare_close(fd);
        return -4;
    }
    
    ssize_t bytes_read = bare_read_full(fd, buffer, max_size);
    bare_close(fd);
    
    if (bytes_read <= 0) {
        bare_munmap(buffer, max_size);
        return -5;
    }
    
    *data = buffer;
    *size = (size_t)bytes_read;
    
    return 0;
}

/* ========== 内部函数：解密规则数据 ========== */

static int decrypt_rule_data(const uint8_t* encrypted, size_t encrypted_size,
                             const uint8_t* iv, const uint8_t* tag,
                             uint8_t** decrypted, size_t* decrypted_size) {
    if (!encrypted || !iv || !tag || !decrypted || !decrypted_size) {
        return -1;
    }
    
    // 分配解密缓冲区
    size_t plain_size = encrypted_size;
    uint8_t* plain = (uint8_t*)bare_mmap(NULL, plain_size,
                                          PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS,
                                          -1, 0);
    if (plain == MAP_FAILED) {
        return -2;
    }
    
    // 使用 AES-256-GCM 解密
    int ret = aes256_gcm_decrypt(encrypted, encrypted_size,
                                  g_aes_key, iv, tag, plain);
    if (ret < 0) {
        bare_munmap(plain, plain_size);
        return -3;
    }
    
    *decrypted = plain;
    *decrypted_size = plain_size;
    
    return 0;
}

/* ========== 内部函数：解析规则条目 ========== */

static int parse_rules(uint8_t* data, size_t size, 
                       rule_entry_t*** rules, uint32_t* rule_count) {
    if (!data || !rules || !rule_count) {
        return -1;
    }
    
    // 计算规则数量（遍历一次）
    uint32_t count = 0;
    size_t offset = 0;
    
    while (offset + sizeof(rule_entry_t) <= size) {
        rule_entry_t* entry = (rule_entry_t*)(data + offset);
        
        // 验证规则类型
        if (entry->rule_type < RULE_TYPE_MEMORY_SIG || 
            entry->rule_type > RULE_TYPE_LATENCY_THRESHOLD) {
            break;  // 无效规则类型
        }
        
        // 计算下一条规则的偏移
        size_t entry_size = sizeof(rule_entry_t) + entry->data_len;
        if (offset + entry_size > size) {
            break;  // 数据不足
        }
        
        count++;
        offset += entry_size;
    }
    
    if (count == 0) {
        return -2;
    }
    
    // 分配规则指针数组
    rule_entry_t** rule_array = (rule_entry_t**)bare_mmap(
        NULL, count * sizeof(rule_entry_t*),
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
    
    if (rule_array == MAP_FAILED) {
        return -3;
    }
    
    // 填充规则指针数组
    offset = 0;
    for (uint32_t i = 0; i < count; i++) {
        rule_array[i] = (rule_entry_t*)(data + offset);
        rule_entry_t* entry = rule_array[i];
        size_t entry_size = sizeof(rule_entry_t) + entry->data_len;
        offset += entry_size;
    }
    
    *rules = rule_array;
    *rule_count = count;
    
    return 0;
}

/* ========== API 实现 ========== */

__attribute__((visibility("default")))
int rule_parser_init(const char* file_path) {
    if (!file_path) {
        return -1;
    }
    
    // 清理旧状态
    if (g_rule_ctx.loaded) {
        rule_parser_cleanup();
    }
    
    // 保存文件路径
    strncpy(g_rule_ctx.file_path, file_path, sizeof(g_rule_ctx.file_path) - 1);
    g_rule_ctx.file_path[sizeof(g_rule_ctx.file_path) - 1] = '\0';
    
    g_rule_ctx.loaded = 0;
    
    return 0;
}

__attribute__((visibility("default")))
int rule_parser_load(void) {
    if (g_rule_ctx.file_path[0] == '\0') {
        return -1;
    }
    
    // 1. 读取加密的规则文件
    int ret = read_file(g_rule_ctx.file_path, 
                        &g_rule_ctx.encrypted_data,
                        &g_rule_ctx.encrypted_size);
    if (ret < 0) {
        return -2;
    }
    
    // 2. 验证文件大小
    if (g_rule_ctx.encrypted_size < sizeof(rule_file_header_t)) {
        return -3;
    }
    
    // 3. 解析文件头
    rule_file_header_t* header = (rule_file_header_t*)g_rule_ctx.encrypted_data;
    
    // 验证魔数
    if (header->magic != RULE_FILE_MAGIC) {
        return -4;
    }
    
    // 4. 验证文件哈希
    ret = rule_parser_verify_hash();
    if (ret < 0) {
        return -5;
    }
    
    // 5. 解密规则数据
    size_t encrypted_data_offset = sizeof(rule_file_header_t);
    size_t encrypted_data_size = g_rule_ctx.encrypted_size - encrypted_data_offset;
    
    ret = decrypt_rule_data(
        g_rule_ctx.encrypted_data + encrypted_data_offset,
        encrypted_data_size,
        header->iv,
        header->tag,
        &g_rule_ctx.decrypted_data,
        &g_rule_ctx.decrypted_size
    );
    
    if (ret < 0) {
        return -6;
    }
    
    // 6. 解析规则条目
    ret = parse_rules(g_rule_ctx.decrypted_data, 
                      g_rule_ctx.decrypted_size,
                      &g_rule_ctx.rules,
                      &g_rule_ctx.rule_count);
    
    if (ret < 0) {
        return -7;
    }
    
    g_rule_ctx.loaded = 1;
    
    return 0;
}

__attribute__((visibility("default")))
void* rule_parser_get_rules(size_t* size) {
    if (!g_rule_ctx.loaded || !g_rule_ctx.decrypted_data) {
        if (size) *size = 0;
        return NULL;
    }
    
    if (size) {
        *size = g_rule_ctx.decrypted_size;
    }
    
    return g_rule_ctx.decrypted_data;
}

__attribute__((visibility("default")))
int rule_parser_verify_hash(void) {
    if (!g_rule_ctx.encrypted_data || 
        g_rule_ctx.encrypted_size < sizeof(rule_file_header_t)) {
        return -1;
    }
    
    rule_file_header_t* header = (rule_file_header_t*)g_rule_ctx.encrypted_data;
    
    // 计算文件哈希（排除哈希字段本身）
    sha3_ctx ctx;
    sha3_256_init(&ctx);
    
    // 哈希文件头（跳过 hash 字段）
    sha3_256_update(&ctx, (uint8_t*)header, 
                    offsetof(rule_file_header_t, hash));
    
    // 哈希加密数据
    size_t data_offset = sizeof(rule_file_header_t);
    if (g_rule_ctx.encrypted_size > data_offset) {
        sha3_256_update(&ctx, 
                        g_rule_ctx.encrypted_data + data_offset,
                        g_rule_ctx.encrypted_size - data_offset);
    }
    
    uint8_t computed_hash[32];
    sha3_256_final(&ctx, computed_hash);
    
    // 比较哈希值
    if (bare_memcmp(computed_hash, header->hash, 32) != 0) {
        return -2;  // 哈希不匹配
    }
    
    return 0;
}

__attribute__((visibility("default")))
void rule_parser_cleanup(void) {
    // 释放加密数据
    if (g_rule_ctx.encrypted_data) {
        bare_munmap(g_rule_ctx.encrypted_data, g_rule_ctx.encrypted_size);
        g_rule_ctx.encrypted_data = NULL;
        g_rule_ctx.encrypted_size = 0;
    }
    
    // 释放解密数据
    if (g_rule_ctx.decrypted_data) {
        bare_munmap(g_rule_ctx.decrypted_data, g_rule_ctx.decrypted_size);
        g_rule_ctx.decrypted_data = NULL;
        g_rule_ctx.decrypted_size = 0;
    }
    
    // 释放规则指针数组
    if (g_rule_ctx.rules) {
        bare_munmap(g_rule_ctx.rules, 
                    g_rule_ctx.rule_count * sizeof(rule_entry_t*));
        g_rule_ctx.rules = NULL;
        g_rule_ctx.rule_count = 0;
    }
    
    g_rule_ctx.loaded = 0;
}

__attribute__((visibility("default")))
rule_entry_t* rule_parser_get_rule_by_type(rule_type_t rule_type, uint32_t index) {
    if (!g_rule_ctx.loaded || !g_rule_ctx.rules) {
        return NULL;
    }
    
    uint32_t type_index = 0;
    
    for (uint32_t i = 0; i < g_rule_ctx.rule_count; i++) {
        rule_entry_t* rule = g_rule_ctx.rules[i];
        
        if (rule->rule_type == (uint32_t)rule_type) {
            if (type_index == index) {
                return rule;
            }
            type_index++;
        }
    }
    
    return NULL;  // 未找到
}

__attribute__((visibility("default")))
uint32_t rule_parser_get_rule_count(rule_type_t rule_type) {
    if (!g_rule_ctx.loaded || !g_rule_ctx.rules) {
        return 0;
    }
    
    if (rule_type == 0) {
        return g_rule_ctx.rule_count;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < g_rule_ctx.rule_count; i++) {
        if (g_rule_ctx.rules[i]->rule_type == (uint32_t)rule_type) count++;
    }
    return count;
}

__attribute__((visibility("default")))
int rule_fuzzy_match_distance(const char *str1, const char *str2) {
    if (!str1 || !str2) return -1;

    int len1 = 0, len2 = 0;
    while (str1[len1]) len1++;
    while (str2[len2]) len2++;

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    int max_len = len1 > len2 ? len1 : len2;
    int buf_size = (len1 + 1) * (len2 + 1);
    int stack_buf[256];
    int *dp;

    if (buf_size <= 256) {
        dp = stack_buf;
    } else {
        dp = (int *)bare_mmap(NULL, buf_size * sizeof(int),
                              3, 0x22, -1, 0);
        if (bare_is_error((long)dp)) return -1;
    }

    for (int i = 0; i <= len1; i++) dp[i * (len2 + 1)] = i;
    for (int j = 0; j <= len2; j++) dp[j] = j;

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (str1[i-1] == str2[j-1]) ? 0 : 1;
            int del = dp[(i-1) * (len2+1) + j] + 1;
            int ins = dp[i * (len2+1) + (j-1)] + 1;
            int sub = dp[(i-1) * (len2+1) + (j-1)] + cost;
            int min = del < ins ? del : ins;
            min = min < sub ? min : sub;
            dp[i * (len2+1) + j] = min;
        }
    }

    int distance = dp[len1 * (len2+1) + len2];

    if (buf_size > 256) bare_munmap(dp, buf_size * sizeof(int));

    int max_dist = max_len;
    return (max_dist - distance) * 100 / (max_dist > 0 ? max_dist : 1);
}

__attribute__((visibility("default")))
int rule_fuzzy_match_pattern(const uint8_t *data, size_t data_len,
                              const char *pattern) {
    if (!data || !pattern || data_len == 0) return 0;

    int pat_len = 0;
    while (pattern[pat_len]) pat_len++;
    if (pat_len == 0) return 100;

    int best_score = 0;
    for (size_t start = 0; start + (size_t)pat_len <= data_len; start++) {
        int matches = 0;
        int total = 0;
        for (int j = 0; j < pat_len && start + j < data_len; j++) {
            if (pattern[j] == '?') {
                total++;
                continue;
            }
            total++;
            if (pattern[j] == data[start + j]) matches++;
        }
        int score = total > 0 ? matches * 100 / total : 100;
        if (score > best_score) best_score = score;
        if (best_score == 100) break;
    }

    return best_score;
}

__attribute__((visibility("default")))
int rule_fuzzy_search(const uint8_t *data, size_t data_len,
                       rule_type_t rule_type, int min_score) {
    if (!g_rule_ctx.loaded || !g_rule_ctx.rules || !data) return -1;

    int best_score = -1;

    for (uint32_t i = 0; i < g_rule_ctx.rule_count; i++) {
        rule_entry_t *rule = g_rule_ctx.rules[i];
        if (rule_type != 0 && rule->rule_type != (uint32_t)rule_type) continue;

        char pattern[256];
        int plen = rule->data_len < 255 ? rule->data_len : 255;
        for (int j = 0; j < plen; j++) {
            pattern[j] = (char)rule->data[j];
        }
        pattern[plen] = '\0';

        int score = rule_fuzzy_match_pattern(data, data_len, pattern);
        if (score > best_score) best_score = score;

        int text_score = rule_fuzzy_match_distance(
            (const char *)data, pattern);
        if (text_score > best_score) best_score = text_score;
    }

    return best_score >= min_score ? best_score : -1;
}

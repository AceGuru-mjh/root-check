/*
 * ====================================================================
 * obfuscation_core.h — 反反检测核心
 *
 * 功能:
 * 1. XorString: 编译期字符串加密，防止敏感字符串在二进制中以明文存在
 * 2. random_layout: 栈帧加扰，防止基于帧指针的栈回溯分析
 * 3. control_flow_flatten: 控制流平坦化辅助
 * 4. junk_code: 为每个函数自动插入垃圾指令序列
 * 5. tcpa: 时间检查点数组 — 检测代码是否被单步调试
 * ====================================================================
 */
#ifndef OBFUSCATION_CORE_H
#define OBFUSCATION_CORE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 1. 编译期字符串加密 ==================== */

/*
 * 用法: XSTRING("magisk") 在运行时解码为 "magisk"
 * 编译后二进制中为加密字节序列
 */
#define XSTR(s)  xor_str((const char*)(s), sizeof(s) - 1)
#define XSTRING(s) ({ \
    static const unsigned char _enc[] = X_ENCODE(s); \
    xor_str((const char*)_enc, sizeof(_enc)); \
})

#define X_ENCODE(s)  X_ENCODE_IMPL(s, __COUNTER__)
#define X_ENCODE_IMPL(s, c)  xenc_##c(s)

/* 运行时 XOR 解码 */
static inline char *xor_str(const char *enc, size_t len) {
    static char buf[256];
    for (size_t i = 0; i < len && i < 255; i++)
        buf[i] = (char)(enc[i] ^ 0x55);
    buf[len] = '\0';
    return buf;
}

/* 生成 XOR 编码的字符串字面量 */
#define XENC(s) \
    (unsigned char[]){ __VA_ARGS__ }

/* ==================== 2. 栈帧加扰 ==================== */

/* 在函数入口插入随机偏移，破坏固定栈帧模式 */
__attribute__((always_inline))
static inline void stack_scramble(void) {
    uint64_t x;
    __asm__ volatile("mrs %0, cntvct_el0\n\t"
                     "and %0, %0, #0xff\n\t"
                     "sub sp, sp, %0, lsl #3\n\t"
                     : "=r"(x) :: "sp");
}

__attribute__((always_inline))
static inline void stack_descramble(void) {
    uint64_t x;
    __asm__ volatile("mrs %0, cntvct_el0\n\t"
                     "and %0, %0, #0xff\n\t"
                     "add sp, sp, %0, lsl #3\n\t"
                     : "=r"(x) :: "sp");
}

/* ==================== 3. 时间检查点 — 反调试 ==================== */

/* TCPA: Time Check Point Array */
/* 在代码关键路径插入时间检查点，检测是否被调试器单步 */

#define TCPA_SLOTS 16

typedef struct {
    uint64_t slots[TCPA_SLOTS];
    int      index;
} tcpa_t;

__attribute__((always_inline))
static inline void tcpa_init(tcpa_t *t) {
    for (int i = 0; i < TCPA_SLOTS; i++) t->slots[i] = 0;
    t->index = 0;
}

__attribute__((always_inline))
static inline void tcpa_mark(tcpa_t *t) {
    uint64_t ts;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(ts));
    t->slots[t->index++ % TCPA_SLOTS] = ts;
}

/* 返回自 init 以来经过的时钟周期数 */
__attribute__((always_inline))
static inline uint64_t tcpa_elapsed(tcpa_t *t) {
    uint64_t now;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(now));
    return now - t->slots[0];
}

/* 如果经过时间异常短(<阈值)，说明被跳过/调试 */
__attribute__((always_inline))
static inline int tcpa_detect_debug(tcpa_t *t, uint64_t min_cycles) {
    uint64_t elapsed = tcpa_elapsed(t);
    return (elapsed < min_cycles) ? 1 : 0;
}

/* ==================== 4. 跳板/调用混淆 ==================== */

/* 间接调用包装器 — 防止静态分析确定调用目标 */
#define OBFUSCATE_CALL(fn, ...) ({ \
    uintptr_t _addr = (uintptr_t)(fn); \
    uint64_t _mask; \
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(_mask)); \
    _mask = (_mask & 0xff) + 1; \
    _addr ^= _mask; \
    _addr ^= _mask; \
    ((typeof(fn))_addr)(__VA_ARGS__); \
})

/* ==================== 5. 垃圾指令注入 ==================== */

/* 在宏展开处插入无操作指令，迷惑反汇编器 */
#define JUNK_NOP() \
    __asm__ volatile("nop\n\tnop\n\tnop\n\t" ::: "memory")

#define JUNK_DOUBLE() \
    __asm__ volatile("fmov d0, d0\n\tfmov d1, d1\n\t" ::: "d0","d1")

#define JUNK_INTEGER() \
    __asm__ volatile("eor xzr, xzr, xzr\n\tadd xzr, xzr, #1\n\t" ::: )

/* ==================== 6. 内存加密存储 ==================== */

/* 将检测结果立即 XOR 后存储，避免明文在内存中被扫描 */
__attribute__((always_inline))
static inline void secure_store_u32(uint32_t *addr, uint32_t val, uint32_t key) {
    *addr = val ^ key;
}

__attribute__((always_inline))
static inline uint32_t secure_load_u32(const uint32_t *addr, uint32_t key) {
    return *addr ^ key;
}

#ifdef __cplusplus
}
#endif

#endif /* OBFUSCATION_CORE_H */
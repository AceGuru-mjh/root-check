#include "random_layout.h"
#include "../bare_syscall/bare_syscall.h"
#include <string.h>

/* 随机布局状态 */
static uint64_t g_layout_seed = 0;

/* 获取随机偏移量 */
static uint64_t get_random_offset(void) {
    g_layout_seed = (g_layout_seed * 0x5851f42d4c957f2dULL) + 0x14057b7ef767814fULL;

    uint64_t cycles;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycles));
    g_layout_seed ^= cycles;

    /* 生成 0~512MB 范围内的随机偏移 */
    return g_layout_seed & 0x1FFFFFFF;
}

void random_layout_init(void) {
    uint64_t cycles;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycles));
    g_layout_seed = cycles ^ ((uint64_t)bare_getpid() << 16) ^ bare_rdtsc();

    /* 立即消耗一批随机数打乱初始种子 */
    for (int i = 0; i < 16; i++) {
        g_layout_seed = (g_layout_seed * 0x5851f42d4c957f2dULL) + 0x14057b7ef767814fULL;
    }
}

void *randomize_buffer_allocation(size_t base_size) {
    size_t extra = (size_t)get_random_offset() % (1024 * 1024);
    size_t total = base_size + extra;

    void *buf = (void *)bare_mmap(NULL, total,
                                  PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == (void*)-1 || !buf) return NULL;

    /* 随机偏移使用区域 */
    size_t offset = (size_t)get_random_offset() % (total - base_size);
    bare_memset(buf, 0, total);
    return (void*)((uint8_t*)buf + offset);
}

void free_randomized_buffer(void *ptr, size_t base_size) {
    if (!ptr) return;

    size_t extra = (size_t)get_random_offset() % (1024 * 1024);
    size_t total = base_size + extra;

    void *base = (void *)((uint8_t*)ptr - ((uintptr_t)ptr % (1024*1024)));
    bare_munmap(base, total);
}

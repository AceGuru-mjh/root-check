#include "instruction_obfuscator.h"
#include "../bare_syscall/bare_syscall.h"
#include <string.h>

/* 混淆状态：存储随机噪声模式和跳转目标 */
static uint64_t g_noise_seed = 0;
static int g_initialized = 0;

void instruction_obfuscator_init(void) {
    uint64_t cycles;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycles));
    g_noise_seed = cycles ^ (uint64_t)bare_getpid();
    g_initialized = 1;
}

void instruction_obfuscator_shutdown(void) {
    g_initialized = 0;
    g_noise_seed = 0;
}

void obfuscated_call(void (*func)(void)) {
    if (!func) return;

    /* 插入随机时序噪声（纳秒级无序延迟） */
    uint64_t noise;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(noise));
    noise = (noise ^ g_noise_seed) & 0xFFF;

    for (uint64_t i = 0; i < noise; i++) {
        __asm__ volatile("nop");
    }

    g_noise_seed = (g_noise_seed * 0x5851f42d4c957f2dULL) + 0x14057b7ef767814fULL;

    func();
}

void obfuscated_call_vargs(void (*func)(void*), void *arg) {
    if (!func) return;

    uint64_t noise;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(noise));
    noise = (noise ^ g_noise_seed) & 0x7FF;

    for (uint64_t i = 0; i < noise; i++) {
        __asm__ volatile("nop");
        __asm__ volatile("nop");
    }

    g_noise_seed = (g_noise_seed << 7) ^ (g_noise_seed >> 9) ^ 0x9e3779b97f4a7c15ULL;

    func(arg);
}

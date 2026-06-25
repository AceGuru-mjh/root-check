#ifndef MICRO_KERNEL_H
#define MICRO_KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include "plugin_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 微内核配置 ========== */

#define MAX_PLUGINS 12
#define PLUGIN_PATH_MAX 256
#define WORK_BUFFER_SIZE (1024 * 1024)  // 1MB per plugin
#define MIN_SLEEP_NS 100
#define MAX_SLEEP_NS 5000

/* ========== 插件句柄 ========== */

typedef struct {
    void* dl_handle;                    // dlopen 句柄
    char path[PLUGIN_PATH_MAX];         // 插件路径
    plugin_info_t* interface;           // 插件接口指针
    uint8_t* isolated_buffer;           // 隔离内存缓冲区
    size_t buffer_size;
    int cpu_core;                       // 绑定的 CPU 核心
    int loaded;                         // 是否已加载
} plugin_handle_t;

/* ========== 微内核上下文 ========== */

typedef struct {
    plugin_handle_t plugins[MAX_PLUGINS];
    uint32_t plugin_count;
    uint32_t execution_order[MAX_PLUGINS];  // 乱序执行顺序
    int ipc_server_fd;                      // IPC 服务器文件描述符
    int ipc_client_fd;                      // 连接到守护进程的客户端
    const void* rule_data;                  // 解密后的规则数据
    size_t rule_data_size;
    int initialized;
} micro_kernel_ctx_t;

/* ========== 微内核 API ========== */

/**
 * 初始化微内核
 * @param ctx 微内核上下文
 * @param ipc_server_path IPC 服务器套接字路径
 * @param daemon_ipc_path 守护进程 IPC 路径
 * @param rule_file_path 加密规则文件路径
 * @return 0 成功，<0 失败
 */
__attribute__((visibility("default")))
int micro_kernel_init(micro_kernel_ctx_t* ctx,
                      const char* ipc_server_path,
                      const char* daemon_ipc_path,
                      const char* rule_file_path);

/**
 * 运行扫描任务
 * @param ctx 微内核上下文
 * @param task 扫描任务
 * @param report 输出报告
 * @return 0 成功，<0 失败
 */
__attribute__((visibility("default")))
int micro_kernel_run_scan(micro_kernel_ctx_t* ctx,
                          const scan_task_t* task,
                          global_secure_report_t* report);

/**
 * 清理微内核资源
 * @param ctx 微内核上下文
 */
__attribute__((visibility("default")))
void micro_kernel_cleanup(micro_kernel_ctx_t* ctx);

/* ========== 内部函数（供测试使用）========== */

/**
 * 加载插件
 * @param ctx 微内核上下文
 * @param layer_id 层级 ID (1-7)
 * @param plugin_path 插件路径
 * @return 0 成功，<0 失败
 */
int load_plugin(micro_kernel_ctx_t* ctx, uint32_t layer_id, const char* plugin_path);

/**
 * 卸载插件
 * @param ctx 微内核上下文
 * @param layer_id 层级 ID
 * @return 0 成功，<0 失败
 */
int unload_plugin(micro_kernel_ctx_t* ctx, uint32_t layer_id);

/**
 * Fisher-Yates 洗牌算法生成乱序执行顺序
 * @param order 输出顺序数组
 * @param count 元素数量
 */
void shuffle_execution_order(uint32_t* order, uint32_t count);

/**
 * 纳秒级随机睡眠
 * @param min_ns 最小纳秒
 * @param max_ns 最大纳秒
 */
void random_sleep_ns(uint64_t min_ns, uint64_t max_ns);

/**
 * 绑定到随机 CPU 核心
 * @return CPU 核心 ID
 */
int bind_random_cpu(void);

/**
 * 分配隔离内存缓冲区
 * @param size 缓冲区大小
 * @return 缓冲区指针，失败返回 NULL
 */
void* allocate_isolated_buffer(size_t size);

/**
 * 释放隔离内存缓冲区
 * @param buffer 缓冲区指针
 * @param size 缓冲区大小
 */
void free_isolated_buffer(void* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MICRO_KERNEL_H */

#include "micro_kernel.h"
#include "../bare_syscall/bare_syscall.h"
#include "../bare_syscall/cpu_control.h"
#include "../crypto/core/crypto_core.h"
#include "../ipc/ipc.h"
#include "rule_parser/rule_parser.h"
#include "plugin_interface.h"

#include <dlfcn.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* ========== 硬件随机数生成 ========== */

/**
 * 从硬件随机数生成器获取随机数
 * Android 支持 getrandom() 系统调用 (ARM64: __NR_getrandom = 278)
 */
static uint64_t get_hw_random(void) {
    uint64_t random_val = 0;
    
    // 使用 getrandom 系统调用获取硬件随机数
    long ret = bare_syscall(278, &random_val, sizeof(random_val), 0);
    if (ret < 0) {
        // 回退：使用 CPU 时间戳 + PID 作为熵源
        random_val = bare_rdtsc() ^ ((uint64_t)bare_getpid() << 32);
    }
    
    return random_val;
}

/* ========== Fisher-Yates 洗牌算法 ========== */

void shuffle_execution_order(uint32_t* order, uint32_t count) {
    if (!order || count == 0) return;
    
    // 初始化顺序数组
    for (uint32_t i = 0; i < count; i++) {
        order[i] = i;
    }
    
    // Fisher-Yates 洗牌
    for (uint32_t i = count - 1; i > 0; i--) {
        uint64_t rand_val = get_hw_random();
        uint32_t j = (uint32_t)(rand_val % (i + 1));
        
        // 交换 order[i] 和 order[j]
        uint32_t temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    }
}

/* ========== 纳秒级随机睡眠 ========== */

void random_sleep_ns(uint64_t min_ns, uint64_t max_ns) {
    uint64_t range = max_ns - min_ns;
    uint64_t rand_val = get_hw_random();
    uint64_t sleep_ns = min_ns + (rand_val % range);
    
    struct timespec ts;
    ts.tv_sec = sleep_ns / 1000000000ULL;
    ts.tv_nsec = sleep_ns % 1000000000ULL;
    
    nanosleep(&ts, NULL);
}

/* ========== CPU 绑定 ========== */

int bind_random_cpu(void) {
    int cpu_count = bare_get_cpu_count();
    if (cpu_count <= 0) {
        cpu_count = 4; // 默认值
    }
    
    uint64_t rand_val = get_hw_random();
    int cpu_id = (int)(rand_val % cpu_count);
    
    // 使用裸系统调用绑定 CPU
    int ret = bare_bind_cpu(cpu_id);
    if (ret < 0) {
        return -1;
    }
    
    return cpu_id;
}

/* ========== 隔离内存分配器 ========== */

void* allocate_isolated_buffer(size_t size) {
    // 使用 mmap 分配匿名私有内存
    void* buffer = (void*)bare_mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    
    if (buffer == MAP_FAILED || buffer == NULL) {
        return NULL;
    }
    
    // 清零内存
    bare_memset(buffer, 0, size);
    
    return buffer;
}

void free_isolated_buffer(void* buffer, size_t size) {
    if (buffer && buffer != MAP_FAILED) {
        bare_munmap(buffer, size);
    }
}

/* ========== 插件加载器 ========== */

int load_plugin(micro_kernel_ctx_t* ctx, uint32_t layer_id, const char* plugin_path) {
    if (!ctx || !plugin_path || layer_id == 0 || layer_id > MAX_PLUGINS) {
        return -1;
    }
    
    // 检查是否已加载
    if (ctx->plugins[layer_id - 1].loaded) {
        return 0; // 已加载
    }
    
    // dlopen 加载插件
    void* handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        return -2; // 加载失败
    }
    
    // 获取插件接口符号 (新标准: register_plugin)
    plugin_info_t* (*reg_fn)(void);
    reg_fn = (plugin_info_t* (*)(void))dlsym(handle, "register_plugin");
    if (!reg_fn) {
        dlclose(handle);
        return -3; // 符号未找到
    }
    plugin_info_t* info = reg_fn();
    if (!info || info->layer_id != layer_id) {
        dlclose(handle);
        return -4; // 层级不匹配
    }
    
    // 填充插件句柄
    plugin_handle_t* plugin = &ctx->plugins[layer_id - 1];
    plugin->dl_handle = handle;
    strncpy(plugin->path, plugin_path, PLUGIN_PATH_MAX - 1);
    plugin->path[PLUGIN_PATH_MAX - 1] = '\0';
    plugin->interface = info;
    plugin->isolated_buffer = NULL;
    plugin->buffer_size = 0;
    plugin->cpu_core = -1;
    plugin->loaded = 1;
    
    ctx->plugin_count++;
    
    return 0;
}

int unload_plugin(micro_kernel_ctx_t* ctx, uint32_t layer_id) {
    if (!ctx || layer_id == 0 || layer_id > MAX_PLUGINS) {
        return -1;
    }
    
    plugin_handle_t* plugin = &ctx->plugins[layer_id - 1];
    if (!plugin->loaded) {
        return 0; // 未加载
    }
    
    // 调用插件清理函数
    if (plugin->interface && plugin->interface->cleanup) {
        plugin->interface->cleanup();
    }
    
    // 释放隔离缓冲区
    if (plugin->isolated_buffer) {
        free_isolated_buffer(plugin->isolated_buffer, plugin->buffer_size);
        plugin->isolated_buffer = NULL;
    }
    
    // 关闭动态库
    if (plugin->dl_handle) {
        dlclose(plugin->dl_handle);
        plugin->dl_handle = NULL;
    }
    
    plugin->loaded = 0;
    ctx->plugin_count--;
    
    return 0;
}

/* ========== 根据检测级别加载插件 ========== */

static int load_plugins_by_level(micro_kernel_ctx_t* ctx, detect_level_t level) {
    if (!ctx) return -1;
    
    // 构建插件路径
    char plugin_path[PLUGIN_PATH_MAX];
    
    // 根据级别确定需要加载的层级
    uint32_t layers_to_load[MAX_PLUGINS];
    uint32_t layer_count = 0;
    
    switch (level) {
        case DETECT_LEVEL_QUICK:
            // 快速检测：L1-L3
            for (uint32_t i = 1; i <= 3; i++)
                layers_to_load[layer_count++] = i;
            break;
            
        case DETECT_LEVEL_STANDARD:
            // 标准检测：L1-L7
            for (uint32_t i = 1; i <= 7; i++)
                layers_to_load[layer_count++] = i;
            break;
            
        case DETECT_LEVEL_DEEP:
            // 深度检测：L1-L12 (通用 + 专项)
            for (uint32_t i = 1; i <= MAX_PLUGINS; i++)
                layers_to_load[layer_count++] = i;
            break;
            
        case DETECT_LEVEL_FORENSIC:
            // 取证检测：L1-L12 全部 + 每个插件多轮检测
            for (uint32_t i = 1; i <= MAX_PLUGINS; i++)
                layers_to_load[layer_count++] = i;
            break;
            
        default:
            return -1;
    }
    
    // 加载每个层级的插件
    for (uint32_t i = 0; i < layer_count; i++) {
        uint32_t layer_id = layers_to_load[i];
        
        // 构建插件路径（匹配 CMake OUTPUT_NAME: libguard_plugin_XX.so）
        snprintf(plugin_path, PLUGIN_PATH_MAX, 
                 "/data/local/tmp/plugins/libguard_plugin_%02u.so", layer_id);
        
        int ret = load_plugin(ctx, layer_id, plugin_path);
        if (ret < 0) {
            // 加载失败，继续尝试其他插件
            continue;
        }
    }
    
    return (ctx->plugin_count > 0) ? 0 : -2;
}

/* ========== 执行单个插件 ========== */

static int execute_plugin(micro_kernel_ctx_t* ctx, uint32_t layer_id, 
                          plugin_result_t* result) {
    if (!ctx || !result || layer_id == 0 || layer_id > MAX_PLUGINS) {
        return -1;
    }
    
    plugin_handle_t* plugin = &ctx->plugins[layer_id - 1];
    if (!plugin->loaded || !plugin->interface) {
        return -2;
    }
    
    // 1. 绑定 CPU
    int cpu_id = bind_random_cpu();
    if (cpu_id >= 0) {
        plugin->cpu_core = cpu_id;
    }
    
    // 2. 分配隔离缓冲区
    plugin->isolated_buffer = (uint8_t*)allocate_isolated_buffer(WORK_BUFFER_SIZE);
    if (!plugin->isolated_buffer) {
        return -3;
    }
    plugin->buffer_size = WORK_BUFFER_SIZE;
    
    // 3. 构造上下文
    plugin_context_t plugin_ctx;
    plugin_ctx.rule_data        = ctx->rule_data;
    plugin_ctx.rule_data_size   = ctx->rule_data_size;
    plugin_ctx.work_buffer      = plugin->isolated_buffer;
    plugin_ctx.work_buffer_size = plugin->buffer_size;
    plugin_ctx.task_random_salt = 0;
    plugin_ctx.cpu_core         = -1;
    
    // 4. 初始化插件
    if (plugin->interface->init) {
        int ret = plugin->interface->init(ctx->rule_data, ctx->rule_data_size);
        if (ret < 0) {
            free_isolated_buffer(plugin->isolated_buffer, plugin->buffer_size);
            plugin->isolated_buffer = NULL;
            return -4;
        }
    }
    
    // 5. 执行检测（结果直接写入 result 结构）
    bare_memset(result, 0, sizeof(plugin_result_t));
    int ret = plugin->interface->detect(&plugin_ctx, result);
    if (ret < 0) {
        free_isolated_buffer(plugin->isolated_buffer, plugin->buffer_size);
        plugin->isolated_buffer = NULL;
        return -5;
    }
    result->layer_id = layer_id;
    
    // 6. 结果哈希验签（SHA3-512，域C校验）
    sha3_512((const uint8_t*)result,
             (size_t)((uint8_t*)&result->hash - (uint8_t*)result),
             result->hash);
    
    // 7. 释放隔离缓冲区
    free_isolated_buffer(plugin->isolated_buffer, plugin->buffer_size);
    plugin->isolated_buffer = NULL;
    plugin->buffer_size = 0;
    
    return 0;
}

/* ========== 发送结果到守护进程签名 ========== */

static int send_result_to_daemon(micro_kernel_ctx_t* ctx, plugin_result_t* result) {
    if (!ctx || !result || ctx->ipc_client_fd < 0) {
        return -1;
    }
    
    // 构造 IPC 消息
    ipc_session_t session;
    bare_memset(&session, 0, sizeof(session));
    session.sock_fd = ctx->ipc_client_fd;
    
    // 发送插件结果
    int ret = ipc_send_message(
        ctx->ipc_client_fd,
        &session,
        IPC_MSG_PLUGIN_RESULT,
        (uint8_t*)result,
        sizeof(plugin_result_t)
    );
    
    return ret;
}

/* ========== 组装全局安全报告 ========== */

static int assemble_report(micro_kernel_ctx_t* ctx, const scan_task_t* task,
                          plugin_result_t* results, uint32_t result_count,
                          global_secure_report_t* report) {
    if (!ctx || !task || !results || !report) {
        return -1;
    }
    
    bare_memset(report, 0, sizeof(global_secure_report_t));
    
    // 填充报告头部（使用单调时钟，防止时间篡改）
    report->report_id = (uint32_t)bare_rdtsc();
    report->task_id = task->task_id;
    report->timestamp_ns = get_time_ns();
    report->layer_count = result_count;
    
    // 复制各层结果
    for (uint32_t i = 0; i < result_count && i < MAX_LAYERS; i++) {
        bare_memcpy(&report->layer_results[i], &results[i], sizeof(plugin_result_t));
    }
    
    // 计算报告整体聚合哈希（SHA3-512）
    sha3_ctx sha3_ctx;
    sha3_512_init(&sha3_ctx);
    sha3_512_update(&sha3_ctx, (uint8_t*)report, 
                    offsetof(global_secure_report_t, aggregated_hash));
    sha3_512_final(&sha3_ctx, report->aggregated_hash);
    
    return 0;
}

/* ========== 微内核主 API ========== */

int micro_kernel_init(micro_kernel_ctx_t* ctx,
                      const char* ipc_server_path,
                      const char* daemon_ipc_path,
                      const char* rule_file_path) {
    if (!ctx) {
        return -1;
    }
    
    bare_memset(ctx, 0, sizeof(micro_kernel_ctx_t));
    ctx->ipc_server_fd = -1;
    ctx->ipc_client_fd = -1;
    
    // 1. 初始化 IPC 服务器（监听 UI 命令）
    ctx->ipc_server_fd = ipc_server_start(ipc_server_path);
    if (ctx->ipc_server_fd < 0) {
        return -2;
    }
    
    // 2. 初始化 IPC 客户端（连接到守护进程）
    ctx->ipc_client_fd = ipc_client_connect(daemon_ipc_path);
    if (ctx->ipc_client_fd < 0) {
        ipc_server_stop(ctx->ipc_server_fd);
        return -3;
    }
    
    // 3. 加载并解密规则文件
    int ret = rule_parser_init(rule_file_path);
    if (ret < 0) {
        ipc_client_disconnect(ctx->ipc_client_fd);
        ipc_server_stop(ctx->ipc_server_fd);
        return -4;
    }
    
    ret = rule_parser_load();
    if (ret < 0) {
        ipc_client_disconnect(ctx->ipc_client_fd);
        ipc_server_stop(ctx->ipc_server_fd);
        return -5;
    }
    
    ctx->rule_data = rule_parser_get_rules(&ctx->rule_data_size);
    if (!ctx->rule_data) {
        ipc_client_disconnect(ctx->ipc_client_fd);
        ipc_server_stop(ctx->ipc_server_fd);
        return -6;
    }
    
    ctx->initialized = 1;
    
    return 0;
}

int micro_kernel_run_scan(micro_kernel_ctx_t* ctx,
                          const scan_task_t* task,
                          global_secure_report_t* report) {
    if (!ctx || !task || !report || !ctx->initialized) {
        return -1;
    }
    
    // 1. 根据检测级别加载插件
    int ret = load_plugins_by_level(ctx, task->level);
    if (ret < 0) {
        return -2;
    }
    
    // 2. 生成乱序执行顺序
    shuffle_execution_order(ctx->execution_order, ctx->plugin_count);
    
    // 3. 执行插件（乱序）
    plugin_result_t results[MAX_PLUGINS];
    uint32_t result_count = 0;
    
    for (uint32_t i = 0; i < ctx->plugin_count; i++) {
        uint32_t layer_idx = ctx->execution_order[i];
        uint32_t layer_id = layer_idx + 1; // 转换为 1-based
        
        // 执行插件
        ret = execute_plugin(ctx, layer_id, &results[result_count]);
        if (ret == 0) {
            // 发送结果到守护进程签名
            send_result_to_daemon(ctx, &results[result_count]);
            result_count++;
        }
        
        // 插入随机睡眠（最后一个插件后不睡眠）
        if (i < ctx->plugin_count - 1) {
            random_sleep_ns(MIN_SLEEP_NS, MAX_SLEEP_NS);
        }
    }
    
    // 4. 组装全局安全报告并签名
    ret = assemble_report(ctx, task, results, result_count, report);
    if (ret < 0) {
        return -3;
    }

    // 5. 发送完整签名报告到域C守护进程
    if (ctx->ipc_client_fd >= 0) {
        ipc_session_t session;
        bare_memset(&session, 0, sizeof(session));
        session.sock_fd = ctx->ipc_client_fd;
        ipc_send_message(ctx->ipc_client_fd, &session,
                         IPC_MSG_DETECT_RESULT,
                         (uint8_t*)report,
                         sizeof(global_secure_report_t));
    }
    
    // 6. 卸载所有插件
    for (uint32_t i = 0; i < MAX_PLUGINS; i++) {
        if (ctx->plugins[i].loaded) {
            unload_plugin(ctx, i + 1);
        }
    }
    
    return 0;
}

void micro_kernel_cleanup(micro_kernel_ctx_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // 卸载所有插件
    for (uint32_t i = 0; i < MAX_PLUGINS; i++) {
        if (ctx->plugins[i].loaded) {
            unload_plugin(ctx, i + 1);
        }
    }
    
    // 关闭 IPC 连接
    if (ctx->ipc_client_fd >= 0) {
        ipc_client_disconnect(ctx->ipc_client_fd);
        ctx->ipc_client_fd = -1;
    }
    
    if (ctx->ipc_server_fd >= 0) {
        ipc_server_stop(ctx->ipc_server_fd);
        ctx->ipc_server_fd = -1;
    }
    
    // 清理规则解析器
    rule_parser_cleanup();
    
    ctx->rule_data = NULL;
    ctx->rule_data_size = 0;
    ctx->initialized = 0;
}

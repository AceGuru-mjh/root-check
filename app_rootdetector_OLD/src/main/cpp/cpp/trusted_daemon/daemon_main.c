#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "../bare_syscall/bare_syscall.h"
#include "../crypto/core/crypto_core.h"
#include "../ipc/ipc.h"
#include "../sandbox_microkernel/plugin_interface.h"
#include "memory_checker.h"
#include "anti_debug.h"
#include "sign_center.h"
#include "kernel_probe.h"
#include "self_verify.h"
#include "instruction_obfuscator.h"
#include "anti_hook.h"
#include "random_layout.h"
#include "../bare_syscall/bare_syscall.h"

static int g_running = 1;
static ipc_session_t g_session;
static uint64_t g_start_time_ns = 0;
static int g_tamper_count = 0;
static int g_alert_count = 0;

/* 从硬件熵源派生设备唯一密钥 */
static void derive_device_key(uint8_t key[32]) {
    uint8_t entropy[64];
    /* 混合多个熵源 */
    uint64_t ts = get_time_ns();
    bare_memcpy(entropy, &ts, 8);
    bare_rdtsc();
    for (int i = 8; i < 64; i += 8) {
        uint64_t val = bare_rdtsc();
        bare_memcpy(entropy + i, &val, 8);
    }
    /* 混入内核随机数 */
    long ret = bare_syscall(278, entropy, 64, 0);  /* getrandom */
    (void)ret;
    sha3_256(entropy, 64, key);
    bare_memset(entropy, 0, 64);
}

void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

/* 域C可信守护进程：独立常驻进程，实时监控域A/域B内存完整性
 * 核心职能：
 *   1. 跨域内存完整性周期性扫描
 *   2. 反调试 & 进程注入拦截
 *   3. 全局验签中心 — 所有检测结果SHA3-512签名
 *   4. 内核轻量探针 — kallsyms/modules周期性校验
 */
int main(int argc, char *argv[]) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    daemon(0, 0);

    g_start_time_ns = get_time_ns();

    anti_debug_init();
    instruction_obfuscator_init();
    random_layout_init();

    if (!self_verify_integrity()) {
        return 1;
    }

    /* 初始化签名中心（设备唯一密钥从硬件熵源派生）*/
    uint8_t device_key[32];
    uint8_t device_id[32];
    derive_device_key(device_key);
    bare_memset(device_id, 0, 32);
    sign_center_init(device_key, device_id);
    bare_memset(device_key, 0, 32);

    kernel_probe_init();

    /* 启动IPC服务器（域B→域C 双向加密管道）*/
    int server_fd = ipc_server_start(IPC_DEFAULT_SOCKET_PATH);
    if (server_fd < 0) return 1;

    int check_counter = 0;
    while (g_running) {
        int client_fd = ipc_server_accept(server_fd);
        if (client_fd < 0) continue;

        if (ipc_handshake_server(client_fd, &g_session) != 0) {
            close(client_fd);
            continue;
        }

        ipc_message_t msg;
        while (g_running && ipc_recv_message(client_fd, &g_session, &msg) == 0) {
            ipc_message_t resp;
            memset(&resp, 0, sizeof(resp));

            switch (msg.header.type) {
                case IPC_MSG_DETECT_REQUEST: {
                    plugin_result_t det_result;
                    memset(&det_result, 0, sizeof(det_result));
                    kernel_probe_check(&det_result);
                    memory_checker_scan(&det_result);
                    anti_hook_check(&det_result);

                    /* 签名结果 */
                    sign_center_sign((const uint8_t *)&det_result,
                                     sizeof(det_result),
                                     det_result.hash);

                    ipc_send_message(client_fd, &g_session,
                                     IPC_MSG_DETECT_RESULT,
                                     (uint8_t *)&det_result,
                                     sizeof(det_result));
                    break;
                }

                case IPC_MSG_PLUGIN_RESULT: {
                    /* 接收域B推送的单层检测结果，验签后存储*/
                    plugin_result_t *r = (plugin_result_t *)msg.payload;
                    if (sign_center_verify((const uint8_t *)r,
                                           sizeof(plugin_result_t),
                                           r->hash)) {
                        /* 签名验证通过，结果可信 */
                    } else {
                        g_tamper_count++;
                    }
                    break;
                }

                case IPC_MSG_DETECT_RESULT: {
                    /* 接收域B推送的完整安全报告，全局验签后记录*/
                    global_secure_report_t *report =
                        (global_secure_report_t *)msg.payload;
                    /* 验证聚合哈希 */
                    uint8_t hash[64];
                    sha3_ctx sha3_ctx;
                    sha3_512_init(&sha3_ctx);
                    sha3_512_update(&sha3_ctx, (uint8_t *)report,
                        offsetof(global_secure_report_t, aggregated_hash));
                    sha3_512_final(&sha3_ctx, hash);
                    if (bare_memcmp(hash, report->aggregated_hash, 64) == 0) {
                        /* 报告完整性验证通过 */
                    } else {
                        g_tamper_count++;
                    }
                    break;
                }

                case IPC_MSG_HEARTBEAT:
                    ipc_send_message(client_fd, &g_session,
                                     IPC_MSG_HEARTBEAT, NULL, 0);
                    break;

                case IPC_MSG_SHUTDOWN:
                    g_running = 0;
                    break;

                default:
                    break;
            }
        }
        close(client_fd);

        /* 周期性自校验和内核探针 */
        check_counter++;
        if (check_counter % 10 == 0) {
            if (!self_verify_integrity()) {
                g_tamper_count++;
            }
            if (kernel_probe_cycle() > 0) {
                g_alert_count++;
            }
        }
    }

    ipc_server_stop(server_fd);
    instruction_obfuscator_shutdown();
    return 0;
}

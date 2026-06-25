#include "micro_kernel.h"
#include "../bare_syscall/bare_syscall.h"
#include "../crypto/core/crypto_core.h"
#include "../ipc/ipc.h"
#include <signal.h>
#include <string.h>

static volatile int g_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 域B沙箱检测执行域入口
     *
     * 架构角色：
     *   1. fork独立分离进程，无ART依赖，纯Native沙箱
     *   2. 微内核动态加载7层检测插件（L1~L7独立SO）
     *   3. 每层结果独立内存隔离，互不污染
     *   4. 结果实时加密推送域C守护进程验签
     *
     * 通信路径：
     *   域A(UI) -> Unix Socket -> 域B(本进程)
     *   域B(本进程) -> Unix Socket -> 域C(可信守护)
     */
    const char *ipc_server_path = "/data/local/tmp/.rootguard_sandbox";
    const char *daemon_path     = IPC_DEFAULT_SOCKET_PATH;
    const char *rule_path       = "/data/data/com.rootguard/cache/rules.enc";

    micro_kernel_ctx_t ctx;
    if (micro_kernel_init(&ctx, ipc_server_path, daemon_path, rule_path) != 0)
        return 1;

    int server_fd = ctx.ipc_server_fd;
    int daemon_fd = ctx.ipc_client_fd;

    while (g_running) {
        int client_fd = ipc_server_accept(server_fd);
        if (client_fd < 0) continue;

        ipc_session_t session;
        memset(&session, 0, sizeof(session));
        session.sock_fd = client_fd;

        ipc_message_t msg;
        while (g_running && ipc_recv_message(client_fd, &session, &msg) == 0) {
            if (msg.header.type == IPC_MSG_HEARTBEAT) {
                ipc_send_message(client_fd, &session,
                                 IPC_MSG_HEARTBEAT, NULL, 0);
                continue;
            }

            if (msg.header.type != IPC_MSG_DETECT_REQUEST)
                continue;

            scan_task_t task;
            memset(&task, 0, sizeof(task));
            if (msg.header.payload_len >= sizeof(scan_task_t))
                memcpy(&task, msg.payload, sizeof(scan_task_t));

            // 使用客户端请求的检测级别和掩码，不覆写
            if (task.level == 0) task.level = DETECT_LEVEL_STANDARD;
            if (task.plugin_mask == 0) task.plugin_mask = 0x3E;

            global_secure_report_t report;
            memset(&report, 0, sizeof(report));

            int ret = micro_kernel_run_scan(&ctx, &task, &report);
            if (ret == 0) {
                ipc_send_message(client_fd, &session,
                                 IPC_MSG_DETECT_RESULT,
                                 (uint8_t *)&report,
                                 sizeof(report));
            }
        }
        close(client_fd);
    }

    micro_kernel_cleanup(&ctx);
    return 0;
}

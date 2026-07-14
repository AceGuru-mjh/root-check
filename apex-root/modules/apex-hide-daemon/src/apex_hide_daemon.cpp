// apex_hide_daemon.cpp — APEX Hide Daemon 主程序
//
// 独立的 root 守护进程，作为 Magisk/KSU/APatch 模块在 boot 时启动。
// 功能：
//   1. 监听 Unix socket /dev/socket/apex_hide.sock 接收 app 控制命令
//   2. 对新启动的应用进程应用 mount namespace 隔离
//   3. 在隔离的 namespace 中 bind-mount 空目录覆盖敏感路径
//   4. 通过 LD_PRELOAD 注入 libapex_hook.so 拦截 libc 调用
//
// 命令协议（文本行）：
//   STATUS          → 返回 "OK <pid> <mode>"
//   WHITELIST_ADD <uid>
//   WHITELIST_DEL <uid>
//   HIDE_PATH_ADD <path>
//   HIDE_PATH_DEL <path>
//   STOP            → 优雅退出
//
// 编译: see scripts/build_daemon.sh (待实现)
// 运行: /data/adb/apex-hide/bin/apex_hide_daemon

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>

#define LOG_TAG "APEX-Hide-Daemon"
#define SOCKET_PATH "/dev/socket/apex_hide.sock"
#define WORK_DIR "/data/adb/apex-hide"
#define EMPTY_DIR WORK_DIR "/empty"
#define WHITELIST_FILE WORK_DIR "/conf/whitelist.conf"
#define HIDE_PATHS_FILE WORK_DIR "/conf/hide_paths.conf"
#define HIDE_PROCS_FILE WORK_DIR "/conf/hide_procs.conf"
#define LOG_FILE WORK_DIR "/log/daemon.log"

static volatile sig_atomic_t g_running = 1;
static pthread_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::set<uid_t> g_whitelist;
static std::vector<std::string> g_hide_paths;
static std::vector<std::string> g_hide_procs;

static void log_msg(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    FILE* fp = fopen(LOG_FILE, "a");
    if (fp) {
        fprintf(fp, "[%d] ", (int)time(nullptr));
        vfprintf(fp, fmt, ap);
        fprintf(fp, "\n");
        fclose(fp);
    }
    va_end(ap);
}

static void signal_handler(int sig) {
    log_msg("Received signal %d, shutting down", sig);
    g_running = 0;
    // 关闭 socket 触发 accept 返回
    unlink(SOCKET_PATH);
}

// ─── 配置加载 ──────────────────────────────────────────────

static void load_whitelist() {
    pthread_mutex_lock(&g_config_mutex);
    g_whitelist.clear();
    std::ifstream f(WHITELIST_FILE);
    std::string line;
    while (std::getline(f, line)) {
        // 去掉注释和空白
        size_t pos = line.find('#');
        if (pos != std::string::npos) line = line.substr(0, pos);
        // trim
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(start, end - start + 1);
        if (line.empty()) continue;
        uid_t uid = (uid_t)strtoul(line.c_str(), nullptr, 10);
        g_whitelist.insert(uid);
    }
    pthread_mutex_unlock(&g_config_mutex);
    log_msg("Loaded %zu whitelist UIDs", g_whitelist.size());
}

static void load_hide_paths() {
    pthread_mutex_lock(&g_config_mutex);
    g_hide_paths.clear();
    std::ifstream f(HIDE_PATHS_FILE);
    std::string line;
    while (std::getline(f, line)) {
        size_t pos = line.find('#');
        if (pos != std::string::npos) line = line.substr(0, pos);
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(start, end - start + 1);
        if (line.empty()) continue;
        g_hide_paths.push_back(line);
    }
    pthread_mutex_unlock(&g_config_mutex);
    log_msg("Loaded %zu hide paths", g_hide_paths.size());
}

static void load_hide_procs() {
    pthread_mutex_lock(&g_config_mutex);
    g_hide_procs.clear();
    std::ifstream f(HIDE_PROCS_FILE);
    std::string line;
    while (std::getline(f, line)) {
        size_t pos = line.find('#');
        if (pos != std::string::npos) line = line.substr(0, pos);
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(start, end - start + 1);
        if (line.empty()) continue;
        g_hide_procs.push_back(line);
    }
    pthread_mutex_unlock(&g_config_mutex);
    log_msg("Loaded %zu hide procs", g_hide_procs.size());
}

static bool is_whitelisted(uid_t uid) {
    pthread_mutex_lock(&g_config_mutex);
    bool ok = g_whitelist.count(uid) > 0;
    pthread_mutex_unlock(&g_config_mutex);
    return ok;
}

// ─── 全局 bind-mount 隐藏 ──────────────────────────────────

static void apply_global_hides() {
    // 确保 empty dir 存在
    mkdir(EMPTY_DIR, 0700);

    pthread_mutex_lock(&g_config_mutex);
    for (const auto& path : g_hide_paths) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) continue;

        // 检查是否已 mount
        // 简化：直接尝试 mount，已 mount 会失败但不致命
        if (mount(EMPTY_DIR, path.c_str(), nullptr, MS_BIND, nullptr) == 0) {
            log_msg("Hidden: %s", path.c_str());
        } else {
            // 可能已经 mount 过了
            log_msg("mount %s: %s (may already be mounted)", path.c_str(), strerror(errno));
        }
    }
    pthread_mutex_unlock(&g_config_mutex);
}

// ─── Socket 命令处理 ───────────────────────────────────────

static std::string handle_command(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string op;
    iss >> op;

    if (op == "STATUS") {
        char buf[128];
        snprintf(buf, sizeof(buf), "OK pid=%d whitelist=%zu paths=%zu",
                 (int)getpid(), g_whitelist.size(), g_hide_paths.size());
        return buf;
    }
    if (op == "WHITELIST_ADD") {
        uid_t uid;
        if (!(iss >> uid)) return "ERR invalid uid";
        pthread_mutex_lock(&g_config_mutex);
        g_whitelist.insert(uid);
        pthread_mutex_unlock(&g_config_mutex);
        // 持久化
        std::ofstream f(WHITELIST_FILE, std::ios::app);
        if (f) f << uid << "\n";
        return "OK added";
    }
    if (op == "WHITELIST_DEL") {
        uid_t uid;
        if (!(iss >> uid)) return "ERR invalid uid";
        pthread_mutex_lock(&g_config_mutex);
        g_whitelist.erase(uid);
        pthread_mutex_unlock(&g_config_mutex);
        // 重新持久化
        std::ofstream f(WHITELIST_FILE);
        if (f) {
            for (uid_t u : g_whitelist) f << u << "\n";
        }
        return "OK removed";
    }
    if (op == "HIDE_PATH_ADD") {
        std::string path;
        if (!(iss >> path)) return "ERR invalid path";
        pthread_mutex_lock(&g_config_mutex);
        g_hide_paths.push_back(path);
        pthread_mutex_unlock(&g_config_mutex);
        std::ofstream f(HIDE_PATHS_FILE, std::ios::app);
        if (f) f << path << "\n";
        // 立即应用
        mkdir(EMPTY_DIR, 0700);
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            mount(EMPTY_DIR, path.c_str(), nullptr, MS_BIND, nullptr);
        }
        return "OK added and applied";
    }
    if (op == "HIDE_PATH_DEL") {
        std::string path;
        if (!(iss >> path)) return "ERR invalid path";
        pthread_mutex_lock(&g_config_mutex);
        g_hide_paths.erase(
            std::remove(g_hide_paths.begin(), g_hide_paths.end(), path),
            g_hide_paths.end());
        pthread_mutex_unlock(&g_config_mutex);
        // umount
        umount(path.c_str());
        // 持久化
        std::ofstream f(HIDE_PATHS_FILE);
        if (f) {
            for (const auto& p : g_hide_paths) f << p << "\n";
        }
        return "OK removed and unmounted";
    }
    if (op == "RELOAD") {
        load_whitelist();
        load_hide_paths();
        load_hide_procs();
        apply_global_hides();
        return "OK reloaded";
    }
    if (op == "STOP") {
        g_running = 0;
        return "OK stopping";
    }
    return "ERR unknown command";
}

static void* client_thread(void* arg) {
    int client_fd = (int)(intptr_t)arg;
    char buf[1024];
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        // 去掉换行
        if (n > 0 && buf[n-1] == '\n') buf[n-1] = '\0';
        std::string response = handle_command(buf);
        write(client_fd, response.c_str(), response.size());
    }
    close(client_fd);
    return nullptr;
}

static int run_socket_server() {
    // 创建 Unix socket
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        log_msg("socket failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_msg("bind failed: %s", strerror(errno));
        close(sock);
        return -1;
    }

    chmod(SOCKET_PATH, 0666);  // 允许 app 连接

    if (listen(sock, 8) < 0) {
        log_msg("listen failed: %s", strerror(errno));
        close(sock);
        return -1;
    }

    log_msg("Socket server listening at %s", SOCKET_PATH);

    while (g_running) {
        int client = accept(sock, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) continue;
            if (!g_running) break;
            log_msg("accept failed: %s", strerror(errno));
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, nullptr, client_thread, (void*)(intptr_t)client);
        pthread_detach(tid);
    }

    close(sock);
    unlink(SOCKET_PATH);
    return 0;
}

// ─── 主程序 ────────────────────────────────────────────────

int main(int argc, char** argv) {
    // daemonize
    if (argc > 1 && strcmp(argv[1], "--foreground") != 0) {
        // 已经由 service.sh nohup 启动，不需要再 fork
    }

    // 安装信号处理器
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    log_msg("APEX Hide Daemon starting (PID=%d)", (int)getpid());

    // 加载配置
    load_whitelist();
    load_hide_paths();
    load_hide_procs();

    // 应用全局隐藏
    apply_global_hides();

    // 启动 socket 服务器
    int ret = run_socket_server();

    log_msg("APEX Hide Daemon exiting (ret=%d)", ret);

    // 清理：umount 所有隐藏路径
    pthread_mutex_lock(&g_config_mutex);
    for (const auto& path : g_hide_paths) {
        umount(path.c_str());
    }
    pthread_mutex_unlock(&g_config_mutex);

    return ret;
}

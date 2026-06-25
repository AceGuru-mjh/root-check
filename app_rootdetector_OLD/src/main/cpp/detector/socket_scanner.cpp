#include "socket_scanner.h"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>
#include <algorithm>

static const std::vector<std::string> root_sockets = {
    "magiskd", "ksud", "lspd", "apatchd", "zygiskd",
    "shamikod", "rirusocket", "rirud", "dbus-daemon"
};

bool SocketScanner::isRootDaemonSocket(const std::string &name) {
    for (const auto &rs : root_sockets) {
        if (name.find(rs) != std::string::npos) return true;
    }
    return false;
}

bool SocketScanner::isSuspiciousSocket(const SocketInfo &sock) {
    if (isRootDaemonSocket(sock.peer_name)) return true;
    if (sock.type == 1 && sock.peer_name.find("_agent") != std::string::npos) return true;
    if (sock.type == 1 && sock.peer_name.find("@") == 0 &&
        sock.peer_name.find("magisk") != std::string::npos) return true;
    return false;
}

std::vector<SocketInfo> SocketScanner::scanUnixSockets() {
    std::vector<SocketInfo> sockets;
    std::ifstream file("/proc/net/unix");
    if (!file.is_open()) return sockets;

    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        SocketInfo sock;
        sock.type = 1;
        sock.protocol = "UNIX";
        std::string num, refcnt, flags, type_dummy, st, inode_dummy;
        iss >> num >> refcnt >> flags >> type_dummy >> st >> inode_dummy;
        size_t path_pos = line.find_last_of(' ');
        if (path_pos != std::string::npos) {
            sock.peer_name = line.substr(path_pos + 1);
        }
        sock.is_suspicious = isSuspiciousSocket(sock);
        sockets.push_back(sock);
    }
    return sockets;
}

std::vector<SocketInfo> SocketScanner::scanTcpSockets() {
    std::vector<SocketInfo> sockets;
    std::ifstream file("/proc/net/tcp");
    if (!file.is_open()) return sockets;

    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        SocketInfo sock;
        sock.type = 2;
        sock.protocol = "TCP";
        std::string num, local, remote, st;
        iss >> num >> local >> remote >> st;
        sock.local_address = local;
        sock.remote_address = remote;
        int state = std::stoi(st, nullptr, 16);
        switch (state) {
            case 1: sock.state = "ESTABLISHED"; break;
            case 2: sock.state = "SYN_SENT"; break;
            case 10: sock.state = "LISTEN"; break;
            default: sock.state = "UNKNOWN"; break;
        }
        sock.is_suspicious = false;
        sockets.push_back(sock);
    }
    return sockets;
}

std::vector<SocketInfo> SocketScanner::scanProcessFd(pid_t pid) {
    std::vector<SocketInfo> results;
    std::string fd_path = "/proc/" + std::to_string(pid) + "/fd";
    DIR *dir = opendir(fd_path.c_str());
    if (!dir) return results;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;
        int fd = std::atoi(entry->d_name);
        std::string link_path = fd_path + "/" + entry->d_name;
        char link_buf[256];
        ssize_t len = readlink(link_path.c_str(), link_buf, sizeof(link_buf) - 1);
        if (len > 0) {
            link_buf[len] = '\0';
            std::string target(link_buf);
            SocketInfo sock;
            sock.fd = fd;
            sock.peer_name = target;
            if (target.find("socket:") != std::string::npos) {
                sock.type = 1;
                sock.protocol = "UNIX";
                sock.is_suspicious = isRootDaemonSocket(target);
                results.push_back(sock);
            }
        }
    }
    closedir(dir);
    return results;
}

SocketScanResult SocketScanner::detect() {
    SocketScanResult result;
    result.detected = false;
    result.layer = "SocketScanner";
    result.total_sockets = 0;
    result.suspicious_sockets = 0;

    result.unix_sockets = scanUnixSockets();
    result.tcp_sockets = scanTcpSockets();
    auto proc_socks = scanProcessFd(getpid());

    result.total_sockets = (int)(result.unix_sockets.size() +
                                 result.tcp_sockets.size() +
                                 proc_socks.size());

    for (const auto &s : result.unix_sockets) {
        if (s.is_suspicious) {
            result.suspicious_sockets++;
            result.detected = true;
            result.findings.push_back("UNIX socket: " + s.peer_name);
        }
    }

    for (const auto &s : proc_socks) {
        if (s.is_suspicious) {
            result.suspicious_sockets++;
            result.detected = true;
            result.findings.push_back("FD " + std::to_string(s.fd) + ": " + s.peer_name);
        }
    }

    if (result.detected) {
        result.detail = "Found " + std::to_string(result.suspicious_sockets) +
                        " suspicious sockets";
    } else {
        result.detail = "No suspicious sockets found";
    }

    return result;
}

#ifndef ROOTDETECTOR_SOCKET_SCANNER_H
#define ROOTDETECTOR_SOCKET_SCANNER_H

#include "property_detector.h"
#include <string>
#include <vector>

struct SocketInfo {
    int fd;
    int type;
    std::string protocol;
    std::string local_address;
    std::string remote_address;
    std::string state;
    std::string peer_name;
    int inode;
    bool is_suspicious;
    std::string suspicious_reason;
};

struct SocketScanResult {
    bool detected;
    std::string layer;
    std::string detail;
    std::vector<SocketInfo> unix_sockets;
    std::vector<SocketInfo> tcp_sockets;
    int total_sockets;
    int suspicious_sockets;
    std::vector<std::string> findings;
};

class SocketScanner {
public:
    static SocketScanResult detect();

private:
    static std::vector<SocketInfo> scanUnixSockets();
    static std::vector<SocketInfo> scanTcpSockets();
    static std::vector<SocketInfo> scanProcessFd(pid_t pid);
    static bool isSuspiciousSocket(const SocketInfo &sock);
    static bool isRootDaemonSocket(const std::string &name);
};

#endif

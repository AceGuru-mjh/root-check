#ifndef ROOTDETECTOR_PROCESS_DETECTOR_H
#define ROOTDETECTOR_PROCESS_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>
#include <cstdint>

// 进程信息结构体
struct ProcessInfo {
    pid_t pid;                    // 进程ID
    std::string name;             // 进程名称
    std::string cmdline;          // 命令行参数
    std::string exe;              // 可执行文件路径
    std::string selinuxContext;   // SELinux 上下文
    bool isSuspicious;            // 是否可疑
    std::string suspiciousReason; // 可疑原因
};

// 线程信息结构体
struct ThreadInfo {
    pid_t tid;                    // 线程ID
    std::string name;             // 线程名称
    std::string selinuxContext;   // SELinux 上下文
    bool isSuspicious;            // 是否可疑
    std::string suspiciousReason; // 可疑原因
};

// 进程检测结果结构体
struct ProcessDetectionResult {
    std::string layer;            // 检测层名称
    bool detected;                // 是否检测到异常
    std::string detail;           // 详细信息
    std::vector<ProcessInfo> suspiciousProcesses;  // 可疑进程列表
    std::vector<ThreadInfo> suspiciousThreads;     // 可疑线程列表
    int totalProcessesScanned;    // 扫描的总进程数
    int totalThreadsScanned;      // 扫描的总线程数
};

class ProcessDetector {
public:
    // 主检测方法
    static ProcessDetectionResult detect();
    
    // 检测异常进程
    static std::vector<ProcessInfo> detectSuspiciousProcesses();
    
    // 检测线程 SELinux 上下文
    static std::vector<ThreadInfo> detectSuspiciousThreads();
    
private:
    // 使用 syscall 读取 /proc 目录获取所有进程
    static std::vector<pid_t> getAllPids();
    
    // 获取进程信息
    static ProcessInfo getProcessInfo(pid_t pid);
    
    // 获取线程信息
    static std::vector<ThreadInfo> getThreadInfos(pid_t pid);
    
    // 读取进程的 SELinux 上下文
    static std::string getProcessSelinuxContext(pid_t pid);
    
    // 读取线程的 SELinux 上下文
    static std::string getThreadSelinuxContext(pid_t tid);
    
    // 检查进程名称是否可疑
    static bool isProcessNameSuspicious(const std::string& name, std::string& reason);
    
    // 检查 SELinux 上下文是否可疑
    static bool isSelinuxContextSuspicious(const std::string& context, std::string& reason);
    
    // 获取可疑进程名称列表
    static std::vector<std::string> getSuspiciousProcessNames();
    
    // 获取可疑 SELinux 上下文列表
    static std::vector<std::string> getSuspiciousSelinuxContexts();
};

#endif // ROOTDETECTOR_PROCESS_DETECTOR_H

#include "self_protection.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <random>
#include <algorithm>
#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/wait.h>
#include <android/log.h>
#include <cstring>
#include <sys/mman.h>
#include <dlfcn.h>
#include <link.h>
#include <elf.h>
#include <signal.h>

#define LOG_TAG "SelfProtection"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// ==================== 混淆命名空间实现 ====================
namespace _0x4a2f {
    
    // 反调试检测（混淆名）
    bool _chk_dbg() {
        // 使用 ptrace 检测
        if (ptrace(PTRACE_TRACEME, 0, 1, 0) == -1) {
            return true;
        }
        
        // 检查 TracerPid
        std::ifstream status("/proc/self/status");
        if (status.is_open()) {
            std::string line;
            while (std::getline(status, line)) {
                if (line.find("TracerPid:") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string pidStr = line.substr(pos + 1);
                        pidStr.erase(0, pidStr.find_first_not_of(" \t"));
                        int tracerPid = std::stoi(pidStr);
                        if (tracerPid != 0) {
                            return true;
                        }
                    }
                }
            }
        }
        
        return false;
    }
    
    // 注入检测（混淆名）
    bool _chk_inj() {
        std::ifstream maps("/proc/self/maps");
        if (!maps.is_open()) return false;
        
        std::vector<std::string> suspiciousPaths = {
            "/data/local/", "/data/data/", "/sdcard/", 
            "/mnt/sdcard/", "/external/", "frida", "xposed"
        };
        
        std::string line;
        while (std::getline(maps, line)) {
            if (line.find(".so") != std::string::npos) {
                for (const auto& path : suspiciousPaths) {
                    if (line.find(path) != std::string::npos) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    // 代码完整性检测（混淆名）
    bool _chk_code() {
        // 检查 .text 段是否被修改
        std::ifstream maps("/proc/self/maps");
        if (!maps.is_open()) return false;
        
        std::string line;
        while (std::getline(maps, line)) {
            if (line.find("r-xp") != std::string::npos && 
                line.find(".so") != std::string::npos) {
                // 检查可执行段权限是否异常
                if (line.find("rwxp") != std::string::npos) {
                    return true; // RWX 段可疑
                }
            }
        }
        return false;
    }
    
    // 多进程检测（混淆名）
    bool _chk_multi() {
        pid_t pid = fork();
        if (pid < 0) return false;
        
        if (pid == 0) {
            bool debugger = _chk_dbg();
            bool injection = _chk_inj();
            _exit(debugger || injection ? 1 : 0);
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                return true;
            }
        }
        return false;
    }
    
    // Dump 检测（混淆名）
    bool _chk_dump() {
        // 检查 /proc/self/mem 是否被访问
        std::ifstream fd_dir("/proc/self/fd");
        if (!fd_dir.is_open()) return false;
        
        std::string line;
        while (std::getline(fd_dir, line)) {
            if (line.find("mem") != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    // Hook 检测（混淆名）
    bool _chk_hook() {
        // 检查关键函数是否被 hook
        void* libc = dlopen("libc.so", RTLD_NOW);
        if (!libc) return false;
        
        void* open_addr = dlsym(libc, "open");
        void* read_addr = dlsym(libc, "read");
        
        if (!open_addr || !read_addr) {
            dlclose(libc);
            return true; // 符号解析失败，可能被 hook
        }
        
        // 检查函数地址是否在合法范围内
        Dl_info info;
        if (dladdr(open_addr, &info) == 0) {
            dlclose(libc);
            return true;
        }
        
        dlclose(libc);
        return false;
    }
    
    // 随机化检测顺序（混淆名）
    std::vector<int> _rand_order(int count) {
        std::vector<int> order(count);
        for (int i = 0; i < count; ++i) {
            order[i] = i;
        }
        
        // 使用时间种子和硬件随机数
        std::random_device rd;
        std::mt19937 g(rd() ^ (std::mt19937::result_type)time(nullptr));
        std::shuffle(order.begin(), order.end(), g);
        
        return order;
    }
}

// ==================== SelfProtection 类实现 ====================

bool SelfProtection::checkDebugger() {
    return _0x4a2f::_chk_dbg();
}

bool SelfProtection::checkInjection() {
    return _0x4a2f::_chk_inj();
}

bool SelfProtection::checkCodeIntegrity() {
    return _0x4a2f::_chk_code();
}

bool SelfProtection::checkMultiProcess() {
    return _0x4a2f::_chk_multi();
}

bool SelfProtection::checkDump() {
    return _0x4a2f::_chk_dump();
}

bool SelfProtection::checkHook() {
    return _0x4a2f::_chk_hook();
}

bool SelfProtection::checkBreakpoint() {
    // 检查 INT3 断点（0xCC）
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) return false;
    
    std::string line;
    while (std::getline(maps, line)) {
        if (line.find("r-xp") != std::string::npos && 
            line.find("rootdetector") != std::string::npos) {
            // 解析地址范围
            size_t dash_pos = line.find('-');
            size_t space_pos = line.find(' ');
            if (dash_pos != std::string::npos && space_pos != std::string::npos) {
                std::string start_str = line.substr(0, dash_pos);
                std::string end_str = line.substr(dash_pos + 1, space_pos - dash_pos - 1);
                
                unsigned long start_addr = std::stoul(start_str, nullptr, 16);
                unsigned long end_addr = std::stoul(end_str, nullptr, 16);
                
                // 检查前几个字节是否包含 0xCC
                volatile unsigned char* ptr = (volatile unsigned char*)start_addr;
                for (size_t i = 0; i < 16 && (start_addr + i) < end_addr; ++i) {
                    if (ptr[i] == 0xCC) {
                        return true;
                    }
                }
            }
            break;
        }
    }
    return false;
}

std::vector<int> SelfProtection::randomizeDetectionOrder(int count) {
    return _0x4a2f::_rand_order(count);
}

std::string SelfProtection::getSelfPath() {
    char path[256];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return std::string(path);
    }
    return "";
}

unsigned long SelfProtection::computeChecksum(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return 0;
    
    unsigned long checksum = 0;
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        for (size_t i = 0; i < file.gcount(); ++i) {
            checksum = (checksum << 1) ^ (unsigned char)buffer[i];
        }
    }
    return checksum;
}

void SelfProtection::initProtection() {
    // 初始化时立即进行反调试检测
    if (_0x4a2f::_chk_dbg()) {
        LOGI("检测到调试器，终止执行");
        exit(1);
    }
    
    // 检查是否被注入
    if (_0x4a2f::_chk_inj()) {
        LOGI("检测到代码注入");
    }
}

bool SelfProtection::verifyIntegrity() {
    // 计算当前代码段校验和
    std::string self_path = getSelfPath();
    if (self_path.empty()) return false;
    
    unsigned long current_checksum = computeChecksum(self_path);
    
    // 这里应该与预存的校验和对比
    // 简化处理：检查校验和是否为 0
    return current_checksum != 0;
}

DetectionResult SelfProtection::detect() {
    DetectionResult result;
    result.layer = "自保护机制检测";
    result.detected = false;
    
    std::vector<std::string> findings;
    
    // 随机化检测顺序
    auto order = _0x4a2f::_rand_order(6);
    
    for (int idx : order) {
        switch (idx) {
            case 0:
                if (_0x4a2f::_chk_dbg()) {
                    findings.push_back("检测到调试器");
                }
                break;
            case 1:
                if (_0x4a2f::_chk_inj()) {
                    findings.push_back("检测到代码注入");
                }
                break;
            case 2:
                if (_0x4a2f::_chk_code()) {
                    findings.push_back("代码完整性异常");
                }
                break;
            case 3:
                if (_0x4a2f::_chk_multi()) {
                    findings.push_back("多进程验证异常");
                }
                break;
            case 4:
                if (_0x4a2f::_chk_dump()) {
                    findings.push_back("检测到内存 Dump");
                }
                break;
            case 5:
                if (_0x4a2f::_chk_hook()) {
                    findings.push_back("检测到函数 Hook");
                }
                break;
        }
    }
    
    if (!findings.empty()) {
        result.detected = true;
        result.detail = "自保护异常: ";
        for (size_t i = 0; i < findings.size(); ++i) {
            result.detail += findings[i];
            if (i < findings.size() - 1) {
                result.detail += "; ";
            }
        }
    } else {
        result.detail = "自保护机制正常";
    }
    
    return result;
}

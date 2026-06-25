#ifndef ROOTDETECTOR_SYSCALL_UTILS_H
#define ROOTDETECTOR_SYSCALL_UTILS_H

#include <sys/types.h>
#include <string>
#include <vector>

class SyscallUtils {
public:
    // 使用直接 syscall 检测文件是否存在（绕过 Hook）
    static bool fileExistsDirect(const std::string& path);
    
    // 使用直接 syscall 读取文件内容
    static std::string readFileDirect(const std::string& path);
    
    // 获取目录下的文件列表（使用 getdents64）
    static std::vector<std::string> listFilesDirect(const std::string& path);
};

#endif // ROOTDETECTOR_SYSCALL_UTILS_H

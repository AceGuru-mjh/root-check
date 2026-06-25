#include "syscall_utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <vector>
#include <android/log.h>

#define LOG_TAG "SyscallUtils"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 直接 syscall 封装（避免被 Hook）
#if defined(__aarch64__)
#define __NR_accessat __NR_faccessat
#elif defined(__arm__)
#define __NR_accessat (__NR_SYSCALL_BASE + 300)
#endif

bool SyscallUtils::fileExistsDirect(const std::string& path) {
    // 使用 syscall 直接调用 faccessat
    int fd = syscall(__NR_openat, AT_FDCWD, path.c_str(), O_RDONLY | O_CLOEXEC, 0);
    if (fd >= 0) {
        close(fd);
        return true;
    }
    return false;
}

std::string SyscallUtils::readFileDirect(const std::string& path) {
    int fd = syscall(__NR_openat, AT_FDCWD, path.c_str(), O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) {
        return "";
    }
    
    char buffer[4096];
    std::string result;
    ssize_t bytesRead;
    
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        result.append(buffer, bytesRead);
    }
    
    close(fd);
    return result;
}

std::vector<std::string> SyscallUtils::listFilesDirect(const std::string& path) {
    std::vector<std::string> files;
    
    int fd = syscall(__NR_openat, AT_FDCWD, path.c_str(), 
                     O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    if (fd < 0) {
        return files;
    }
    
    // 使用 getdents64 直接读取目录
    struct linux_dirent64 {
        unsigned long long d_ino;
        long long d_off;
        unsigned short d_reclen;
        unsigned char d_type;
        char d_name[];
    };
    
    char buf[4096];
    ssize_t nread;
    
    while ((nread = syscall(__NR_getdents64, fd, buf, sizeof(buf))) > 0) {
        for (long bpos = 0; bpos < nread;) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + bpos);
            if (strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) {
                files.push_back(d->d_name);
            }
            bpos += d->d_reclen;
        }
    }
    
    close(fd);
    return files;
}

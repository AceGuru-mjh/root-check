#include "mount_detector.h"
#include <fstream>
#include <sstream>
#include <set>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <android/log.h>

#define LOG_TAG "MountDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

static std::set<std::string> getMountInfo() {
    std::set<std::string> mounts;
    std::ifstream mountinfo("/proc/self/mountinfo");

    if (!mountinfo.is_open()) {
        return mounts;
    }

    std::string line;
    while (std::getline(mountinfo, line)) {
        mounts.insert(line);
    }

    return mounts;
}

DetectionResult MountDetector::detectNamespaceManipulation() {
    DetectionResult result;
    result.layer = "第4层增强：挂载命名空间对比";
    result.detected = false;

    // 获取主进程的挂载信息
    auto mainMounts = getMountInfo();

    if (mainMounts.empty()) {
        result.detail = "无法读取主进程挂载信息";
        return result;
    }

    // 创建临时文件用于子进程传递挂载信息
    std::string tmpPath = "/data/local/tmp/.mount_check_" + std::to_string(getpid());

    pid_t pid = fork();

    if (pid < 0) {
        result.detail = "无法创建子进程进行命名空间对比";
        LOGW("fork() 失败");
        return result;
    }

    if (pid == 0) {
        // 子进程：获取挂载信息写入临时文件后退出
        auto childMounts = getMountInfo();
        FILE* fp = fopen(tmpPath.c_str(), "w");
        if (fp) {
            for (const auto& line : childMounts) {
                fprintf(fp, "%s\n", line.c_str());
            }
            fclose(fp);
        }
        _exit(0);
    }

    // 父进程：等待子进程完成
    int status;
    waitpid(pid, &status, 0);

    // 读取子进程的挂载信息
    std::set<std::string> childMounts;
    {
        std::ifstream childFile(tmpPath);
        if (childFile.is_open()) {
            std::string line;
            while (std::getline(childFile, line)) {
                childMounts.insert(line);
            }
        }
    }

    // 清理临时文件
    unlink(tmpPath.c_str());

    if (childMounts.empty()) {
        result.detail = "无法读取子进程挂载信息";
        return result;
    }

    // 对比差异
    std::vector<std::string> differences;

    // 查找主进程有但子进程没有的挂载项（可能被隐藏）
    for (const auto& mount : mainMounts) {
        if (childMounts.find(mount) == childMounts.end()) {
            differences.push_back("主进程独有: " + mount);
        }
    }

    // 查找子进程有但主进程没有的挂载项（可能被注入）
    for (const auto& mount : childMounts) {
        if (mainMounts.find(mount) == mainMounts.end()) {
            differences.push_back("子进程独有: " + mount);
        }
    }

    if (!differences.empty()) {
        result.detected = true;
        result.detail = "发现 " + std::to_string(differences.size()) + " 处命名空间差异，可能存在挂载隐藏";
        LOGI("检测到命名空间操纵，差异数: %zu", differences.size());
    } else {
        result.detail = "未发现命名空间操纵";
        LOGI("命名空间对比正常，挂载项数: %zu", mainMounts.size());
    }

    return result;
}

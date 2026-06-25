#ifndef ROOTDETECTOR_KERNEL_MODULE_DETECTOR_H
#define ROOTDETECTOR_KERNEL_MODULE_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>

// 内核模块信息
struct KernelModuleInfo {
    std::string name;       // 模块名称
    std::string size;       // 模块大小
    std::string refCount;   // 引用计数
    std::string state;      // 状态 (Live, Loading, Unloading)
    std::string address;    // 加载地址
    std::string source;     // 来源: "proc" 或 "sys"
};

// 内核符号信息
struct KernelSymbolInfo {
    std::string address;    // 符号地址
    std::string type;       // 符号类型 (T=全局代码, t=局部代码, D=全局数据等)
    std::string name;       // 符号名称
    std::string reason;     // 被标记为可疑的原因
};

// 内核模块检测详细结果
struct KernelModuleDetectionDetail {
    bool hasUnofficialModules;
    bool hasSymbolAnomaly;
    bool hasMemoryAnomaly;
    bool hasKallsymsExported;
    bool hasSyscallTableHook;
    std::vector<KernelModuleInfo> allModules;
    std::vector<KernelModuleInfo> suspiciousModules;
    std::vector<KernelSymbolInfo> suspiciousSymbols;
    std::vector<std::string> findings;
};

class KernelModuleDetector {
public:
    // 标准检测接口（返回 DetectionResult）
    static DetectionResult detect();

    // 详细检测接口（返回完整检测结果）
    static KernelModuleDetectionDetail detectDetailed();

    // 扫描 /proc/modules 获取已加载模块列表
    static std::vector<KernelModuleInfo> scanProcModules();

    // 扫描 /sys/module 获取模块详细信息
    static std::vector<std::string> scanSysModules();

    // 检测非官方内核模块
    static std::vector<KernelModuleInfo> detectUnofficialModules(
        const std::vector<KernelModuleInfo>& modules);

    // 检测内核符号表异常
    static bool detectSymbolTableAnomaly(std::vector<KernelSymbolInfo>& suspiciousSymbols);

    // 检测内核内存中的异常代码段
    static bool detectMemoryAnomaly(std::vector<std::string>& findings);

private:
    // 获取官方内核模块白名单
    static std::vector<std::string> getOfficialModuleWhitelist();

    // 获取可疑模块关键词列表
    static std::vector<std::string> getSuspiciousKeywords();

    // 获取可疑内核符号列表
    static std::vector<std::string> getSuspiciousSymbolPatterns();

    // 检查模块名是否匹配白名单
    static bool isModuleWhitelisted(const std::string& moduleName);

    // 检查模块名是否包含可疑关键词
    static bool containsSuspiciousKeyword(const std::string& moduleName, std::string& matchedKeyword);

    // 检查 /proc/kallsyms 中 kallsyms_lookup_name 是否被导出
    static bool checkKallsymsLookupNameExported();

    // 检查系统调用表是否被修改
    static bool checkSyscallTableModified();

    // 扫描 /proc/kallsyms 查找异常符号
    static void scanKallsymsForSuspiciousSymbols(std::vector<KernelSymbolInfo>& suspiciousSymbols);
};

#endif // ROOTDETECTOR_KERNEL_MODULE_DETECTOR_H

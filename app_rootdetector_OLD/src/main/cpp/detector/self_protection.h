#ifndef ROOTDETECTOR_SELF_PROTECTION_H
#define ROOTDETECTOR_SELF_PROTECTION_H

#include "property_detector.h"
#include <string>

// 混淆后的函数名（手动重命名）
namespace _0x4a2f {
    bool _chk_dbg();
    bool _chk_inj();
    bool _chk_code();
    bool _chk_multi();
    bool _chk_dump();
    bool _chk_hook();
    std::vector<int> _rand_order(int count);
}

class SelfProtection {
public:
    static DetectionResult detect();
    
    // 初始化保护机制
    static void initProtection();
    
    // 运行时完整性校验
    static bool verifyIntegrity();

private:
    // 原始检测方法
    static bool checkDebugger();
    static bool checkInjection();
    static bool checkCodeIntegrity();
    static bool checkMultiProcess();
    static std::vector<int> randomizeDetectionOrder(int count);
    
    // 新增保护方法
    static bool checkDump();
    static bool checkHook();
    static bool checkBreakpoint();
    
    // 辅助方法
    static std::string getSelfPath();
    static unsigned long computeChecksum(const std::string& path);
};

#endif // ROOTDETECTOR_SELF_PROTECTION_H

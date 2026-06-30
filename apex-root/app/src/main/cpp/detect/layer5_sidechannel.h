#ifndef APEX_ROOT_LAYER5_SIDECHANNEL_H
#define APEX_ROOT_LAYER5_SIDECHANNEL_H

// 保留：用户态时延分析
bool detectSyscallTimingAnomaly();
bool detectCacheTimingAnomaly();

// 新增：syscall 结果一致性检测（替代原 syscall 表 hook 检测）
bool detectSyscallResultInconsistency();

// 已移除：detectBinderLatencyAnomaly() — 误报率高，与 Ring3 root 检测定位不符

#endif // APEX_ROOT_LAYER5_SIDECHANNEL_H

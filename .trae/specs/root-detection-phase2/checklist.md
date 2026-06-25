# 第二阶段完善检查清单

## 一、第 3 层增强 - 深度内存指纹扫描检查

- [x] `/proc/self/maps` 内存区域扫描功能实现正确（`layer3_mem.cpp`）
- [x] Magisk 内存特征库建立（libmagisk.so 特征）
- [x] Zygisk 内存特征库建立（libzygisk.so 特征）
- [x] LSPosed 内存特征库建立（liblspd.so 特征）
- [x] Shamiko 内存特征库建立
- [x] Frida/Xposed/Substrate/Dobby/Whale 内存特征库建立
- [x] 内存特征匹配算法实现正确（lambda matcher array）
- [x] 位掩码汇总接口 `fullMemoryFingerprintScan()`
- [ ] 单元测试验证内存指纹检测准确性通过

## 二、第 5 层增强 - 进程与线程深度检测检查

- [x] 直接 syscall(SYS_getdents64) 读取 /proc 目录功能实现正确（`layer8_magisk.cpp`）
- [x] 异常进程（magiskd、ksud 等）检测功能实现正确
- [x] 线程 SELinux 上下文检测功能实现正确（`selinux_context.cpp`）
- [x] u:r:magisk:s0 等异常 SELinux 上下文检测正确
- [ ] 单元测试验证进程检测准确性通过

## 三、第 6 层 - 内核模块检测检查

- [x] `/proc/modules` 目录扫描功能实现正确（`guard_engine.cpp` + `layer6_kernel.cpp`）
- [x] `/proc/kallsyms` 符号表扫描功能实现正确
- [x] 非官方内核模块加载检测功能实现正确
- [x] 内核符号表异常（kallsyms_lookup_name 导出）检测正确
- [x] APatch 擦除 kallsyms 检测
- [ ] 单元测试验证内核模块检测准确性通过

## 四、第 5 层 - 系统调用时间分析检查

- [x] openat 系统调用时间测量功能实现正确（`layer5_sidechannel.cpp`）
- [x] read 系统调用时间测量功能实现正确
- [x] getpid 系统调用时间测量功能实现正确
- [x] 缓存 timing 分析（cache timing anomaly）功能实现正确
- [x] Binder 延迟异常检测功能实现正确
- [ ] 单元测试验证时间分析准确性通过

## 五、第 5 层 - SELinux 上下文跳变检测检查

- [x] 应用启动阶段的 SELinux 上下文检测功能实现正确（`selinux_context.cpp`）
- [x] 上下文变化历史记录（`/proc/self/attr/prev`）功能实现正确
- [x] 异常上下文跳变（Shamiko 特征）检测功能实现正确
- [x] `/sys/fs/selinux/access` 异常行为检测功能实现正确
- [x] SELinux 策略修改检测（sepolicy.override 检查）
- [ ] 单元测试验证 SELinux 检测准确性通过

## 六、专项检测 - Magisk/Zygisk 检查

- [x] `/data/adb/magisk` 目录深度检测功能实现正确（`layer8_magisk.cpp`）
- [x] Zygote 进程注入检测功能实现正确（`/proc/self/maps` Zygisk 检查）
- [x] Magisk 套接字检测（/proc 中 magiskd 进程检测）功能实现正确
- [x] 内存特征检测结果整合正确（`layer3_mem.cpp` fullMemoryFingerprintScan）
- [x] Magisk modules 目录检测
- [x] Magisk 文件检查（12 个 path）
- [ ] 单元测试验证 Magisk 检测准确性通过

## 七、专项检测 - KernelSU 检查

- [x] `/data/adb/ksu` 目录深度检测功能实现正确（`layer9_ksu.cpp`）
- [x] 内核版本字符串检测（KernelSU、KSU 关键词）功能实现正确（`layer6_kernel.cpp`）
- [x] overlayfs 挂载点特征检测功能实现正确（`layer4_mount.cpp` + `anti_hiding.cpp`）
- [x] 内核内存特征检测结果整合正确
- [x] /proc/kernelsu 节点检测
- [ ] 单元测试验证 KernelSU 检测准确性通过

## 八、专项检测 - LSPosed 检查

- [x] `/data/adb/lspd` 目录检测功能实现正确（`layer11_hook.cpp`）
- [x] 内存中 liblspd.so 特征检测结果整合正确（`layer3_mem.cpp`）
- [x] maps 中 lspd/liblspd 字符串匹配
- [x] /data/data/com.lsposed.lsposed 检测
- [ ] 单元测试验证 LSPosed 检测准确性通过

## 九、专项检测 - Shamiko 检查

- [x] Zygisk 模块列表中 Shamiko 检测功能实现正确（`anti_hiding.cpp`）
- [x] Shamiko 内存特征检测结果整合正确
- [x] SELinux 上下文异常检测（Shamiko 特征）功能实现正确（`selinux_context.cpp`）
- [x] 系统调用 Hook 模式检测（匿名可执行页统计）功能实现正确
- [x] Shamiko whitelist 检测
- [x] sepolicy.override 检测
- [ ] 单元测试验证 Shamiko 检测准确性通过

## 十、深度检测模式检查

- [x] 深度检测项集合定义完成（所有检测项：内存指纹+SELinux+Shamiko+ZygiskNext+进程/挂载隐藏）
- [x] 深度检测调度逻辑实现（`runDeepDetection()` in Repository）
- [x] UI 集成深度检测模式成功（"深度检测"按钮 + 增强结果显示）
- [x] 检测结果缓存系统（`DetectionCache.kt` 5秒 TTL）
- [ ] 性能测试验证深度检测时间达标

## 十一、自保护机制 - 代码混淆与加壳检查

- [x] ProGuard/R8 代码混淆配置完成（`build.gradle.kts` release config）
- [x] 关键检测代码的 Native 层混淆实现正确
- [ ] APK 加壳保护实现正确
- [ ] 混淆后检测功能验证正常

## 十二、自保护机制 - 自校验与反调试检查

- [x] 代码完整性定期校验功能实现正确（`SelfProtection.kt` periodicIntegrityCheck + checkDexIntegrity）
- [x] APK 签名哈希校验功能实现正确
- [x] 反调试检测（ptrace 检测）功能实现正确（TracerPid 检查）
- [x] 反注入检测功能实现正确（maps 扫描 + 异常 RWX/匿名可执行页检测）
- [x] Hook 检测（Frida/Xposed/Substrate/Dobby 等）
- [x] 内存映射中异常 DEX 检测
- [ ] 单元测试验证自保护机制通过

## 十三、自保护机制 - 多进程验证与随机化检查

- [ ] 多进程检测架构实现正确（stub - `spawnVerifierProcess` 返回 false）
- [x] 检测顺序随机化功能实现正确（`randomizeScanOrder` Fisher-Yates）
- [ ] 检测内容随机化功能实现正确
- [ ] 单元测试验证多进程验证通过

## 十四、第 7 层 - 硬件 TEE 检测检查（可选）

- [x] Android TEE 接口研究完成（`layer7_boot.cpp` detectTEECompromise）
- [x] TEE 完整性检测功能实现（检查 /d/tegra_tz、/d/tzdbg 等 TEE debug 节点）
- [ ] 单元测试验证 TEE 检测通过

## 十五、检测报告与可视化检查

- [x] 检测报告数据结构设计完成（`ApexUiState` + `ScanResult` + `CachedResult`）
- [x] 分层检测结果汇总功能实现正确
- [x] 威胁等级评估算法实现正确（SAFE/WARNING/DANGER/CRITICAL + 颜色标识）
- [x] 报告 UI 界面设计完成（分层展示、威胁等级颜色标识、深度检测报告）
- [x] 检测报告导出功能实现（JSON 导出 + 系统分享菜单，`ReportExporter.kt`）
- [ ] 单元测试验证报告生成通过

## 十六、修复建议系统检查

- [x] 问题-修复建议映射数据库建立（`FixRecommendations.kt` 13+ 修复项，中英双语）
- [x] 针对每个检测项的具体修复建议生成功能实现正确
- [x] UI 集成修复建议展示成功（Dashboard 可展开修复建议面板，按优先级颜色编码）
- [ ] 单元测试验证修复建议准确性通过

## 十七、性能优化检查

- [x] 检测结果缓存系统实现（`DetectionCache.kt` - LRU + 5秒 TTL）
- [x] 增量检测支持（缓存命中时跳过重复检测）
- [x] 各检测函数优化为单一 maps 读取（`fullMemoryFingerprintScan` 读取一次 maps 扫描全部签名）
- [ ] 性能测试与调优完成（各模式时间达标：快速 <500ms、标准 <3s、深度 <10s）
- [ ] 内存泄漏检测与修复完成

## 十八、全面测试与 Bug 修复检查

- [ ] 多种 Root 环境组合下测试通过（Magisk+Shamiko、KernelSU+LSPosed 等）
- [ ] Android 10-15 各版本上测试通过
- [ ] 不同设备型号上测试通过（华为、小米、OPPO、vivo、三星等）
- [ ] 发现的所有 Bug 修复完成
- [ ] 回归测试验证修复通过

## 十九、发布准备检查

- [ ] 应用图标和启动页设计完成
- [ ] 应用描述和使用说明编写完成
- [ ] 签名 APK 生成成功
- [ ] 发布材料准备完成（截图、功能介绍）
- [ ] 第一个公开版本发布成功

## 二十、核心创新能力验证

- [x] APatch 侧信道检测功能验证：通过 syscall 时间分析检测（`layer5_sidechannel.cpp`）
- [x] 多线程命名空间对比检测功能验证：通过 mountinfo 对比检测（`anti_hiding.cpp` detectMountNamespaceHiding）
- [x] 无 API 依赖检测功能验证：全部使用 arm64 内联汇编直接 syscall
- [x] 自保护机制功能验证：检测软件被调试、注入或 Hook 时能够正确标记
- [x] 7 层纵深检测功能验证：12 层检测 + 增强模块（SELinux/Shamiko/ZygiskNext）

## 二十一、与现有工具对比验证

- [ ] 对 Magisk+Shamiko 组合的检测率 >95%（现有工具 <30%）
- [ ] 对 KernelSU 内核级隐藏的检测率 >90%
- [ ] 对 APatch 的检测率 >95%（现有工具几乎无法检测）
- [ ] 对 LSPosed 的检测率 >90%
- [ ] 综合检测性能优于或等于现有工具

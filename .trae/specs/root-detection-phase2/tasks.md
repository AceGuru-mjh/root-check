# 第二阶段完善任务列表

## 阶段一：深度检测功能实现（第 1-2 周）

- [ ] Task 1: 第 3 层增强 - 深度内存指纹扫描
  - [ ] SubTask 1.1: 实现 `/proc/self/mem` 可执行内存区域扫描
  - [ ] SubTask 1.2: 建立 Magisk 内存特征库（libmagisk.so 特征）
  - [ ] SubTask 1.3: 建立 Zygisk 内存特征库（libzygisk.so 特征）
  - [ ] SubTask 1.4: 建立 LSPosed 内存特征库（liblspd.so 特征）
  - [ ] SubTask 1.5: 建立 Shamiko 内存特征库
  - [ ] SubTask 1.6: 实现内存特征匹配算法
  - [ ] SubTask 1.7: 编写单元测试验证内存指纹检测准确性

- [ ] Task 2: 第 5 层增强 - 进程与线程深度检测
  - [ ] SubTask 2.1: 实现直接 syscall(SYS_getdents64) 读取 /proc 目录
  - [ ] SubTask 2.2: 检测异常进程（magiskd、ksud 等）
  - [ ] SubTask 2.3: 实现线程 SELinux 上下文检测
  - [ ] SubTask 2.4: 检测 u:r:magisk:s0 等异常 SELinux 上下文
  - [ ] SubTask 2.5: 编写单元测试验证进程检测准确性

- [ ] Task 3: 第 6 层 - 内核模块检测
  - [ ] SubTask 3.1: 实现扫描 `/proc/modules` 目录
  - [ ] SubTask 3.2: 实现扫描 `/sys/modules` 目录
  - [ ] SubTask 3.3: 检测非官方内核模块加载
  - [ ] SubTask 3.4: 检测内核符号表异常（kallsyms_lookup_name 导出）
  - [ ] SubTask 3.5: 检测内核内存中的异常代码段
  - [ ] SubTask 3.6: 编写单元测试验证内核模块检测准确性

- [ ] Task 4: 第 5 层 - 系统调用时间分析
  - [ ] SubTask 4.1: 实现 openat 系统调用时间测量
  - [ ] SubTask 4.2: 实现 readlinkat 系统调用时间测量
  - [ ] SubTask 4.3: 实现 getdents64 系统调用时间测量
  - [ ] SubTask 4.4: 建立基准设备执行时间数据库
  - [ ] SubTask 4.5: 实现时间异常检测与报告
  - [ ] SubTask 4.6: 编写单元测试验证时间分析准确性

- [ ] Task 5: 第 5 层 - SELinux 上下文跳变检测
  - [ ] SubTask 5.1: 实现应用启动不同阶段的 SELinux 上下文检测
  - [ ] SubTask 5.2: 实现上下文变化历史记录
  - [ ] SubTask 5.3: 检测异常上下文跳变（Shamiko 特征）
  - [ ] SubTask 5.4: 检测 `/sys/fs/selinux/access` 异常行为
  - [ ] SubTask 5.5: 编写单元测试验证 SELinux 检测准确性

## 阶段二：专项 Root 工具检测（第 3 周）

- [ ] Task 6: 专项检测 - Magisk/Zygisk
  - [ ] SubTask 6.1: 实现 `/data/adb/magisk` 目录深度检测
  - [ ] SubTask 6.2: 实现 Zygote 进程注入检测
  - [ ] SubTask 6.3: 实现 Magisk 套接字检测（/dev/socket/magiskd）
  - [ ] SubTask 6.4: 整合内存特征检测结果
  - [ ] SubTask 6.5: 编写单元测试验证 Magisk 检测准确性

- [ ] Task 7: 专项检测 - KernelSU
  - [ ] SubTask 7.1: 实现 `/data/adb/ksu` 目录深度检测
  - [ ] SubTask 7.2: 实现内核版本字符串检测（KernelSU、KSU 关键词）
  - [ ] SubTask 7.3: 实现 overlayfs 挂载点特征检测
  - [ ] SubTask 7.4: 整合内核内存特征检测结果
  - [ ] SubTask 7.5: 编写单元测试验证 KernelSU 检测准确性

- [ ] Task 8: 专项检测 - LSPosed
  - [ ] SubTask 8.1: 实现 `/data/adb/lspd` 目录深度检测
  - [ ] SubTask 8.2: 整合内存中 liblspd.so 特征检测
  - [ ] SubTask 8.3: 实现 ART 虚拟机 Hook 痕迹检测
  - [ ] SubTask 8.4: 实现异常 Binder 调用检测
  - [ ] SubTask 8.5: 编写单元测试验证 LSPosed 检测准确性

- [ ] Task 9: 专项检测 - Shamiko
  - [ ] SubTask 9.1: 实现 Zygisk 模块列表中 Shamiko 检测
  - [ ] SubTask 9.2: 整合 Shamiko 内存特征检测
  - [ ] SubTask 9.3: 实现 SELinux 上下文异常检测（Shamiko 特征）
  - [ ] SubTask 9.4: 实现系统调用 Hook 模式检测（Shamiko 独特模式）
  - [ ] SubTask 9.5: 编写单元测试验证 Shamiko 检测准确性

- [ ] Task 10: 深度检测模式实现
  - [ ] SubTask 10.1: 定义深度检测项集合（所有检测项）
  - [ ] SubTask 10.2: 实现深度检测调度逻辑（10-15 秒完成）
  - [ ] SubTask 10.3: UI 集成深度检测模式
  - [ ] SubTask 10.4: 性能测试验证深度检测时间

## 阶段三：自保护机制（第 4 周）

- [ ] Task 11: 自保护机制 - 代码混淆与加壳
  - [ ] SubTask 11.1: 配置 ProGuard/R8 代码混淆
  - [ ] SubTask 11.2: 实现关键检测代码的 Native 层混淆
  - [ ] SubTask 11.3: 实现 APK 加壳保护
  - [ ] SubTask 11.4: 验证混淆后检测功能正常

- [ ] Task 12: 自保护机制 - 自校验与反调试
  - [ ] SubTask 12.1: 实现代码完整性定期校验
  - [ ] SubTask 12.2: 实现数据完整性校验
  - [ ] SubTask 12.3: 实现反调试检测（ptrace 检测）
  - [ ] SubTask 12.4: 实现反注入检测
  - [ ] SubTask 12.5: 编写单元测试验证自保护机制

- [ ] Task 13: 自保护机制 - 多进程验证与随机化
  - [ ] SubTask 13.1: 实现多进程检测架构
  - [ ] SubTask 13.2: 实现进程间相互验证机制
  - [ ] SubTask 13.3: 实现检测顺序随机化
  - [ ] SubTask 13.4: 实现检测内容随机化
  - [ ] SubTask 13.5: 编写单元测试验证多进程验证

## 阶段四：报告系统与优化（第 5 周）

- [ ] Task 14: 第 7 层 - 硬件 TEE 检测（可选）
  - [ ] SubTask 14.1: 研究 Android TEE 接口
  - [ ] SubTask 14.2: 实现 TEE 完整性检测（如果可行）
  - [ ] SubTask 14.3: 编写单元测试验证 TEE 检测

- [ ] Task 15: 检测报告与可视化
  - [ ] SubTask 15.1: 设计检测报告数据结构
  - [ ] SubTask 15.2: 实现分层检测结果汇总
  - [ ] SubTask 15.3: 实现威胁等级评估算法
  - [ ] SubTask 15.4: 设计报告 UI 界面（分层展示、威胁等级颜色标识）
  - [ ] SubTask 15.5: 实现检测报告导出功能（PDF/JSON）
  - [ ] SubTask 15.6: 编写单元测试验证报告生成

- [ ] Task 16: 修复建议系统
  - [ ] SubTask 16.1: 建立问题-修复建议映射数据库
  - [ ] SubTask 16.2: 实现针对每个检测项的具体修复建议生成
  - [ ] SubTask 16.3: UI 集成修复建议展示
  - [ ] SubTask 16.4: 编写单元测试验证修复建议准确性

- [ ] Task 17: 性能优化
  - [ ] SubTask 17.1: 优化检测算法时间复杂度
  - [ ] SubTask 17.2: 优化内存扫描性能
  - [ ] SubTask 17.3: 实现检测结果缓存与增量检测
  - [ ] SubTask 17.4: 性能测试与调优（确保各模式时间达标）
  - [ ] SubTask 17.5: 内存泄漏检测与修复

## 阶段五：测试与发布（第 6 周）

- [ ] Task 18: 全面测试与 Bug 修复
  - [ ] SubTask 18.1: 在多种 Root 环境组合下测试（Magisk+Shamiko、KernelSU+LSPosed 等）
  - [ ] SubTask 18.2: 在 Android 10-15 各版本上测试
  - [ ] SubTask 18.3: 在不同设备型号上测试（华为、小米、OPPO、vivo、三星等）
  - [ ] SubTask 18.4: 修复发现的所有 Bug
  - [ ] SubTask 18.5: 回归测试验证修复

- [ ] Task 19: 发布准备
  - [ ] SubTask 19.1: 编写应用图标和启动页
  - [ ] SubTask 19.2: 编写应用描述和使用说明
  - [ ] SubTask 19.3: 生成签名 APK
  - [ ] SubTask 19.4: 准备发布材料（截图、功能介绍）
  - [ ] SubTask 19.5: 发布第一个公开版本

# 任务依赖关系

- [Task 1] 依赖于 [root-env-detection Task 4]（基础内存扫描）
- [Task 2] 依赖于 [root-env-detection Task 3]（基础文件检测）
- [Task 3] 依赖于 [root-env-detection Task 9]（系统调用表检查）
- [Task 4] 依赖于 [root-env-detection Task 8]（APatch 侧信道检测）
- [Task 5] 依赖于 [Task 2]（进程深度检测）
- [Task 6] 依赖于 [Task 1, Task 2]（内存指纹、进程检测）
- [Task 7] 依赖于 [root-env-detection Task 7, Task 3]（命名空间对比、内核模块）
- [Task 8] 依赖于 [root-env-detection Task 10, Task 1]（ART 检测、内存指纹）
- [Task 9] 依赖于 [Task 5, Task 6]（SELinux 检测、Magisk 检测）
- [Task 10] 依赖于 [Task 1-9]（所有深度检测和专项检测）
- [Task 11] 依赖于 [Task 10]（深度检测模式）
- [Task 12] 依赖于 [Task 11]（代码混淆）
- [Task 13] 依赖于 [Task 12]（自校验）
- [Task 14] 依赖于 [Task 10]（可选，依赖深度检测）
- [Task 15] 依赖于 [Task 10]（检测报告）
- [Task 16] 依赖于 [Task 15]（修复建议）
- [Task 17] 依赖于 [Task 13, Task 15, Task 16]（性能优化）
- [Task 18] 依赖于 [Task 17]（全面测试）
- [Task 19] 依赖于 [Task 18]（发布准备）

# 并行任务

以下任务可以并行开发：
- [Task 1, Task 2, Task 3, Task 4] 可以并行（分别依赖不同的已完成任务）
- [Task 5] 可以在 Task 2 完成后开始
- [Task 6, Task 7, Task 8, Task 9] 可以并行（专项检测相互独立，但依赖前置任务）
- [Task 11, Task 14, Task 15] 可以并行（自保护、TEE 检测、报告系统）

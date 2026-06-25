# 任务列表

## 阶段一：基础功能实现（第 1-2 周）

- [x] Task 1: 项目初始化与基础架构搭建
  - [x] SubTask 1.1: 创建 Android 项目（Kotlin + C/C++ NDK）
  - [x] SubTask 1.2: 配置 JNI/NDK 开发环境
  - [x] SubTask 1.3: 建立分层检测架构的基础框架
  - [x] SubTask 1.4: 实现检测引擎接口和调度器

- [x] Task 2: 第 1 层 - 应用层 API 与系统属性检测
  - [x] SubTask 2.1: 实现直接读取 `/dev/__properties__` 文件的 C++ 模块
  - [x] SubTask 2.2: 解析系统属性二进制格式，提取键值对
  - [x] SubTask 2.3: 实现关键词匹配（magisk、zygisk、su、ksu、apatch 等）
  - [x] SubTask 2.4: 建立设备型号官方属性基线数据库
  - [x] SubTask 2.5: 实现属性篡改检测逻辑
  - [x] SubTask 2.6: 编写单元测试验证属性检测准确性

- [x] Task 3: 第 4 层 - 基础文件系统检测
  - [x] SubTask 3.1: 实现直接 syscall 调用封装（SYS_access、SYS_openat）
  - [x] SubTask 3.2: 实现超过 100 个 su 路径变种的检测列表
  - [x] SubTask 3.3: 检测 `/data/adb` 目录及其子目录（magisk、ksu、lspd）
  - [x] SubTask 3.4: 检测 `/proc/kallsyms` 权限和内容异常
  - [x] SubTask 3.5: 扫描 loop 设备检测 magisk.img 等镜像挂载
  - [x] SubTask 3.6: 检测 overlayfs 挂载点（KernelSU 特征）
  - [x] SubTask 3.7: 编写单元测试验证文件检测准确性

- [x] Task 4: 第 3 层 - 基础内存扫描
  - [x] SubTask 4.1: 实现读取 `/proc/self/maps` 解析内存映射
  - [x] SubTask 4.2: 识别匿名内存映射的可执行代码区域
  - [x] SubTask 4.3: 实现基础内存字符串搜索（magisk、zygisk、lsposed 关键词）
  - [x] SubTask 4.4: 检测 `/memfd:jit-cache (deleted)` 区域
  - [x] SubTask 4.5: 编写单元测试验证内存扫描准确性

- [x] Task 5: 基础 UI 界面设计
  - [x] SubTask 5.1: 设计主界面布局（检测按钮、结果展示区）
  - [x] SubTask 5.2: 实现检测进度指示器
  - [x] SubTask 5.3: 实现基础结果展示（通过/失败状态）
  - [x] SubTask 5.4: 支持 Android 10-14 界面适配

- [x] Task 6: 快速检测模式实现
  - [x] SubTask 6.1: 定义快速检测项集合（最关键的 5-10 项检测）
  - [x] SubTask 6.2: 实现快速检测调度逻辑（<1 秒完成）
  - [x] SubTask 6.3: UI 集成快速检测模式
  - [x] SubTask 6.4: 性能测试验证快速检测时间

## 阶段二：核心功能突破（第 3-4 周）

- [x] Task 7: 第 4 层增强 - 多线程挂载命名空间对比检测
  - [x] SubTask 7.1: 实现主进程读取 `/proc/self/mountinfo`
  - [x] SubTask 7.2: 实现新进程中读取 `/proc/self/mountinfo`
  - [x] SubTask 7.3: 实现挂载信息对比算法
  - [x] SubTask 7.4: 检测命名空间操纵并报告
  - [x] SubTask 7.5: 编写单元测试验证命名空间检测准确性
  - [x] SubTask 7.6: 性能优化确保检测时间 <500ms

- [x] Task 8: 第 5 层 - APatch 侧信道时间差检测
  - [x] SubTask 8.1: 实现 CPU 核心绑定（sched_setaffinity）
  - [x] SubTask 8.2: 实现高精度时间测量（clock_gettime CLOCK_MONOTONIC_RAW）
  - [x] SubTask 8.3: 实现预热循环（100 次）
  - [x] SubTask 8.4: 实现 T1 测量（10000 次 truncate 0xffff）
  - [x] SubTask 8.5: 实现 T2 测量（10000 次 truncate 0x10000）
  - [x] SubTask 8.6: 实现时间差计算与阈值判断（500 纳秒）
  - [x] SubTask 8.7: 编写单元测试验证侧信道检测准确性
  - [x] SubTask 8.8: 在不同设备上校准阈值参数

- [x] Task 9: 第 5 层 - 系统调用表完整性检查
  - [x] SubTask 9.1: 实现从 `/proc/kallsyms` 获取系统调用表地址
  - [x] SubTask 9.2: 实现系统调用函数哈希计算
  - [x] SubTask 9.3: 建立官方内核系统调用哈希基准库
  - [x] SubTask 9.4: 实现哈希对比与异常检测
  - [x] SubTask 9.5: 特别检测 truncate 系统调用（45 号）
  - [x] SubTask 9.6: 编写单元测试验证系统调用检测准确性

- [x] Task 10: 第 2 层 - ART 虚拟机异常检测
  - [x] SubTask 10.1: 实现 ClassLoader 链扫描，检测非系统类加载器
  - [x] SubTask 10.2: 实现 Dex 文件加载来源检测（检查是否来自 /data/adb）
  - [x] SubTask 10.3: 实现 libart.so 函数内存哈希与原始哈希对比
  - [x] SubTask 10.4: 检测 ART 虚拟机中的 Hook 痕迹
  - [x] SubTask 10.5: 编写单元测试验证 ART 检测准确性

- [x] Task 11: 标准检测模式实现
  - [x] SubTask 11.1: 定义标准检测项集合（大部分检测项）
  - [x] SubTask 11.2: 实现标准检测调度逻辑（3-5 秒完成）
  - [x] SubTask 11.3: UI 集成标准检测模式
  - [x] SubTask 11.4: 性能测试验证标准检测时间

- [x] Task 12: Android 15 兼容性适配
  - [x] SubTask 12.1: 测试所有检测功能在 Android 15 上的兼容性
  - [x] SubTask 12.2: 修复 Android 15 特有的 API 变更问题
  - [x] SubTask 12.3: 验证 Android 15 上的检测准确性

## 阶段三：超越现有工具（第 5-6 周）

- [x] Task 13: 第 3 层增强 - 深度内存指纹扫描
  - [x] SubTask 13.1: 实现 `/proc/self/mem` 可执行内存区域扫描
  - [x] SubTask 13.2: 建立 Magisk 内存特征库（libmagisk.so 特征）
  - [x] SubTask 13.3: 建立 Zygisk 内存特征库（libzygisk.so 特征）
  - [x] SubTask 13.4: 建立 LSPosed 内存特征库（liblspd.so 特征）
  - [x] SubTask 13.5: 建立 Shamiko 内存特征库
  - [x] SubTask 13.6: 实现内存特征匹配算法
  - [x] SubTask 13.7: 编写单元测试验证内存指纹检测准确性

- [x] Task 14: 第 5 层增强 - 进程与线程深度检测
  - [x] SubTask 14.1: 实现直接 syscall(SYS_getdents64) 读取 /proc 目录
  - [x] SubTask 14.2: 检测异常进程（magiskd、ksud 等）
  - [x] SubTask 14.3: 实现线程 SELinux 上下文检测
  - [x] SubTask 14.4: 检测 u:r:magisk:s0 等异常 SELinux 上下文
  - [x] SubTask 14.5: 编写单元测试验证进程检测准确性

- [x] Task 15: 第 6 层 - 内核模块检测
  - [x] SubTask 15.1: 实现扫描 `/proc/modules` 目录
  - [x] SubTask 15.2: 实现扫描 `/sys/modules` 目录
  - [x] SubTask 15.3: 检测非官方内核模块加载
  - [x] SubTask 15.4: 检测内核符号表异常（kallsyms_lookup_name 导出）
  - [x] SubTask 15.5: 检测内核内存中的异常代码段
  - [x] SubTask 15.6: 编写单元测试验证内核模块检测准确性

- [x] Task 16: 第 5 层 - 系统调用时间分析
  - [x] SubTask 16.1: 实现 openat 系统调用时间测量
  - [x] SubTask 16.2: 实现 readlinkat 系统调用时间测量
  - [x] SubTask 16.3: 实现 getdents64 系统调用时间测量
  - [x] SubTask 16.4: 建立基准设备执行时间数据库
  - [x] SubTask 16.5: 实现时间异常检测与报告
  - [x] SubTask 16.6: 编写单元测试验证时间分析准确性

- [x] Task 17: 第 5 层 - SELinux 上下文跳变检测
  - [x] SubTask 17.1: 实现应用启动不同阶段的 SELinux 上下文检测
  - [x] SubTask 17.2: 实现上下文变化历史记录
  - [x] SubTask 17.3: 检测异常上下文跳变（Shamiko 特征）
  - [x] SubTask 17.4: 检测 `/sys/fs/selinux/access` 异常行为
  - [x] SubTask 17.5: 编写单元测试验证 SELinux 检测准确性

- [x] Task 18: 专项检测 - Magisk/Zygisk
  - [x] SubTask 18.1: 实现 `/data/adb/magisk` 目录深度检测
  - [x] SubTask 18.2: 实现 Zygote 进程注入检测
  - [x] SubTask 18.3: 实现 Magisk 套接字检测（/dev/socket/magiskd）
  - [x] SubTask 18.4: 整合内存特征检测结果
  - [x] SubTask 18.5: 编写单元测试验证 Magisk 检测准确性

- [x] Task 19: 专项检测 - KernelSU
  - [x] SubTask 19.1: 实现 `/data/adb/ksu` 目录深度检测
  - [x] SubTask 19.2: 实现内核版本字符串检测（KernelSU、KSU 关键词）
  - [x] SubTask 19.3: 实现 overlayfs 挂载点特征检测
  - [x] SubTask 19.4: 整合内核内存特征检测结果
  - [x] SubTask 19.5: 编写单元测试验证 KernelSU 检测准确性

- [x] Task 20: 专项检测 - LSPosed
  - [x] SubTask 20.1: 实现 `/data/adb/lspd` 目录深度检测
  - [x] SubTask 20.2: 整合内存中 liblspd.so 特征检测
  - [x] SubTask 20.3: 实现 ART 虚拟机 Hook 痕迹检测
  - [x] SubTask 20.4: 实现异常 Binder 调用检测
  - [x] SubTask 20.5: 编写单元测试验证 LSPosed 检测准确性

- [x] Task 21: 专项检测 - Shamiko
  - [x] SubTask 21.1: 实现 Zygisk 模块列表中 Shamiko 检测
  - [x] SubTask 21.2: 整合 Shamiko 内存特征检测
  - [x] SubTask 21.3: 实现 SELinux 上下文异常检测（Shamiko 特征）
  - [x] SubTask 21.4: 实现系统调用 Hook 模式检测（Shamiko 独特模式）
  - [x] SubTask 21.5: 编写单元测试验证 Shamiko 检测准确性

- [x] Task 22: 深度检测模式实现
  - [x] SubTask 22.1: 定义深度检测项集合（所有检测项）
  - [x] SubTask 22.2: 实现深度检测调度逻辑（10-15 秒完成）
  - [x] SubTask 22.3: UI 集成深度检测模式
  - [x] SubTask 22.4: 性能测试验证深度检测时间

## 阶段四：自保护与优化（第 7-8 周）

- [ ] Task 23: 自保护机制 - 代码混淆与加壳
  - [ ] SubTask 23.1: 配置 ProGuard/R8 代码混淆
  - [ ] SubTask 23.2: 实现关键检测代码的 Native 层混淆
  - [ ] SubTask 23.3: 实现 APK 加壳保护
  - [ ] SubTask 23.4: 验证混淆后检测功能正常

- [x] Task 24: 自保护机制 - 自校验与反调试
  - [x] SubTask 24.1: 实现代码完整性定期校验
  - [x] SubTask 24.2: 实现数据完整性校验
  - [x] SubTask 24.3: 实现反调试检测（ptrace 检测）
  - [x] SubTask 24.4: 实现反注入检测
  - [x] SubTask 24.5: 编写单元测试验证自保护机制

- [x] Task 25: 自保护机制 - 多进程验证与随机化
  - [x] SubTask 25.1: 实现多进程检测架构
  - [x] SubTask 25.2: 实现进程间相互验证机制
  - [x] SubTask 25.3: 实现检测顺序随机化
  - [x] SubTask 25.4: 实现检测内容随机化
  - [x] SubTask 25.5: 编写单元测试验证多进程验证

- [x] Task 27: 检测报告与可视化
  - [x] SubTask 27.1: 设计检测报告数据结构
  - [x] SubTask 27.2: 实现分层检测结果汇总
  - [x] SubTask 27.3: 实现威胁等级评估算法
  - [x] SubTask 27.4: 设计报告 UI 界面（分层展示、威胁等级颜色标识）
  - [x] SubTask 27.5: 实现检测报告导出功能（PDF/JSON）
  - [x] SubTask 27.6: 编写单元测试验证报告生成

- [ ] Task 28: 修复建议系统
  - [ ] SubTask 28.1: 建立问题-修复建议映射数据库
  - [ ] SubTask 28.2: 实现针对每个检测项的具体修复建议生成
  - [ ] SubTask 28.3: UI 集成修复建议展示
  - [ ] SubTask 28.4: 编写单元测试验证修复建议准确性

- [ ] Task 29: 性能优化
  - [ ] SubTask 29.1: 优化检测算法时间复杂度
  - [ ] SubTask 29.2: 优化内存扫描性能
  - [ ] SubTask 29.3: 实现检测结果缓存与增量检测
  - [ ] SubTask 29.4: 性能测试与调优（确保各模式时间达标）
  - [ ] SubTask 29.5: 内存泄漏检测与修复

- [ ] Task 30: 全面测试与 Bug 修复
  - [ ] SubTask 30.1: 在多种 Root 环境组合下测试（Magisk+Shamiko、KernelSU+LSPosed 等）
  - [ ] SubTask 30.2: 在 Android 10-15 各版本上测试
  - [ ] SubTask 30.3: 在不同设备型号上测试（华为、小米、OPPO、vivo、三星等）
  - [ ] SubTask 30.4: 修复发现的所有 Bug
  - [ ] SubTask 30.5: 回归测试验证修复

- [ ] Task 31: 发布准备
  - [ ] SubTask 31.1: 编写应用图标和启动页
  - [ ] SubTask 31.2: 编写应用描述和使用说明
  - [ ] SubTask 31.3: 生成签名 APK
  - [ ] SubTask 31.4: 准备发布材料（截图、功能介绍）
  - [ ] SubTask 31.5: 发布第一个公开版本

# 任务依赖关系

- [Task 2] 依赖于 [Task 1]
- [Task 3] 依赖于 [Task 1]
- [Task 4] 依赖于 [Task 1]
- [Task 5] 依赖于 [Task 1]
- [Task 6] 依赖于 [Task 2, Task 3, Task 4, Task 5]
- [Task 7] 依赖于 [Task 3]
- [Task 8] 依赖于 [Task 3]
- [Task 9] 依赖于 [Task 3]
- [Task 10] 依赖于 [Task 4]
- [Task 11] 依赖于 [Task 7, Task 8, Task 9, Task 10]
- [Task 12] 依赖于 [Task 11]
- [Task 13] 依赖于 [Task 4]
- [Task 14] 依赖于 [Task 3]
- [Task 15] 依赖于 [Task 9]
- [Task 16] 依赖于 [Task 8]
- [Task 17] 依赖于 [Task 14]
- [Task 18] 依赖于 [Task 13, Task 14]
- [Task 19] 依赖于 [Task 7, Task 15]
- [Task 20] 依赖于 [Task 10, Task 13]
- [Task 21] 依赖于 [Task 17, Task 18]
- [Task 22] 依赖于 [Task 13, Task 14, Task 15, Task 16, Task 17, Task 18, Task 19, Task 20, Task 21]
- [Task 23] 依赖于 [Task 22]
- [Task 24] 依赖于 [Task 23]
- [Task 25] 依赖于 [Task 24]
- [Task 26] 依赖于 [Task 22]（可选）
- [Task 27] 依赖于 [Task 22]
- [Task 28] 依赖于 [Task 27]
- [Task 29] 依赖于 [Task 25, Task 27, Task 28]
- [Task 30] 依赖于 [Task 29]
- [Task 31] 依赖于 [Task 30]

# 并行任务

以下任务可以并行开发：
- [Task 2, Task 3, Task 4] 可以并行（都依赖 Task 1）
- [Task 7, Task 8, Task 9] 可以并行（都依赖 Task 3）
- [Task 13, Task 14, Task 15] 可以并行（分别依赖 Task 4, Task 3, Task 9）
- [Task 18, Task 19, Task 20, Task 21] 可以并行（专项检测相互独立）

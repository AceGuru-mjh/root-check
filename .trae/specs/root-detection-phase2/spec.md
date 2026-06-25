# Root 环境检测软件第二阶段完善 Spec

## Why
基于已完成的基础架构和核心检测功能（Task 1-12），需要继续实现深度内存指纹扫描、专项 Root 工具检测、自保护机制、性能优化和发布准备，以达到超越现有工具（momo、ruru、春秋检测、牛头检测）的目标。

## What Changes
- 实现第 3 层增强：深度内存指纹扫描（Magisk/Zygisk/LSPosed/Shamiko 特征库）
- 实现第 5 层增强：进程与线程深度检测（直接 syscall 读取 /proc、SELinux 上下文检测）
- 实现第 6 层：内核模块检测（/proc/modules、/sys/modules 扫描）
- 实现第 5 层：系统调用时间分析（openat、readlinkat、getdents64 时间测量）
- 实现第 5 层：SELinux 上下文跳变检测
- 实现专项检测：Magisk/Zygisk、KernelSU、LSPosed、Shamiko 专属检测
- 实现深度检测模式（10-15 秒完成所有检测项）
- 实现自保护机制：代码混淆、自校验、反调试、多进程验证、随机化
- 实现第 7 层：硬件 TEE 检测（可选）
- 实现检测报告与修复建议系统
- 性能优化：快速检测 <500ms、标准检测 <3s、深度检测 <10s
- 全面测试与 Bug 修复
- 发布准备：UI 完善、签名 APK、发布材料

## Impact
- Affected specs: root-env-detection（延续）
- Affected code: 
  - Native 层：新增内存指纹扫描、进程检测、内核模块检测、系统调用时间分析等模块
  - Kotlin 层：新增专项检测器、深度检测模式、修复建议系统
  - UI 层：完善报告界面、修复建议展示
  - 构建配置：ProGuard 混淆、APK 加壳

## ADDED Requirements

### Requirement: 深度内存指纹扫描
系统 SHALL 扫描 `/proc/self/mem` 中所有可执行内存区域，建立 Magisk、Zygisk、LSPosed、Shamiko 的内存特征库，通过特征匹配算法检测这些 Root 工具的存在。

#### Scenario: 内存指纹检测
- **WHEN** 执行深度内存扫描
- **THEN** 读取 `/proc/self/mem` 可执行区域，匹配内存特征库，报告命中的 Root 工具及其内存特征

### Requirement: 进程与线程深度检测
系统 SHALL 通过直接 syscall(SYS_getdents64) 读取 `/proc` 目录，检测异常进程（magiskd、ksud 等），检查线程 SELinux 上下文，识别 u:r:magisk:s0 等异常上下文。

#### Scenario: 进程深度检测
- **WHEN** 执行进程深度检测
- **THEN** 使用直接 syscall 读取进程列表，检测异常进程和 SELinux 上下文，报告可疑进程

### Requirement: 内核模块检测
系统 SHALL 扫描 `/proc/modules` 和 `/sys/modules` 目录，检测非官方内核模块加载，检查内核符号表异常（kallsyms_lookup_name 导出），检测内核内存中的异常代码段。

#### Scenario: 内核模块检测
- **WHEN** 执行内核模块检测
- **THEN** 扫描内核模块目录，识别非官方模块，报告可疑内核模块和符号表异常

### Requirement: 系统调用时间分析
系统 SHALL 测量 openat、readlinkat、getdents64 等敏感系统调用的执行时间，建立基准设备执行时间数据库，检测时间异常（可能被 Hook）。

#### Scenario: 系统调用时间分析
- **WHEN** 执行系统调用时间分析
- **THEN** 测量各系统调用执行时间，与基准对比，报告时间异常的系統调用

### Requirement: SELinux 上下文跳变检测
系统 SHALL 在应用启动的不同阶段多次检测自身 SELinux 上下文，记录上下文变化历史，检测异常上下文跳变（Shamiko 特征），检测 `/sys/fs/selinux/access` 异常行为。

#### Scenario: SELinux 跳变检测
- **WHEN** 执行 SELinux 上下文检测
- **THEN** 在不同启动阶段检测上下文，记录变化历史，报告异常跳变和 SELinux 异常行为

### Requirement: Magisk/Zygisk 专项检测
系统 SHALL 针对 Magisk/Zygisk 实现专项检测：检测 `/data/adb/magisk` 目录、Zygote 进程注入、Magisk 套接字（/dev/socket/magiskd）、内存中 libmagisk.so/libzygisk.so 特征。

#### Scenario: Magisk 专项检测
- **WHEN** 执行 Magisk 专项检测
- **THEN** 检测 Magisk 目录、套接字、Zygote 注入、内存特征，报告 Magisk 存在性及其证据

### Requirement: KernelSU 专项检测
系统 SHALL 针对 KernelSU 实现专项检测：检测 `/data/adb/ksu` 目录、内核版本字符串（KernelSU、KSU）、overlayfs 挂载点特征、内核内存特征。

#### Scenario: KernelSU 专项检测
- **WHEN** 执行 KernelSU 专项检测
- **THEN** 检测 KernelSU 目录、内核字符串、overlayfs 特征、内存特征，报告 KernelSU 存在性

### Requirement: LSPosed 专项检测
系统 SHALL 针对 LSPosed 实现专项检测：检测 `/data/adb/lspd` 目录、内存中 liblspd.so 特征、ART 虚拟机 Hook 痕迹、异常 Binder 调用。

#### Scenario: LSPosed 专项检测
- **WHEN** 执行 LSPosed 专项检测
- **THEN** 检测 LSPosed 目录、内存特征、ART Hook 痕迹、Binder 调用，报告 LSPosed 存在性

### Requirement: Shamiko 专项检测
系统 SHALL 针对 Shamiko 实现专项检测：检测 Zygisk 模块列表中的 Shamiko、内存特征、SELinux 上下文异常（Shamiko 特征）、系统调用 Hook 模式（Shamiko 独特模式）。

#### Scenario: Shamiko 专项检测
- **WHEN** 执行 Shamiko 专项检测
- **THEN** 检测 Zygisk 模块、内存特征、SELinux 异常、系统调用 Hook 模式，报告 Shamiko 存在性

### Requirement: 深度检测模式
系统 SHALL 提供深度检测模式，运行所有检测项（包括内核级和侧信道检测），在 10-15 秒内完成，提供最全的 Root 环境检测。

#### Scenario: 深度检测模式
- **WHEN** 用户选择深度检测模式
- **THEN** 运行所有检测项，在 10-15 秒内完成，展示完整的检测结果

### Requirement: 代码混淆与加壳
系统 SHALL 使用 ProGuard/R8 进行代码混淆，实现关键检测代码的 Native 层混淆，实现 APK 加壳保护，防止被反编译和 Hook。

#### Scenario: 代码混淆生效
- **WHEN** 生成发布版本 APK
- **THEN** 代码经过混淆和加壳，难以被反编译和 Hook

### Requirement: 自校验与反调试
系统 SHALL 实现代码完整性定期校验、数据完整性校验、反调试检测（ptrace 检测）、反注入检测，确保检测软件自身不被篡改。

#### Scenario: 自校验与反调试
- **WHEN** 检测软件被调试、注入或篡改
- **THEN** 检测到异常并标记为 Root 环境，检测结果不受干扰

### Requirement: 多进程验证与随机化
系统 SHALL 实现多进程检测架构，进程间相互验证，检测顺序随机化，检测内容随机化，防止被侧信道分析。

#### Scenario: 多进程验证与随机化
- **WHEN** 执行检测
- **THEN** 多个进程相互验证，检测顺序和内容随机化，防止被预测和绕过

### Requirement: 硬件 TEE 检测（可选）
系统 SHALL 研究 Android TEE 接口，如果可行则实现 TEE 完整性检测，利用硬件可信执行环境验证系统完整性。

#### Scenario: TEE 检测
- **WHEN** 执行 TEE 检测（如果可行）
- **THEN** 利用 TEE 接口验证系统完整性，报告 TEE 检测结果

### Requirement: 检测报告与修复建议
系统 SHALL 设计检测报告数据结构，实现分层检测结果汇总，实现威胁等级评估算法，设计报告 UI 界面（分层展示、威胁等级颜色标识），实现检测报告导出功能（PDF/JSON），建立问题-修复建议映射数据库，生成针对每个检测项的具体修复建议。

#### Scenario: 检测报告与修复建议
- **WHEN** 检测完成
- **THEN** 生成分层检测报告，展示威胁等级，提供具体修复建议，支持导出

### Requirement: 性能优化
系统 SHALL 优化检测算法时间复杂度，优化内存扫描性能，实现检测结果缓存与增量检测，确保各模式时间达标（快速 <500ms、标准 <3s、深度 <10s），检测并修复内存泄漏。

#### Scenario: 性能优化
- **WHEN** 执行各模式检测
- **THEN** 快速检测 <500ms、标准检测 <3s、深度检测 <10s，内存占用合理，无内存泄漏

### Requirement: 全面测试与 Bug 修复
系统 SHALL 在多种 Root 环境组合下测试（Magisk+Shamiko、KernelSU+LSPosed 等），在 Android 10-15 各版本上测试，在不同设备型号上测试（华为、小米、OPPO、vivo、三星等），修复发现的所有 Bug，进行回归测试验证修复。

#### Scenario: 全面测试
- **WHEN** 在各种设备和 Root 环境下测试
- **THEN** 所有功能正常，Bug 修复完成，回归测试通过

### Requirement: 发布准备
系统 SHALL 设计应用图标和启动页，编写应用描述和使用说明，生成签名 APK，准备发布材料（截图、功能介绍），发布第一个公开版本。

#### Scenario: 发布准备
- **WHEN** 准备发布
- **THEN** 应用图标、启动页、描述、说明完成，签名 APK 生成，发布材料准备完成，成功发布

## MODIFIED Requirements

### Requirement: 分级检测模式（修改）
系统 SHALL 提供三种检测模式：
- **快速检测**（<500ms）：仅运行最关键的检测项（系统属性、基础文件检测、基础内存扫描）
- **标准检测**（<3s）：运行大部分检测项（包括命名空间对比、APatch 侧信道、系统调用表检查、ART 检测）
- **深度检测**（<10s）：运行所有检测项，包括内核级检测、侧信道检测、内存指纹扫描、专项检测

#### Scenario: 模式选择（修改）
- **WHEN** 用户选择检测模式并启动
- **THEN** 按所选模式执行对应检测项集合，在预期时间内完成并展示结果

## REMOVED Requirements
无

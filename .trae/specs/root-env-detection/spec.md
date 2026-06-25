# 超一流 Root 环境检测软件 Spec

## Why
现有 Root 检测工具（春秋检测、牛头检测、momo、ruru、Duck Detector）均只在用户空间做检测，依赖已知特征库，对 APatch/KernelSU 内核级隐藏、Shamiko+ZygiskNext 组合、KPM 等新型隐藏技术检测率极低。需要一款具备 7 层深度检测架构、侧信道分析、内核完整性校验能力的全新产品。

## What Changes
- 新建 Android 项目，采用 Kotlin + C/C++ (JNI/NDK) 混合架构
- 实现 7 层检测架构（应用层 → ART 虚拟机层 → 内存/进程层 → 文件系统/挂载层 → 系统调用/进程行为层 → 内核完整性层 → 硬件 TEE 层）
- 实现 APatch 侧信道时间差检测（独家技术）
- 实现多线程挂载命名空间对比检测（独家技术）
- 实现无 API 依赖的直接 syscall 检测（防 Hook）
- 实现自保护机制（反调试、反注入、反 Hook）
- 实现分级检测模式（快速/标准/深度）
- 实现可视化检测报告与修复建议

## Impact
- Affected specs: 全新项目，无已有 spec
- Affected code: 全新 Android 项目（Kotlin + C/C++ NDK）

## ADDED Requirements

### Requirement: 7 层检测架构
系统 SHALL 实现 7 层纵深检测，从应用层穿透到硬件层：

#### Scenario: 完整检测流程
- **WHEN** 用户启动检测
- **THEN** 系统按层级依次执行检测并汇总结果
  - 第 1 层：应用层 API 与系统属性检测
  - 第 2 层：Java/ART 虚拟机异常检测
  - 第 3 层：内存与进程空间深度扫描
  - 第 4 层：文件系统与挂载命名空间检测
  - 第 5 层：系统调用与进程行为分析
  - 第 6 层：内核完整性与行为检测
  - 第 7 层：硬件可信执行环境 (TEE) 检测

### Requirement: 应用层与系统属性检测
系统 SHALL 直接读取 `/dev/__properties__` 文件解析所有系统属性，不使用 `System.getProperty()` 或 `getprop` 命令，检测包含 "magisk"、"zygisk"、"su"、"ksu"、"apatch" 等关键词的属性，并对比设备型号官方基线检测被篡改的属性。

#### Scenario: 属性检测
- **WHEN** 执行应用层检测
- **THEN** 直接解析 `/dev/__properties__` 二进制文件，匹配 Root 相关关键词，标记异常属性

### Requirement: ART 虚拟机异常检测
系统 SHALL 扫描 ClassLoader 链中的非系统类加载器，检测 Dex 文件加载来源是否来自 `/data/adb`，检查 `/proc/self/maps` 中匿名内存映射的可执行代码，对比 `libart.so` 函数内存哈希与原始哈希检测 Hook。

#### Scenario: ART 检测
- **WHEN** 执行 ART 虚拟机检测
- **THEN** 报告非系统 ClassLoader、可疑 Dex 来源、匿名可执行内存映射、被 Hook 的 ART 函数

### Requirement: 内存与进程空间深度扫描
系统 SHALL 扫描 `/proc/self/mem` 中所有可执行内存区域，匹配 Magisk/Zygisk/LSPosed/Shamiko 内存特征，检测 `/memfd:jit-cache (deleted)` 区域，搜索内存中的 Root 工具关键词字符串。

#### Scenario: 内存扫描
- **WHEN** 执行内存检测
- **THEN** 报告匹配到的 Root 工具内存特征和可疑字符串

### Requirement: 文件系统与挂载命名空间检测
系统 SHALL 使用直接 syscall（`SYS_access`、`SYS_openat`）而非标准库函数检测文件存在性，扫描超过 100 个 su 路径变种，检测 `/proc/kallsyms` 权限异常，并通过多线程挂载命名空间对比检测 Magisk/KernelSU 的进程级隐藏。

#### Scenario: 挂载命名空间对比
- **WHEN** 执行挂载检测
- **THEN** 在主进程和新进程中分别读取 `/proc/self/mountinfo`，对比差异，发现差异则判定存在命名空间操纵

### Requirement: 系统调用与进程行为分析
系统 SHALL 通过 `syscall()` 指令直接调用内核（不经过 libc 包装），使用 `SYS_getdents64` 读取 `/proc` 目录，检测异常进程（magiskd、ksud），检查线程 SELinux 上下文异常。

#### Scenario: 系统调用检测
- **WHEN** 执行系统调用层检测
- **THEN** 通过直接 syscall 绕过 Hook，报告异常进程和 SELinux 上下文

### Requirement: 内核完整性检测
系统 SHALL 从 `/proc/kallsyms` 获取系统调用表地址，计算系统调用函数哈希并与基准对比，检测系统调用 Hook（特别是 `open`、`read`、`write`、`stat`、`truncate`），扫描 `/proc/modules` 和 `/sys/modules` 检测非官方内核模块。

#### Scenario: 内核检测
- **WHEN** 执行内核完整性检测
- **THEN** 报告被 Hook 的系统调用和可疑内核模块

### Requirement: APatch 侧信道时间差检测
系统 SHALL 通过将线程绑定到固定 CPU 核心，执行 10000 次 `truncate("/dev/null", 0xffff)` 和 `truncate("/dev/null", 0x10000)`，计算平均执行时间差，当时间差超过 500 纳秒时判定存在 APatch。

#### Scenario: APatch 侧信道检测
- **WHEN** 执行侧信道检测
- **THEN** 绑定 CPU 核心 → 预热 → 测量两组 truncate 调用时间差 → 与阈值对比 → 报告 APatch 存在性

### Requirement: 系统调用时间分析
系统 SHALL 测量 `openat`、`readlinkat`、`getdents64` 等敏感系统调用的执行时间，与基准对比，时间异常则标记为可能被 Hook。

#### Scenario: 时间分析
- **WHEN** 执行系统调用时间分析
- **THEN** 报告执行时间异常的系統调用及其偏差程度

### Requirement: SELinux 上下文跳变检测
系统 SHALL 在应用启动的不同阶段多次检测自身 SELinux 上下文，检测上下文异常变化和 `/sys/fs/selinux/access` 的异常行为。

#### Scenario: SELinux 检测
- **WHEN** 执行 SELinux 上下文检测
- **THEN** 报告上下文跳变和异常 SELinux 行为

### Requirement: 专项 Root 工具检测
系统 SHALL 针对以下工具实现专项检测：
- **Magisk/Zygisk**：检测 `/data/adb/magisk` 目录、内存中 `libmagisk.so`/`libzygisk.so` 特征、Zygote 注入、Magisk 套接字
- **KernelSU**：检测 `/data/adb/ksu` 目录、内核版本字符串、overlayfs 特征、内核内存特征
- **LSPosed**：检测 `/data/adb/lspd` 目录、内存中 `liblspd.so` 特征、ART Hook 痕迹、异常 Binder 调用
- **Shamiko**：检测 Zygisk 模块列表中的 Shamiko、内存特征、SELinux 上下文异常、系统调用 Hook 模式

#### Scenario: 专项检测
- **WHEN** 执行专项检测
- **THEN** 逐一检测各 Root 工具的特征，报告命中的工具及其具体证据

### Requirement: 自保护机制
系统 SHALL 实现以下自保护措施：
- 代码混淆与加壳
- 定期自校验代码和数据完整性
- 反调试与反注入检测
- 多进程相互验证
- 检测顺序随机化
- 全部检测本地完成，无网络依赖

#### Scenario: 自保护生效
- **WHEN** 检测软件被调试、注入或 Hook
- **THEN** 检测到异常并标记为 Root 环境，检测结果不受干扰

### Requirement: 分级检测模式
系统 SHALL 提供三种检测模式：
- **快速检测**（<1 秒）：仅运行最关键的检测项
- **标准检测**（3-5 秒）：运行大部分检测项
- **深度检测**（10-15 秒）：运行所有检测项，包括内核级和侧信道检测

#### Scenario: 模式选择
- **WHEN** 用户选择检测模式并启动
- **THEN** 按所选模式执行对应检测项集合，在预期时间内完成并展示结果

### Requirement: 检测报告与修复建议
系统 SHALL 生成可视化检测报告，包含：
- 各层级的检测结果和威胁等级
- 具体命中的检测项及其证据
- 针对每个问题的修复建议

#### Scenario: 报告展示
- **WHEN** 检测完成
- **THEN** 展示分层检测结果，标注威胁等级，提供修复建议

## MODIFIED Requirements
无（全新项目）

## REMOVED Requirements
无（全新项目）

# 顶级零后门隔离式 Root 检测全域架构 Spec

## Why
现有单进程架构存在致命缺陷：Java/Native 耦合在同一进程可被 Zygisk 全局 Hook 劫持；七层检测引擎串行执行，单层被篡改直接污染全部结果；自保护依附主进程，进程被 ptrace/注入后防护全部失效；特征库与引擎编译绑定无法离线热更新；TEE/内核层检测缺少硬件隔离通道；无密码学可信校验链路，所有检测结果可被内存篡改伪造；缺少对抗侧信道逆向的动态混淆调度单元。需要构建行业天花板级别的三进程物理隔离 + 微内核插件化 + 密码学可信闭环 + 裸汇编底层的全域安全架构。

## What Changes
- **BREAKING**: 彻底重构为三进程物理隔离架构（UI 域 A / 沙箱域 B / 守护域 C）+ 裸系统抽象层（域 D）
- **BREAKING**: 七层检测引擎拆分为独立静态插件 SO，微内核插件化调度，单模块崩溃不影响整体
- **BREAKING**: 建立可信密码学闭环，每层输出结果实时哈希签名，守护进程验签，篡改直接标记高危
- **BREAKING**: 实现双执行域分离：用户态标准检测 + 可选 SUID 隔离预加载检测（无权限也可通过原生 fork 隔离）
- **BREAKING**: 动态调度随机化硬件绑定：每次检测 CPU 核绑定、执行顺序、纳秒级随机噪声、内存地址随机映射
- **BREAKING**: 规则引擎与执行引擎彻底解耦：二进制规则库独立 assets，AES-256 加密，离线热更无需重新编译 APK
- **BREAKING**: 逆向免疫架构：代码分段内存自校验、多副本哈希比对、指令流动态变形、反 Hook 底层 syscall 汇编层
- **BREAKING**: TEE 可信通道旁路检测：不依赖系统 API 读取 TrustZone 节点，原生 /dev 节点裸读写绕过所有上层劫持
- **BREAKING**: 全域加密 IPC 通信：三层进程通信全部使用本地 Unix Socket，禁止 Binder（Binder 易被 LSPosed Hook），AES-GCM 带消息认证码，64bit 递增序列号防重放
- **BREAKING**: 独立可信守护信任根：持续监控全部进程内存完整性，周期性扫描 A/B 域内存段，检测到篡改直接标记全局 CRITICAL_ROOT_TAMPER

## Impact
- Affected specs: root-env-detection, root-detection-phase2（完全替代）
- Affected code: 
  - **全新架构**：三进程物理隔离（UI App / 沙箱检测进程 / 可信守护进程）
  - **域 A（前端可信交互域）**：纯 Kotlin Compose UI，零 NDK/JNI 接口，加密 Socket 客户端
  - **域 B（沙箱检测执行域）**：微内核调度器 + 7 层独立插件 SO + 规则引擎解析器 + 侧信道计时校准单元
  - **域 C（可信安全守护域）**：跨域内存校验 + 反调试监控 + 全局验签中心 + 内核轻量探针
  - **域 D（底层裸系统抽象层）**：全 arm64 汇编裸系统调用封装 + CPU 亲和控制 + 原始文件 mmap + 内存读写原语 + 底层哈希/AES 加密
  - **加密 IPC 层**：Unix Socket 加密通信 + AES-GCM 签名 + 时间戳防重放 + 消息序列号防篡改
  - **规则引擎**：AES-256 CBC 加密二进制指纹库 + 离线热更新系统
  - **逆向免疫子系统**：分段多副本自校验 + 指令流动态混淆 + 反 Hook 底层拦截 + 随机内存布局

## ADDED Requirements

### Requirement: 三进程物理隔离架构
系统 SHALL 实现三进程物理隔离：
- **域 A（UI 业务进程）**：纯 Kotlin Compose，无任何文件/proc 读取、无 syscall 调用，仅通过加密本地 Socket 与下层沙箱进程通信
- **域 B（沙箱检测执行进程）**：fork 独立分离进程，无 ART 依赖，纯 Native 沙箱，微内核插件化 7 层检测引擎
- **域 C（可信安全守护进程）**：常驻独立守护进程，最高优先级，实时监控 A/B 域，双进程互校验、反调试、内存篡改扫描、结果验签、进程注入拦截、内核探针

#### Scenario: 进程隔离验证
- **WHEN** 启动应用
- **THEN** 三个进程独立运行，互相无共享内存，私有 IPC 加密通信，A 域无法直接调用任何检测代码，B 域无权限访问 UI 内存，C 域具备独立内核探针能力

### Requirement: 域 A 前端可信交互域（Presentation 可信层）
系统 SHALL 实现域 A 纯展示层，硬性约束：
- 禁止导入任何 NDK JNI 接口、文件操作、进程读取 API
- 所有耗时任务、文件扫描、内核读取全部交由独立沙箱进程完成
- UI 收到报告必须携带守护进程签发的哈希签名，无签名直接判定报告伪造

包分层：
- `ui.compose`：液态玻璃 UI、分级检测面板、风险报告可视化、设备信息展示
- `viewmodel.trusted`：可信任务调度 VM，仅封装加密 IPC 请求结构体
- `ipc.client`：本地 Unix Socket 加密客户端，AES-GCM 签名、时间戳防重放、消息序列号防篡改
- `domain.trust.model`：全局统一可信数据模型（与底层完全解耦）
  - `ScanTask`：下发任务（检测等级、启用插件白名单、随机盐值）
  - `TrustedLayerResult`：带哈希签名的单层检测输出
  - `GlobalSecureReport`：全量验签后的完整风险报告
  - `SecurityAlert`：进程篡改、注入、调试器高危告警

#### Scenario: 可信任务下发
- **WHEN** 用户点击检测按钮
- **THEN** UI 层构造 ScanTask（检测等级 + 插件白名单 + 随机盐值 + 设备唯一密钥签名 + 时间戳），通过加密 Socket 发送至沙箱进程，等待带签名的检测报告返回

### Requirement: 域 B 沙箱检测执行域（微内核插件式 7 层检测引擎）
系统 SHALL 实现域 B 微内核调度器（`scan_micro_kernel.c`），核心能力：
- **插件动态加载器**：7 层检测全部为独立无依赖插件 .so，按需加载/卸载
- **动态乱序调度器**：每次执行随机打乱 7 层内部检测项顺序，随机插入 100~5000ns 休眠噪声
- **CPU 绑定单元**：每次检测随机绑定 1~2 个物理 CPU 核心，关闭调度切换干扰侧信道计时
- **内存隔离分配器**：每层插件分配独立匿名私有内存缓冲区，结果互不污染
- **规则引擎解析器**：加载 assets 内加密二进制指纹库（Magisk/KSU/APatch/LSPosed 内存特征、内核基线哈希、su 隐藏路径库）

#### Scenario: 微内核调度检测
- **WHEN** 沙箱进程接收检测任务
- **THEN** 微内核动态加载 7 层插件 SO，随机打乱执行顺序，每层分配独立内存缓冲区，CPU 绑定随机核心，插入纳秒级随机噪声，输出加密结果推送守护进程验签

### Requirement: 七层独立检测插件（完全解耦，单插件故障隔离）
系统 SHALL 实现 7 层独立插件，所有插件仅依赖底层 D 域汇编 syscall 接口，无 libc、无上层依赖，独立编译：

**Plugin L1：原始属性裸读插件**
- 直接 mmap `/dev/__properties__` 二进制解析，绕过 getprop
- 裸 openat 遍历全量隐藏 su/ksu/adb 路径
- 输出加密属性篡改标记

**Plugin L2：ART 虚拟机注入检测插件**
- 裸读取 `/proc/self/maps`，扫描匿名可执行内存
- libart.so 函数段本地哈希比对，检测 Inline Hook
- 异常 ClassLoader、`/data/adb` dex 映射识别

**Plugin L3：进程 & 内存指纹扫描插件**
- 纯 syscall `getdents64` 遍历 `/proc`，绕过进程隐藏
- `/proc/self/mem` 逐段匹配 Rootkit 内存特征
- `memfd` jit 可执行匿名区域检测 Zygisk 模块

**Plugin L4：命名空间挂载对比插件**
- 沙箱内 fork 子线程创建独立 mnt 命名空间
- 父子进程并行读取 mountinfo、fd、environ、status
- 差分对比识别 overlayfs 挂载劫持、目录隐藏

**Plugin L5：侧信道时延分析插件（APatch 专属检测）**
- 锁 CPU 频率、绑定固定核心消除调度误差
- `truncate` 系统调用纳秒级时间差采样
- `openat/readlinkat` 多轮基线时延比对识别 syscall Hook
- SELinux 上下文多阶段采样跳变检测 Shamiko

**Plugin L6：内核完整性探针插件**
- 裸读取 `/proc/kallsyms` 解析系统调用表基址
- 内核 syscall 函数段哈希校验，识别内核 Hook
- `/proc/modules/sys/modules` 非法内核模块扫描
- `kallsyms` 权限隐藏、内容擦除检测

**Plugin L7：TEE TrustZone 硬件可信旁路插件**
- 绕过 KeyStore 系统 API，裸读写 `/dev/tee`/`/dev/qseecom`
- TEE 分区镜像完整性校验
- 硬件设备基线指纹比对，识别底层篡改

#### Scenario: 插件独立执行
- **WHEN** 微内核调度某层插件
- **THEN** 插件独立加载，仅依赖 D 域汇编 syscall 接口，执行检测逻辑，输出加密结果到独立内存缓冲区，插件崩溃不影响其他层

### Requirement: 规则引擎独立模块（与检测插件解耦，支持离线热更）
系统 SHALL 实现规则引擎与执行引擎彻底解耦：
- **加密二进制规则库**：`fingerprint.bin.enc`，AES-256 CBC 加密
- **规则类型**：内存特征串、内核函数哈希、挂载特征、su 路径、syscall 时延阈值
- **动态热更逻辑**：启动时校验 assets 指纹库哈希，更新无需重编原生代码

#### Scenario: 规则热更新
- **WHEN** 启动应用或检测到规则库更新
- **THEN** 校验 assets 内 `fingerprint.bin.enc` 哈希，加载解密规则到内存，无需重新编译 APK 即可更新检测特征

### Requirement: 域 C 可信安全守护域（全局最高权限防护进程，信任根）
系统 SHALL 实现域 C 独立常驻进程，开机跟随 App fork 生成，三大核心模块：

**4.1 跨域内存可信校验模块**
- 周期性扫描 UI 进程、沙箱检测进程全部代码段
- 预存 libdetect 插件、主沙箱程序、classes.dex 多副本哈希
- 任意内存段 patch/注入/修改，立即生成高危签名告警

**4.2 反调试 & 进程监控模块**
- `ptrace` 附着检测、调试端口扫描、tracerpid 校验
- 监控沙箱进程是否被 Zygisk 注入、挂载劫持
- 拦截外部进程对检测沙箱的读写、ptrace 调试
- 检测 Shamiko 全局上下文篡改行为

**4.3 全局验签中心（架构信任根）**
- 沙箱每一层检测结果上传守护进程，计算 SHA3-512 哈希
- 守护进程使用内置设备唯一密钥对结果签名
- UI 层仅展示带合法签名的报告，无签名判定伪造
- 检测到篡改直接标记全局 `CRITICAL_ROOT_TAMPER`

**4.4 内核轻量探针**
- 通过裸 syscall 周期性读取内核关键节点，独立于沙箱检测流程，做二次交叉验证，杜绝单进程被劫持漏报

#### Scenario: 守护进程验签
- **WHEN** 沙箱层检测完成上传结果
- **THEN** 守护进程计算 SHA3-512 哈希，使用设备唯一密钥签名，UI 层展示带合法签名的报告，无签名或签名错误判定伪造

### Requirement: 域 D 底层裸系统汇编抽象层（无 libc，全架构底层支撑）
系统 SHALL 实现域 D，所有上层模块禁止调用 libc 标准函数（open/stat/read/write/access 等），统一调用本层汇编封装接口：

模块划分：
- `syscall_arm64.S`：纯汇编裸系统调用封装（`SYS_openat`、`SYS_getdents64`、`SYS_truncate` 等全部敏感调用）
- `cpu_control.c`：CPU 核心绑定、调频锁定、纳秒级高精度时钟（`CLOCK_MONOTONIC_RAW`）
- `raw_file_io.c`：原始 mmap、无缓存裸读写文件、节点解析原语
- `memory_raw.c`：进程内存裸读写、匿名内存分配、地址随机化工具
- `crypto_basic.c`：底层哈希、AES 加密工具（无第三方加密库，自研轻量化实现，防 Hook）

#### Scenario: 裸 syscall 调用
- **WHEN** 上层插件需要读取文件或进程信息
- **THEN** 调用 D 域汇编封装接口，直接触发 `svc 0` 系统调用，不经过 libc，规避所有 libc Hook

### Requirement: 全域加密 IPC 通信架构（防中间人劫持）
系统 SHALL 实现三层进程通信全部使用本地 Unix Socket，禁止 Binder：
- **UI ↔ 沙箱**：客户端单向下发任务，沙箱加密返回原始检测数据
- **沙箱 ↔ 可信守护**：双向加密管道，每层结果实时推送验签
- **通信安全策略**：
  - 每条消息附加 64bit 递增序列号，防重放攻击
  - 附加设备硬件随机盐值时间戳
  - AES-GCM 带消息认证码，篡改直接丢弃数据包并触发安全告警

#### Scenario: 加密通信
- **WHEN** UI 下发检测任务或沙箱上传结果
- **THEN** 消息附加 64bit 序列号 + 硬件随机盐时间戳 + AES-GCM 签名，接收方验签解密，篡改直接丢弃并触发告警

### Requirement: 逆向免疫自防护子系统（嵌入守护进程 + 沙箱微内核）
系统 SHALL 实现逆向免疫架构：

**分段多副本自校验**
- 原生程序拆分为 8 段独立哈希，存储于守护进程，每 3 秒循环比对，单段篡改立刻告警

**指令流动态混淆**
- 每次加载检测插件时，对内部指令流插入随机无运算空指令，运行时动态变形，静态逆向失效

**反 Hook 底层拦截**
- 裸 syscall 层内置检测，识别 libc、libart、libbinder 内联 Hook，直接标记环境风险

**随机内存布局**
- 插件加载基址随机映射，缓冲区地址每次检测重新分配，固定内存特征逆向无效

#### Scenario: 逆向免疫生效
- **WHEN** 攻击者尝试逆向或 Hook 检测进程
- **THEN** 分段自校验检测到篡改，指令流动态变形导致静态分析失效，反 Hook 底层识别 libc/libart Hook，随机内存布局使固定特征逆向无效

### Requirement: 工程目录落地（顶级架构标准分层，无代码耦合）
系统 SHALL 实现标准分层目录结构：

```
app/src/main/
├── java/com/rootguard/
│   ├── ui                // 域 A 前端可信交互域 Compose UI
│   ├── domain/trust      // 全域统一可信数据模型
│   ├── ipc/client        // 加密 Socket 客户端
│   └── viewmodel/trusted // 可信任务调度 VM
├── cpp/
│   ├── bare_syscall/     // 域 D 裸汇编底层系统抽象层（.S 汇编文件）
│   ├── sandbox_microkernel/ // 域 B 沙箱微内核调度器
│   │   ├── plugins/     // 7 层独立检测插件（L1~L7 独立子目录）
│   │   └── rule_parser/  // 加密规则库解析引擎
│   ├── trusted_daemon/   // 域 C 可信守护进程完整代码
│   ├── crypto/core       // 底层哈希/AES 加密原语
│   └── ipc/server        // 沙箱/守护进程加密 Socket 服务端
├── assets/
│   └── secure_fingerprint/ // AES 加密二进制特征规则库（可离线热更）
├── binary/
│   └── daemon_executable  // 独立守护进程原生可执行文件（非 SO，fork 独立进程）
└── CMakeLists.txt
    // 编译分离：7 层插件独立 SO、沙箱主程序、守护独立二进制、底层汇编 syscall 库
```

#### Scenario: 工程目录结构
- **WHEN** 构建项目
- **THEN** CMake 编译分离：7 层插件独立 SO、沙箱主程序、守护独立二进制、底层汇编 syscall 库，无代码耦合

### Requirement: 对比上一版架构的顶级优势（核心升级点）
系统 SHALL 实现以下核心升级：
1. 三进程物理隔离，彻底解决单进程全局 Hook 劫持
2. 插件微内核七层检测，单一层被篡改不影响其他层校验
3. 密码学可信闭环，所有检测结果带守护进程签名，无法内存伪造
4. 规则引擎与检测逻辑解耦，离线热更新特征库，无需重编 APK
5. 独立 TEE 旁路检测，绕过所有上层系统 API 劫持
6. 裸汇编系统调用底层，完全规避 libc/ART 全局 Hook
7. 独立可信守护信任根，持续监控全部进程内存完整性
8. 动态随机调度对抗逆向，消除固定执行时序、内存特征
9. 无 JNI 耦合依赖，UI 与原生代码完全通过加密 Socket 隔离，JNI 攻击面归零

#### Scenario: 架构优势验证
- **WHEN** 对比新旧架构
- **THEN** 新架构彻底解决单进程 Hook、单点篡改、特征库绑定、TEE 劫持、无密码学校验、固定执行时序等所有缺陷

### Requirement: 扩展兼容能力（面向未来所有新型 Rootkit）
系统 SHALL 支持扩展兼容：
- **新增 Root 隐藏工具**：仅需更新 assets 加密指纹库，或新增单层插件，上层 UI/守护进程零改动
- **新增硬件安全检测**：直接扩展 L7 TEE 插件，不改动微内核调度
- **新增检测算法（新型侧信道）**：独立新增插件，分层隔离
- **迁移底层实现（汇编转 Rust 裸 syscall）**：仅替换域 D 底层抽象层，上层全部兼容

#### Scenario: 扩展新检测能力
- **WHEN** 需要检测新型 Root 工具
- **THEN** 仅需更新 assets 加密指纹库或新增单层插件 SO，上层 UI/守护进程零改动，无需重新编译整个项目

## MODIFIED Requirements

### Requirement: 分级检测模式（修改）
系统 SHALL 提供三种检测模式：
- **快速检测**（<500ms）：仅运行最关键的检测项（L1 属性裸读 + L3 基础内存扫描 + L4 挂载对比）
- **标准检测**（<3s）：运行大部分检测项（L1~L5，包括侧信道时延分析）
- **深度检测**（<10s）：运行所有检测项（L1~L7，包括内核探针和 TEE 旁路检测）

所有检测任务通过加密 IPC 下发至沙箱进程执行，UI 层仅展示带守护进程签名的结果。

#### Scenario: 模式选择（修改）
- **WHEN** 用户选择检测模式并启动
- **THEN** UI 层构造加密任务下发至沙箱，沙箱微内核按模式加载对应插件集合，随机调度执行，结果加密推送守护进程验签，UI 展示带签名的报告

## REMOVED Requirements

### Requirement: 单进程 JNI 耦合架构
**Reason**: 单进程架构可被 Zygisk 全局 Hook 劫持全部逻辑，七层检测引擎串行执行，单层被篡改直接污染全部结果，自保护依附主进程，进程被 ptrace/注入后防护全部失效
**Migration**: 完全重构为三进程物理隔离架构，UI 与原生代码通过加密 Socket 隔离，JNI 攻击面归零

### Requirement: 七层检测引擎同进程串行执行
**Reason**: 单一层被篡改直接污染全部检测结果，无法隔离故障
**Migration**: 拆分为微内核插件化架构，7 层检测完全拆分为独立静态插件 SO，单模块崩溃不影响整体检测

### Requirement: 特征库与引擎编译绑定
**Reason**: 无法离线热更新，每次更新特征库需要重新编译 APK
**Migration**: 规则引擎与执行引擎彻底解耦，二进制规则库独立 assets，AES-256 加密，离线热更无需重新编译 APK

### Requirement: 无密码学可信校验链路
**Reason**: 所有检测结果可被内存篡改伪造，无法验证结果真实性
**Migration**: 建立可信密码学闭环，每一层输出结果实时哈希签名，守护进程验签，篡改直接标记高危

### Requirement: TEE/内核层检测依赖系统 API
**Reason**: 用户态可被完全屏蔽，系统 API 可被 Hook 劫持
**Migration**: TEE 可信通道旁路检测，不依赖系统 API 读取 TrustZone 节点，原生 /dev 节点裸读写绕过所有上层劫持

### Requirement: libc 标准函数调用
**Reason**: libc 可被全局 Hook，所有文件/进程操作可被劫持
**Migration**: 域 D 底层裸系统汇编抽象层，全 arm64 汇编裸系统调用封装，无 libc、无任何可 Hook 封装

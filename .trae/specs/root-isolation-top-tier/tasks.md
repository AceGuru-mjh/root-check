# 顶级零后门隔离式 Root 检测全域架构 - 任务列表

## 阶段一：基础架构搭建（第 1-2 周）

- [ ] Task 1: 域 D 底层裸系统汇编抽象层实现
  - [ ] SubTask 1.1: 创建 `bare_syscall/syscall_arm64.S` 纯汇编裸系统调用封装（SYS_openat、SYS_getdents64、SYS_truncate、SYS_read、SYS_write、SYS_mmap 等）
  - [ ] SubTask 1.2: 实现 `cpu_control.c` CPU 核心绑定（sched_setaffinity）、调频锁定、纳秒级高精度时钟（CLOCK_MONOTONIC_RAW）
  - [ ] SubTask 1.3: 实现 `raw_file_io.c` 原始 mmap、无缓存裸读写文件、节点解析原语
  - [ ] SubTask 1.4: 实现 `memory_raw.c` 进程内存裸读写、匿名内存分配、地址随机化工具
  - [ ] SubTask 1.5: 实现 `crypto_basic.c` 底层 SHA3-512 哈希、AES-256-GCM 加密工具（自研轻量化实现，无第三方库）
  - [ ] SubTask 1.6: 编写单元测试验证所有汇编 syscall 接口正确性

- [ ] Task 2: 加密 IPC 通信层实现
  - [ ] SubTask 2.1: 实现 `ipc/server/unix_socket_server.c` Unix Socket 服务端（沙箱端 + 守护进程端）
  - [ ] SubTask 2.2: 实现 `ipc/client/unix_socket_client.c` Unix Socket 客户端（UI 端）
  - [ ] SubTask 2.3: 实现 AES-GCM 加密解密模块，支持消息认证码
  - [ ] SubTask 2.4: 实现 64bit 递增序列号防重放机制
  - [ ] SubTask 2.5: 实现设备硬件随机盐值时间戳生成与校验
  - [ ] SubTask 2.6: 实现消息完整性校验，篡改直接丢弃并触发告警
  - [ ] SubTask 2.7: 编写单元测试验证加密通信安全性

- [ ] Task 3: 域 B 沙箱微内核调度器核心实现
  - [ ] SubTask 3.1: 实现 `sandbox_microkernel/micro_kernel.c` 插件动态加载器（dlopen/dlsym 封装，支持按需加载/卸载）
  - [ ] SubTask 3.2: 实现动态乱序调度器（Fisher-Yates 洗牌算法随机打乱 7 层执行顺序）
  - [ ] SubTask 3.3: 实现纳秒级随机休眠噪声插入（100~5000ns 随机）
  - [ ] SubTask 3.4: 实现 CPU 绑定单元（每次检测随机绑定 1~2 个物理 CPU 核心）
  - [ ] SubTask 3.5: 实现内存隔离分配器（每层插件分配独立匿名私有内存缓冲区，mmap MAP_ANONYMOUS）
  - [ ] SubTask 3.6: 实现插件接口标准定义（plugin_interface.h，统一输入输出结构）
  - [ ] SubTask 3.7: 编写单元测试验证微内核调度器功能

## 阶段二：七层独立检测插件实现（第 3-5 周）

- [ ] Task 4: Plugin L1 原始属性裸读插件
  - [ ] SubTask 4.1: 实现 mmap `/dev/__properties__` 二进制解析（绕过 getprop）
  - [ ] SubTask 4.2: 实现裸 openat 遍历全量隐藏 su/ksu/adb 路径（100+ 变种）
  - [ ] SubTask 4.3: 实现属性关键词匹配（magisk、zygisk、su、ksu、apatch 等）
  - [ ] SubTask 4.4: 实现属性篡改标记与加密输出
  - [ ] SubTask 4.5: 编译为独立 SO 插件，验证与微内核接口兼容

- [ ] Task 5: Plugin L2 ART 虚拟机注入检测插件
  - [ ] SubTask 5.1: 实现裸读取 `/proc/self/maps`，扫描匿名可执行内存
  - [ ] SubTask 5.2: 实现 libart.so 函数段本地哈希比对（检测 Inline Hook）
  - [ ] SubTask 5.3: 实现异常 ClassLoader 检测
  - [ ] SubTask 5.4: 实现 `/data/adb` dex 映射识别
  - [ ] SubTask 5.5: 编译为独立 SO 插件，验证与微内核接口兼容

- [ ] Task 6: Plugin L3 进程 & 内存指纹扫描插件
  - [ ] SubTask 6.1: 实现纯 syscall `getdents64` 遍历 `/proc`（绕过进程隐藏）
  - [ ] SubTask 6.2: 实现 `/proc/self/mem` 逐段匹配 Rootkit 内存特征（Magisk/Zygisk/LSPosed/Shamiko）
  - [ ] SubTask 6.3: 实现 `memfd` jit 可执行匿名区域检测（Zygisk 模块识别）
  - [ ] SubTask 6.4: 建立内存特征库（libmagisk.so、libzygisk.so、liblspd.so 特征）
  - [ ] SubTask 6.5: 编译为独立 SO 插件，验证与微内核接口兼容

- [ ] Task 7: Plugin L4 命名空间挂载对比插件
  - [ ] SubTask 7.1: 实现沙箱内 fork 子线程创建独立 mnt 命名空间（clone CLONE_NEWNS）
  - [ ] SubTask 7.2: 实现父子进程并行读取 mountinfo、fd、environ、status
  - [ ] SubTask 7.3: 实现差分对比算法识别 overlayfs 挂载劫持、目录隐藏
  - [ ] SubTask 7.4: 编译为独立 SO 插件，验证与微内核接口兼容

- [ ] Task 8: Plugin L5 侧信道时延分析插件（APatch 专属检测）
  - [ ] SubTask 8.1: 实现锁 CPU 频率、绑定固定核心消除调度误差
  - [ ] SubTask 8.2: 实现 `truncate` 系统调用纳秒级时间差采样（10000 次 0xffff vs 10000 次 0x10000）
  - [ ] SubTask 8.3: 实现 `openat/readlinkat` 多轮基线时延比对（识别 syscall Hook）
  - [ ] SubTask 8.4: 实现 SELinux 上下文多阶段采样跳变检测（Shamiko 识别）
  - [ ] SubTask 8.5: 实现时间差阈值判断（500 纳秒基准）
  - [ ] SubTask 8.6: 编译为独立 SO 插件，验证与微内核接口兼容

- [ ] Task 9: Plugin L6 内核完整性探针插件
  - [ ] SubTask 9.1: 实现裸读取 `/proc/kallsyms` 解析系统调用表基址
  - [ ] SubTask 9.2: 实现内核 syscall 函数段哈希校验（识别内核 Hook）
  - [ ] SubTask 9.3: 实现 `/proc/modules/sys/modules` 非法内核模块扫描
  - [ ] SubTask 9.4: 实现 `kallsyms` 权限隐藏、内容擦除检测
  - [ ] SubTask 9.5: 编译为独立 SO 插件，验证与微内核接口兼容

- [ ] Task 10: Plugin L7 TEE TrustZone 硬件可信旁路插件
  - [ ] SubTask 10.1: 实现绕过 KeyStore 系统 API，裸读写 `/dev/tee`/`/dev/qseecom`
  - [ ] SubTask 10.2: 实现 TEE 分区镜像完整性校验
  - [ ] SubTask 10.3: 实现硬件设备基线指纹比对（识别底层篡改）
  - [ ] SubTask 10.4: 编译为独立 SO 插件，验证与微内核接口兼容

## 阶段三：规则引擎与加密资产（第 6 周）

- [ ] Task 11: 规则引擎独立模块实现
  - [ ] SubTask 11.1: 设计加密二进制规则库格式（`fingerprint.bin.enc`，AES-256 CBC 加密）
  - [ ] SubTask 11.2: 实现规则类型定义（内存特征串、内核函数哈希、挂载特征、su 路径、syscall 时延阈值）
  - [ ] SubTask 11.3: 实现 `rule_parser/rule_parser.c` 规则库加载与解密
  - [ ] SubTask 11.4: 实现启动时校验 assets 指纹库哈希
  - [ ] SubTask 11.5: 实现动态热更逻辑（更新无需重编原生代码）
  - [ ] SubTask 11.6: 创建初始规则库文件（Magisk/KSU/APatch/LSPosed 特征）
  - [ ] SubTask 11.7: 编写单元测试验证规则引擎功能

## 阶段四：域 C 可信安全守护域实现（第 7-8 周）

- [ ] Task 12: 守护进程核心框架
  - [ ] SubTask 12.1: 实现 `trusted_daemon/daemon_main.c` 独立常驻进程入口（fork 独立进程，非 SO）
  - [ ] SubTask 12.2: 实现守护进程生命周期管理（开机跟随 App 启动，最高优先级）
  - [ ] SubTask 12.3: 实现与沙箱进程的双向加密管道通信
  - [ ] SubTask 12.4: 实现与 UI 进程的加密 Socket 通信
  - [ ] SubTask 12.5: 编写单元测试验证守护进程启动与通信

- [ ] Task 13: 跨域内存可信校验模块
  - [ ] SubTask 13.1: 实现周期性扫描 UI 进程、沙箱检测进程全部代码段（/proc/[pid]/mem）
  - [ ] SubTask 13.2: 实现预存 libdetect 插件、主沙箱程序、classes.dex 多副本哈希
  - [ ] SubTask 13.3: 实现内存段 patch/注入/修改检测
  - [ ] SubTask 13.4: 实现高危签名告警生成
  - [ ] SubTask 13.5: 编写单元测试验证内存校验功能

- [ ] Task 14: 反调试 & 进程监控模块
  - [ ] SubTask 14.1: 实现 `ptrace` 附着检测（PTRACE_TRACEME）
  - [ ] SubTask 14.2: 实现调试端口扫描（23946、8700 等）
  - [ ] SubTask 14.3: 实现 tracerpid 校验（/proc/self/status）
  - [ ] SubTask 14.4: 实现监控沙箱进程是否被 Zygisk 注入、挂载劫持
  - [ ] SubTask 14.5: 实现拦截外部进程对检测沙箱的读写、ptrace 调试
  - [ ] SubTask 14.6: 实现检测 Shamiko 全局上下文篡改行为
  - [ ] SubTask 14.7: 编写单元测试验证反调试功能

- [ ] Task 15: 全局验签中心（架构信任根）
  - [ ] SubTask 15.1: 实现沙箱每层检测结果接收与 SHA3-512 哈希计算
  - [ ] SubTask 15.2: 实现守护进程使用内置设备唯一密钥对结果签名
  - [ ] SubTask 15.3: 实现签名验证逻辑（UI 层调用）
  - [ ] SubTask 15.4: 实现无签名或签名错误判定伪造
  - [ ] SubTask 15.5: 实现检测到篡改直接标记全局 `CRITICAL_ROOT_TAMPER`
  - [ ] SubTask 15.6: 编写单元测试验证验签功能

- [ ] Task 16: 内核轻量探针
  - [ ] SubTask 16.1: 实现通过裸 syscall 周期性读取内核关键节点（/proc/kallsyms、/proc/modules）
  - [ ] SubTask 16.2: 实现独立于沙箱检测流程的二次交叉验证
  - [ ] SubTask 16.3: 实现杜绝单进程被劫持漏报
  - [ ] SubTask 16.4: 编写单元测试验证内核探针功能

## 阶段五：逆向免疫自防护子系统（第 9 周）

- [ ] Task 17: 分段多副本自校验
  - [ ] SubTask 17.1: 实现原生程序拆分为 8 段独立哈希
  - [ ] SubTask 17.2: 实现守护进程存储多副本哈希
  - [ ] SubTask 17.3: 实现每 3 秒循环比对
  - [ ] SubTask 17.4: 实现单段篡改立刻告警
  - [ ] SubTask 17.5: 编写单元测试验证自校验功能

- [ ] Task 18: 指令流动态混淆
  - [ ] SubTask 18.1: 实现每次加载检测插件时对内部指令流插入随机无运算空指令
  - [ ] SubTask 18.2: 实现运行时动态变形
  - [ ] SubTask 18.3: 实现静态逆向失效
  - [ ] SubTask 18.4: 编写单元测试验证混淆功能

- [ ] Task 19: 反 Hook 底层拦截
  - [ ] SubTask 19.1: 实现裸 syscall 层内置检测
  - [ ] SubTask 19.2: 实现识别 libc、libart、libbinder 内联 Hook
  - [ ] SubTask 19.3: 实现直接标记环境风险
  - [ ] SubTask 19.4: 编写单元测试验证反 Hook 功能

- [ ] Task 20: 随机内存布局
  - [ ] SubTask 20.1: 实现插件加载基址随机映射（mmap MAP_FIXED_NOREPLACE + 随机地址）
  - [ ] SubTask 20.2: 实现缓冲区地址每次检测重新分配
  - [ ] SubTask 20.3: 实现固定内存特征逆向无效
  - [ ] SubTask 20.4: 编写单元测试验证随机布局功能

## 阶段六：域 A 前端可信交互域实现（第 10-11 周）

- [ ] Task 21: 域 A Kotlin 层架构搭建
  - [ ] SubTask 21.1: 创建 `ui/compose` 液态玻璃 UI 框架（Jetpack Compose）
  - [ ] SubTask 21.2: 实现分级检测面板（快速/标准/深度）
  - [ ] SubTask 21.3: 实现风险报告可视化（分层展示、威胁等级颜色标识）
  - [ ] SubTask 21.4: 实现设备信息展示
  - [ ] SubTask 21.5: 确保无任何 NDK JNI 接口、文件操作、进程读取 API

- [ ] Task 22: 可信任务调度与 IPC 客户端
  - [ ] SubTask 22.1: 实现 `viewmodel/trusted/TrustedViewModel.kt` 可信任务调度 VM
  - [ ] SubTask 22.2: 实现 `ipc/client/SecureSocketClient.kt` 本地 Unix Socket 加密客户端
  - [ ] SubTask 22.3: 实现 AES-GCM 签名、时间戳防重放、消息序列号防篡改
  - [ ] SubTask 22.4: 实现 `domain/trust/TrustModel.kt` 全局统一可信数据模型
    - ScanTask：下发任务（检测等级、启用插件白名单、随机盐值）
    - TrustedLayerResult：带哈希签名的单层检测输出
    - GlobalSecureReport：全量验签后的完整风险报告
    - SecurityAlert：进程篡改、注入、调试器高危告警
  - [ ] SubTask 22.5: 实现 UI 收到报告必须携带守护进程签发的哈希签名校验
  - [ ] SubTask 22.6: 实现无签名直接判定报告伪造
  - [ ] SubTask 22.7: 编写单元测试验证 IPC 客户端功能

- [ ] Task 23: 检测报告与修复建议系统
  - [ ] SubTask 23.1: 设计检测报告数据结构（GlobalSecureReport）
  - [ ] SubTask 23.2: 实现分层检测结果汇总
  - [ ] SubTask 23.3: 实现威胁等级评估算法
  - [ ] SubTask 23.4: 设计报告 UI 界面（分层展示、威胁等级颜色标识）
  - [ ] SubTask 23.5: 实现检测报告导出功能（PDF/JSON）
  - [ ] SubTask 23.6: 建立问题-修复建议映射数据库
  - [ ] SubTask 23.7: 实现针对每个检测项的具体修复建议生成
  - [ ] SubTask 23.8: UI 集成修复建议展示
  - [ ] SubTask 23.9: 编写单元测试验证报告生成

## 阶段七：工程化与构建配置（第 12 周）

- [ ] Task 24: CMake 构建配置
  - [ ] SubTask 24.1: 配置 CMakeLists.txt 编译分离：7 层插件独立 SO、沙箱主程序、守护独立二进制、底层汇编 syscall 库
  - [ ] SubTask 24.2: 配置 arm64-v8a 汇编编译器支持
  - [ ] SubTask 24.3: 配置插件 SO 独立编译与链接
  - [ ] SubTask 24.4: 配置守护进程独立二进制编译（非 SO，fork 独立进程）
  - [ ] SubTask 24.5: 验证所有模块编译成功

- [ ] Task 25: AndroidManifest 与进程配置
  - [ ] SubTask 25.1: 配置 AndroidManifest.xml 多进程声明
  - [ ] SubTask 25.2: 配置沙箱检测进程（:sandbox）
  - [ ] SubTask 25.3: 配置可信守护进程（:daemon）
  - [ ] SubTask 25.4: 配置进程权限与隔离策略
  - [ ] SubTask 25.5: 验证多进程启动正常

- [ ] Task 26: assets 加密规则库打包
  - [ ] SubTask 26.1: 创建 `assets/secure_fingerprint/` 目录
  - [ ] SubTask 26.2: 打包初始加密规则库 `fingerprint.bin.enc`
  - [ ] SubTask 26.3: 配置 Gradle 打包 assets
  - [ ] SubTask 26.4: 验证 assets 正确打包到 APK

## 阶段八：性能优化与测试（第 13-14 周）

- [ ] Task 27: 性能优化
  - [ ] SubTask 27.1: 优化检测算法时间复杂度
  - [ ] SubTask 27.2: 优化内存扫描性能
  - [ ] SubTask 27.3: 实现检测结果缓存与增量检测
  - [ ] SubTask 27.4: 性能测试与调优（确保各模式时间达标：快速 <500ms、标准 <3s、深度 <10s）
  - [ ] SubTask 27.5: 内存泄漏检测与修复

- [ ] Task 28: 全面测试与 Bug 修复
  - [ ] SubTask 28.1: 在多种 Root 环境组合下测试（Magisk+Shamiko、KernelSU+LSPosed 等）
  - [ ] SubTask 28.2: 在 Android 10-15 各版本上测试
  - [ ] SubTask 28.3: 在不同设备型号上测试（华为、小米、OPPO、vivo、三星等）
  - [ ] SubTask 28.4: 修复发现的所有 Bug
  - [ ] SubTask 28.5: 回归测试验证修复

## 阶段九：发布准备（第 15 周）

- [ ] Task 29: 发布准备
  - [ ] SubTask 29.1: 设计应用图标和启动页
  - [ ] SubTask 29.2: 编写应用描述和使用说明
  - [ ] SubTask 29.3: 生成签名 APK
  - [ ] SubTask 29.4: 准备发布材料（截图、功能介绍）
  - [ ] SubTask 29.5: 发布第一个公开版本

# 任务依赖关系

- [Task 1] 无依赖（底层基础）
- [Task 2] 依赖于 [Task 1]（需要底层加密原语）
- [Task 3] 依赖于 [Task 1, Task 2]（需要底层 syscall 和 IPC）
- [Task 4-10] 依赖于 [Task 1, Task 3]（需要底层 syscall 和微内核调度器）
- [Task 11] 依赖于 [Task 1]（需要底层加密原语）
- [Task 12] 依赖于 [Task 1, Task 2, Task 3]（需要底层 syscall、IPC、微内核）
- [Task 13-16] 依赖于 [Task 12]（需要守护进程框架）
- [Task 17-20] 依赖于 [Task 12, Task 13]（需要守护进程和内存校验）
- [Task 21-23] 依赖于 [Task 2, Task 15]（需要 IPC 和验签中心）
- [Task 24] 依赖于 [Task 1-12]（需要所有模块实现）
- [Task 25] 依赖于 [Task 12]（需要守护进程）
- [Task 26] 依赖于 [Task 11]（需要规则引擎）
- [Task 27] 依赖于 [Task 1-23]（需要所有功能实现）
- [Task 28] 依赖于 [Task 27]（需要性能优化）
- [Task 29] 依赖于 [Task 28]（需要测试通过）

# 并行任务

以下任务可以并行开发：
- [Task 4, Task 5, Task 6, Task 7, Task 8, Task 9, Task 10] 可以并行（7 层插件相互独立，都依赖 Task 1, Task 3）
- [Task 13, Task 14, Task 15, Task 16] 可以并行（守护进程四大模块，都依赖 Task 12）
- [Task 17, Task 18, Task 19, Task 20] 可以并行（逆向免疫四大子系统，都依赖 Task 12, Task 13）
- [Task 21, Task 22, Task 23] 可以并行（域 A 三大模块，都依赖 Task 2, Task 15）

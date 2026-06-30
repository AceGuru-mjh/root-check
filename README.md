---

<p align="center">
  <img src="https://img.shields.io/badge/Android-10%2B-3DDC84?logo=android&logoColor=white" alt="Android" />
  <img src="https://img.shields.io/badge/API-29%2B-critical" alt="API" />
  <img src="https://img.shields.io/badge/Arch-ARM64-blueviolet?logo=arm&logoColor=white" alt="ARM64" />
  <img src="https://img.shields.io/badge/NDK-28.2-FF6F00?logo=androidndk&logoColor=white" alt="NDK" />
  <img src="https://img.shields.io/badge/Kotlin-Compose-7F52FF?logo=kotlin&logoColor=white" alt="Kotlin" />
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white" alt="C++20" />
  <img src="https://img.shields.io/badge/Protobuf-Lite-AA344D?logo=protocolbuffers&logoColor=white" alt="Protobuf" />
  <img src="https://img.shields.io/badge/License-Proprietary-red" alt="License" />
</p>

<h1 align="center">环境检测 · Environment Detection</h1>
<h3 align="center">Android 设备完整性评估的巅峰之作</h3>
<h3 align="center">The Pinnacle of Android Integrity Assessment</h3>

<p align="center">
  <em>在感知安全与地面实况之间架起桥梁——一把分毫不差的诊断仪器。</em><br>
  <em>Bridging the chasm between perceived security and ground truth — a diagnostic instrument of uncompromising precision.</em>
</p>

<br>

---

<br>

## 目录 · Table of Contents

| 中文 | English |
|---|---|
| [序章 · 暗影中的真相](#序章--暗影中的真相) | [Prologue · Truth in the Shadows](#prologue--truth-in-the-shadows) |
| [使命宣言](#使命宣言) | [Mission Statement](#mission-statement) |
| [关于开发者](#关于开发者) | [About the Developer](#about-the-developer) |
| [核心能力矩阵](#核心能力矩阵) | [Core Capability Matrix](#core-capability-matrix) |
| [检测维度详解](#检测维度详解) | [Detection Dimensions in Detail](#detection-dimensions-in-detail) |
| [评估模式](#评估模式) | [Assessment Modes](#assessment-modes) |
| [风险分级模型](#风险分级模型) | [Risk Classification Model](#risk-classification-model) |
| [评分算法哲学](#评分算法哲学) | [Scoring Algorithm Philosophy](#scoring-algorithm-philosophy) |
| [报告系统](#报告系统) | [Report System](#report-system) |
| [用户界面理念](#用户界面理念) | [UI Philosophy](#ui-philosophy) |
| [兼容性矩阵](#兼容性矩阵) | [Compatibility Matrix](#compatibility-matrix) |
| [性能基准](#性能基准) | [Performance Benchmarks](#performance-benchmarks) |
| [生态与愿景](#生态与愿景) | [Ecosystem & Vision](#ecosystem--vision) |
| [设计原则](#设计原则) | [Design Tenets](#design-tenets) |
| [限制与边界](#限制与边界) | [Limitations & Boundaries](#limitations--boundaries) |
| [隐私承诺](#隐私承诺) | [Privacy Commitment](#privacy-commitment) |
| [常见问题](#常见问题) | [Frequently Asked Questions](#frequently-asked-questions) |
| [参与贡献](#参与贡献) | [Contributing](#contributing) |
| [许可证与法律声明](#许可证与法律声明) | [License & Legal](#license--legal) |
| [致谢](#致谢) | [Acknowledgements](#acknowledgements) |

<br>

---

<br>

## 序章 · 暗影中的真相
## Prologue · Truth in the Shadows

<br>

### 中文

在 Android 生态系统的深处，一场无声的战争从未停歇。这场战争的一方是那些寻求绝对掌控自己设备的人——他们解锁、修补、定制，将手中的硬件变成完全属于自己的工具。另一方则是那些试图隐藏这种掌控的人，他们的技艺日益精湛，留下的痕迹愈发难以捕捉。

Magisk、KernelSU、APatch、Zygisk、LSPosed、Shamiko、Frida——这些名字在安全研究的圈子里如雷贯耳，每一个都是一件精妙绝伦的工程艺术品。它们的能力早已超越了简单的"隐藏 root"，进化为完整的系统级 concealment 框架，能够欺骗几乎所有传统的检测手段。

然而，在这片日益复杂的迷雾中，一个问题始终悬而未决：

**我到底还能不能相信我的手机？**

**环境检测**正是为了回答这个问题而诞生。

它不是一个简单的"是或否"的检测器。它是一套完整的、多维度、多层次的**法医级鉴定仪器**。它的目标不是判断"是否被 root"，而是要回答一系列更深刻的问题：

- 我的设备处于什么样的安全状态？
- 哪些层次受到了影响？
- 影响的程度有多深？
- 这些修改是良性的还是恶意的？
- 有什么证据支持这些结论？
- 我该如何理解和应对这些发现？

每一个问题都是一条探索的路径，每一条路径都通向更深层的真相。

### English

Deep within the Android ecosystem, a silent war rages ceaselessly. On one side stand those who seek absolute dominion over their devices — unlocking, patching, customising, transforming their hardware into instruments of pure personal agency. On the other side stand those who seek to conceal this dominion, their craft growing ever more sophisticated, the traces they leave ever more elusive.

Magisk, KernelSU, APatch, Zygisk, LSPosed, Shamiko, Frida — these names resonate throughout the security research community, each a masterpiece of engineering artistry. Their capabilities have long since transcended simple "root hiding", evolving into complete system-level concealment frameworks capable of deceiving virtually all traditional detection methods.

Yet, amid this growing fog of complexity, one question remains unanswered:

**Can I still trust my phone?**

**Environment Detection** was born to answer this question.

It is not a simple "yes or no" detector. It is a complete, multi-dimensional, multi-layered **forensic-grade assessment instrument**. Its purpose is not to determine "whether rooted", but to answer a far more profound set of questions:

- What is the true security posture of my device?
- Which layers have been affected?
- How deep does the impact extend?
- Are these modifications benign or malicious?
- What evidence supports these conclusions?
- How should I understand and respond to these findings?

Each question is a path of exploration, and each path leads to deeper truth.

<br>

---

<br>

## 使命宣言
## Mission Statement

<br>

### 中文

**环境检测** 的核心信仰有三：

**第一，真相可以被发现，但需要正确的方法。** 现代 concealment 技术已经进化到能够欺骗大多数传统检测手段。要穿透这些伪装，需要的是深度、创造力和不懈的探究。每一种隐藏技术都有其物理或逻辑上的限制——总有某种侧信道、某种时序异常、某种内存特征会暴露它的存在。我们的任务就是找到这些突破口。

**第二，安全不是二元状态，而是一个连续的光谱。** "已 root"和"未 root"之间的世界远比想象中丰富。一台设备可能处于数十种不同的安全状态之一：纯原生系统但 Bootloader 已解锁、带有完整隐藏方案的 root 设备、被恶意软件部分入侵的非 root 设备、配置不当的企业设备……每一种状态都有其独特的含义和应对方式。

**第三，知识是赋能，而非恐吓。** 检测结果不是用来制造焦虑的。每一份报告都应该是清晰的、可理解的、可行动的。我们希望用户在阅读报告后，不是感到恐惧，而是感到——**"现在我了解真实情况了，我知道该怎么做了。"**

### English

**Environment Detection** is founded upon three core beliefs:

**First, truth can be discovered, but it requires the right methodology.** Modern concealment technologies have evolved to deceive most traditional detection methods. Penetrating these disguises requires depth, creativity, and relentless inquiry. Every hiding technique has its physical or logical limitations — there is always some side-channel, some timing anomaly, some memory signature that betrays its presence. Our mission is to find these points of exposure.

**Second, security is not a binary state, but a continuous spectrum.** The world between "rooted" and "not rooted" is far richer than imagined. A device may exist in dozens of distinct security states: stock with an unlocked bootloader, rooted with full concealment, partially compromised without root, misconfigured enterprise devices... Each state carries unique implications and appropriate responses.

**Third, knowledge empowers; it does not frighten.** Detection results are not meant to create anxiety. Every report should be clear, comprehensible, and actionable. We want users to walk away from a report not in fear, but with the conviction — **"Now I understand the true state of things, and I know what to do about it."**

<br>

---

<br>

## 关于开发者
## About the Developer

<br>

### 中文

**环境检测** 的诞生源于一个人的执着与热爱。

**开发者：mjh**

我是一个 Android 底层安全研究的深度爱好者，长期沉浸在 Android 框架、Linux 内核、ARM64 体系结构的世界中。这个项目始于一个简单的念头——"为什么没有一个真正深入、真正可靠的检测工具？"——然后一步步演变为数百个日日夜夜的钻研、编码、测试和迭代。

这个项目中的每一行代码，每一个检测算法，每一次侧信道测量的精度提升，都源于对技术真相的追寻。我相信工具应该有灵魂，而一个安全检测工具的灵魂就是**可信**和**精准**。

如果你对这个项目有任何想法、建议，或者发现了新的检测维度想要讨论，欢迎随时联系我：

| 联系方式 | 详细信息 |
|---|---|
| 📧 QQ | **25544240258** |
| 💬 微信 | **meng4117222** |

无论是技术探讨、问题反馈，还是单纯想要交流 Android 安全相关的话题，我都非常欢迎。

### English

**Environment Detection** was born from one person's dedication and passion.

**Developer: mjh**

I am a deep enthusiast of Android low-level security research, immersed in the world of the Android framework, the Linux kernel, and ARM64 architecture. This project began with a simple thought — "Why isn't there a truly thorough, truly reliable detection tool?" — and evolved into hundreds of days and nights of research, coding, testing, and iteration.

Every line of code in this project, every detection algorithm, every incremental improvement in side-channel measurement precision, stems from the pursuit of technical truth. I believe that tools should have a soul, and the soul of a security detection tool is **trustworthiness** and **precision**.

If you have any thoughts, suggestions, or new detection dimensions to discuss, please feel free to reach out:

| Contact | Details |
|---|---|
| 📧 QQ | **25544240258** |
| 💬 WeChat | **meng4117222** |

Whether for technical discussion, bug reports, or simply to chat about Android security topics, you are most welcome.

<br>

---

<br>

## 核心能力矩阵
## Core Capability Matrix

<br>

### 中文

**环境检测** 的核心能力可以用以下的矩阵来概括。这不仅仅是一个功能列表，更是一个完整的安全态势感知框架。

| 能力域 | 能力项 | 描述 |
|---|---|---|
| **属性检测** | 系统属性完整性 | 检测 ro.debuggable、ro.secure、ro.build.tags 等关键系统属性是否被篡改 |
| **属性检测** | 构建指纹验证 | 验证 build fingerprint、release-keys 签名等标识是否一致 |
| **属性检测** | SELinux 状态 | 检测 SELinux 是否处于 Enforcing 模式，上下文是否异常 |
| **运行时检测** | ART 注入检测 | 检测 ART/Dalvik 虚拟机是否被注入或挂钩 |
| **运行时检测** | ClassLoader 验证 | 验证类加载器链路的完整性和可信性 |
| **运行时检测** | JNI 表检查 | 检查 JNI 函数表是否被挂钩或篡改 |
| **内存取证** | 可执行区域扫描 | 扫描进程内存中的可执行区域，识别已知工具指纹 |
| **内存取证** | /proc/self/mem 分析 | 深度分析进程内存映射，寻找隐藏注入 |
| **内存取证** | 代码完整性校验 | 校验关键代码段是否在运行时被修改 |
| **文件系统** | 挂载命名空间检查 | 检测挂载命名空间是否被隔离或篡改 |
| **文件系统** | overlayfs 检测 | 检测 overlay 文件系统层的存在和内容 |
| **文件系统** | 可疑路径扫描 | 扫描已知的 root 工具路径和可疑二进制文件 |
| **行为分析** | 侧信道时序测量 | 通过精确的时序测量检测系统调用的延迟异常 |
| **行为分析** | 缓存时序分析 | 分析 CPU 缓存行为中的时序差异 |
| **行为分析** | 中断延迟测量 | 测量中断响应时间的异常波动 |
| **行为分析** | 缺页异常监控 | 监控缺页异常的频率和模式异常 |
| **行为分析** | Syscall 结果一致性 | 对比敏感路径 vs 无关路径 openat 结果差异（替代原 syscall 表 hook 检测） |
| **Root 守护进程** | Magisk 守护进程 | 扫描 /proc/*/cmdline 中的 magiskd / magisk32 / magisk64 / kitana |
| **Root 守护进程** | KSU 守护进程 | 扫描 ksud / ksud-next / sukid / sukisu-ultra |
| **Root 守护进程** | APatch 守护进程 | 扫描 apd / apatch / apatchd |
| **Root 守护进程** | service / post-fs-data 脚本 | /data/adb/{service.d, post-fs-data.d, modules.d, boot-completed.d} |
| **Root 守护进程** | 配置 DB 指纹 | /data/adb/magisk.db, /data/adb/ksu/db, /data/adb/ap/db 等 |
| **启动路径** | Bootloader 状态 | 检测 Bootloader 的锁定/解锁状态 |
| **启动路径** | AVB/dm-verity 状态 | 验证 Android Verified Boot 和 dm-verity 的状态 |
| **启动路径** | VBmeta 摘要验证 | 验证 vbmeta 分区的签名和摘要 |
| **启动路径** | 恢复模式检测 | 检测自定义恢复环境的存在 |
| **Root 方案指纹** | Magisk 检测 | 检测 Magisk 主流 + Delta/Kitsune/Kitana fork、守护进程与配置 |
| **Root 方案指纹** | KernelSU 检测 | 检测 KernelSU / SukiSU / KSU-NEXT 与 Manager APP 包名 |
| **Root 方案指纹** | APatch 检测 | 检测 APatch 主目录、APD、KPM 用户态模块、Manager APP |
| **Root 方案指纹** | Zygisk 检测 | 检测 Zygisk / ZygiskNext / ReZygisk 进程注入和模块加载 |
| **Hook 框架检测** | Xposed 检测 | 检测 Xposed 框架 (Riru 和 Zygisk 模式) + LSPatch |
| **Hook 框架检测** | LSPosed 检测 | 检测 LSPosed 模块管理器及其作用域 |
| **Hook 框架检测** | Frida 检测 | 检测 Frida server / gadget / agent 多种路径 |
| **Hook 框架检测** | Substrate/Cydia 检测 | 检测 Substrate 和 Cydia 框架 |
| **Hook 框架检测** | ART inline Hook | SandHook / Pine / Epic / Whale / ShadowHook / ByteHook |
| **自定义 ROM 检测** | ROM 指纹识别 | 识别超过 50 种已知的自定义 ROM（含 ArrowOS / Rising / Elixir / PixelOS / StatiX 等） |
| **自定义 ROM 检测** | 厂商修改检测 | 检测厂商特定的系统修改和定制 |
| **虚拟框架检测** | VirtualXposed | 检测 io.va.exposed 系列（免 root Xposed 注入） |
| **虚拟框架检测** | 太极 / TaiChi | 检测 me.weishu.exp 系列（太极·阳 / 太极·阴） |
| **虚拟框架检测** | 双开 / 分身空间 | 平行空间 / 分身大师 / 双开大师 / 微信分身 / 360 分身 |
| **虚拟框架检测** | 应用沙箱 | Island / Shelter / Insular / App Cloner |
| **危险应用检测** | 内存修改器 | GameGuardian / CheatEngine / GameKiller / SB Game Hacker / GameCIH |
| **危险应用检测** | 内购破解 | Lucky Patcher / Freedom / CreeHack / LocalIAPStore |
| **危险应用检测** | APK 破解 | AntiLVL / 八门神器 / 大师助手 |
| **Magisk 扩展检测** | DenyList | 检测 Magisk DenyList 配置（替代 MagiskHide） |
| **Magisk 扩展检测** | Zygisk 模块 | Zygisk / ZygiskNext / ReZygisk 模块路径与内存痕迹 |
| **Magisk 扩展检测** | LSPosed Manager | 检测 LSPosed Manager APP + LSPatch |
| **Magisk 扩展检测** | Riru 模块 | 检测 Riru 核心与 edxp / sandhook 模块 |
| **Magisk 扩展检测** | 现代 fork | Magisk Delta / Kitsune / Kitana / Alpha / Beta |
| **SELinux 分析** | 上下文跳转检测 | 检测 SELinux 上下文的异常跳转和转换 |
| **SELinux 分析** | 域违规审计 | 审计进程域的非正常转换和权限提升 |
| **反隐藏探测** | Shamiko 检测 | 检测 Shamiko Zygisk 隐藏模块（含新版路径） |
| **反隐藏探测** | ZygiskNext 检测 | 检测 ZygiskNext / ReZygisk 隐藏模块 |
| **反隐藏探测** | 进程隐藏检测 | 检测进程列表中的隐藏条目 |
| **反隐藏探测** | 命名空间隐藏检测 | 检测挂载命名空间的伪装和隔离 |
| **反隐藏探测** | HideMyApplist 检测 | 检测应用列表隐藏模块（含 hma 别名） |
| **反隐藏探测** | StorageIsolation 检测 | 检测 Rikka 存储隔离模块 |
| **反隐藏探测** | MagiskHide (legacy) | 检测老式 MagiskHide / magiskimg 残留 |

### English

The core capabilities of **Environment Detection** can be summarised in the following matrix. This is not merely a feature list, but a complete security situational awareness framework.

| Capability Domain | Capability Item | Description |
|---|---|---|
| **Property Detection** | System Property Integrity | Detects tampering of critical system properties (ro.debuggable, ro.secure, ro.build.tags, etc.) |
| **Property Detection** | Build Fingerprint Verification | Verifies consistency of build fingerprint and release-keys signatures |
| **Property Detection** | SELinux Status | Detects SELinux enforcing mode status and context anomalies |
| **Runtime Detection** | ART Injection Detection | Detects ART/Dalvik virtual machine injection or hooking |
| **Runtime Detection** | ClassLoader Verification | Verifies the integrity and trustworthiness of the class loader chain |
| **Runtime Detection** | JNI Table Inspection | Checks JNI function tables for hooks or tampering |
| **Memory Forensics** | Executable Region Scanning | Scans process memory executable regions for known tool fingerprints |
| **Memory Forensics** | /proc/self/mem Analysis | Deep analysis of process memory maps for hidden injections |
| **Memory Forensics** | Code Integrity Verification | Verifies critical code segments haven't been modified at runtime |
| **Filesystem** | Mount Namespace Inspection | Detects mount namespace isolation or tampering |
| **Filesystem** | overlayfs Detection | Detects overlay filesystem layer existence and content |
| **Filesystem** | Suspicious Path Scanning | Scans known root tool paths and suspicious binaries |
| **Behavioural Analysis** | Side-Channel Timing | Detects syscall latency anomalies through precision timing |
| **Behavioural Analysis** | Cache Timing Analysis | Analyses timing differentials in CPU cache behaviour |
| **Behavioural Analysis** | Interrupt Latency Measurement | Measures anomalous fluctuations in interrupt response times |
| **Behavioural Analysis** | Page Fault Monitoring | Monitors frequency and pattern anomalies in page faults |
| **Behavioural Analysis** | Syscall Result Consistency | Compares openat results on sensitive vs random paths (replaces syscall table hook check) |
| **Root Daemon** | Magisk Daemon | Scans /proc/*/cmdline for magiskd / magisk32 / magisk64 / kitana |
| **Root Daemon** | KSU Daemon | Scans for ksud / ksud-next / sukid / sukisu-ultra |
| **Root Daemon** | APatch Daemon | Scans for apd / apatch / apatchd |
| **Root Daemon** | service / post-fs-data scripts | /data/adb/{service.d, post-fs-data.d, modules.d, boot-completed.d} |
| **Root Daemon** | Config DB Fingerprints | /data/adb/magisk.db, /data/adb/ksu/db, /data/adb/ap/db, etc. |
| **Boot Path** | Bootloader Status | Detects bootloader locked/unlocked status |
| **Boot Path** | AVB/dm-verity Status | Verifies Android Verified Boot and dm-verity status |
| **Boot Path** | VBmeta Digest Verification | Verifies vbmeta partition signature and digest |
| **Boot Path** | Recovery Mode Detection | Detects custom recovery environment presence |
| **Root Fingerprinting** | Magisk Detection | Detects Magisk mainstream + Delta/Kitsune/Kitana forks, daemon, config |
| **Root Fingerprinting** | KernelSU Detection | Detects KernelSU / SukiSU / KSU-NEXT and Manager APP package names |
| **Root Fingerprinting** | APatch Detection | Detects APatch main dir, APD, KPM user-space modules, Manager APP |
| **Root Fingerprinting** | Zygisk Detection | Detects Zygisk / ZygiskNext / ReZygisk process injection and module loading |
| **Hook Detection** | Xposed Detection | Detects Xposed framework (both Riru and Zygisk modes) + LSPatch |
| **Hook Detection** | LSPosed Detection | Detects LSPosed module manager and its scopes |
| **Hook Detection** | Frida Detection | Detects Frida server / gadget / agent through multiple paths |
| **Hook Detection** | Substrate/Cydia Detection | Detects Substrate and Cydia frameworks |
| **Hook Detection** | ART inline Hook | SandHook / Pine / Epic / Whale / ShadowHook / ByteHook |
| **Custom ROM Detection** | ROM Fingerprinting | Identifies over 50 known custom ROMs (ArrowOS / Rising / Elixir / PixelOS / StatiX etc.) |
| **Custom ROM Detection** | Vendor Modification Detection | Detects vendor-specific system modifications and customisations |
| **Virtual Framework** | VirtualXposed | Detects io.va.exposed family (root-free Xposed injection) |
| **Virtual Framework** | TaiChi | Detects me.weishu.exp family (TaiChi yang / yin) |
| **Virtual Framework** | Dual Space / Clone | Parallel Space / Dual Master / WeChat Clone / 360 Clone |
| **Virtual Framework** | App Sandbox | Island / Shelter / Insular / App Cloner |
| **Dangerous Apps** | Memory Editors | GameGuardian / CheatEngine / GameKiller / SB Game Hacker / GameCIH |
| **Dangerous Apps** | IAP Crackers | Lucky Patcher / Freedom / CreeHack / LocalIAPStore |
| **Dangerous Apps** | APK Crackers | AntiLVL / Bamen Shenqi / Dashi Zhushou |
| **Magisk Extensions** | DenyList | Detects Magisk DenyList configuration (replaces MagiskHide) |
| **Magisk Extensions** | Zygisk Modules | Zygisk / ZygiskNext / ReZygisk module paths and memory traces |
| **Magisk Extensions** | LSPosed Manager | Detects LSPosed Manager APP + LSPatch |
| **Magisk Extensions** | Riru Modules | Detects Riru core and edxp / sandhook modules |
| **Magisk Extensions** | Modern Forks | Magisk Delta / Kitsune / Kitana / Alpha / Beta |
| **SELinux Analysis** | Context Jump Detection | Detects anomalous SELinux context transitions and switches |
| **SELinux Analysis** | Domain Violation Auditing | Audits abnormal domain transitions and privilege escalation |
| **Anti-Hiding Probes** | Shamiko Detection | Detects Shamiko Zygisk hiding module (incl. new paths) |
| **Anti-Hiding Probes** | ZygiskNext Detection | Detects ZygiskNext / ReZygisk hiding module |
| **Anti-Hiding Probes** | Process Hiding Detection | Detects hidden entries in process listings |
| **Anti-Hiding Probes** | Namespace Hiding Detection | Detects mount namespace cloaking and isolation |
| **Anti-Hiding Probes** | HideMyApplist Detection | Detects app-list hiding module (incl. hma alias) |
| **Anti-Hiding Probes** | StorageIsolation Detection | Detects Rikka storage isolation module |
| **Anti-Hiding Probes** | MagiskHide (legacy) | Detects legacy MagiskHide / magiskimg leftovers |

<br>

---

<br>

## 检测维度详解
## Detection Dimensions in Detail

<br>

### 中文

本项目的检测体系建立在**十三层检测架构**之上，每一层针对 Android 安全栈的一个特定抽象层次。这种分层方法并非随意为之，而是基于对现代 root 隐藏技术的深入理解——每一种隐藏技术都必须作用于栈的某一层或某几层，而我们的策略是在每一层都进行独立的、交叉验证的检测。

#### 第一层 · 系统属性完整性

系统属性（System Properties）是 Android 系统中最基础的信息载体。任何 root 方案在隐藏自身时，都必须修改或屏蔽某些关键系统属性——否则最基础的 `ro.debuggable` 和 `ro.secure` 检测就会暴露它们的存在。

本层的检测内容包括：
- 关键安全属性（ro.debuggable、ro.secure、ro.build.tags 等）的值是否与预期一致
- 是否存在被篡改但试图伪装的属性值
- 属性获取路径是否被挂钩或拦截
- 隐藏属性目录是否被用于屏蔽检测

这一层的检测速度极快（通常在毫秒级），是每次评估的第一步。

#### 第二层 · 运行时注入检测

ART（Android Runtime）是 Android 应用的执行引擎。当 Xposed、LSPosed 等框架需要挂钩应用方法时，它们必须在 ART 层面进行操作。这些操作会留下可检测的痕迹——无论是方法入口点的修改、JNI 函数表的替换，还是 ClassLoader 链路的异常。

本层的检测内容包括：
- ART 运行时环境的关键结构完整性
- JNI 函数表是否被挂钩
- ClassLoader 链路是否存在异常条目
- 系统 Framework 的关键方法是否被替换

#### 第三层 · 内存指纹取证

每一个运行中的进程都在内存中留下独特的指纹。Magisk 的守护进程、Zygisk 的注入模块、Frida 的 agent——它们都有可识别内存特征。本层通过对进程地址空间的深度扫描，寻找这些已知特征。

本层的检测内容包括：
- 扫描进程的可执行内存段，寻找已知工具的代码签名
- 分析内存映射文件（maps），检测隐藏的匿名映射
- 扫描共享库加载列表，检测被伪装或隐藏的库
- 检测进程间共享内存中的异常区域

这一层在"标准"模式下扫描主要进程，在"深度"模式下扫描所有活跃进程。

#### 第四层 · 文件系统与挂载分析

Root 隐藏方案经常通过操作挂载命名空间（Mount Namespace）来隐藏文件系统的真实状态。Magisk 的 MagiskHide、KernelSU 的安全挂载模式、Shamiko 的命名空间隔离——它们都试图让检测工具看到一个"干净"的文件系统。

本层的检测内容包括：
- 挂载命名空间是否与初始命名空间不同
- overlayfs 层是否覆盖了关键系统路径
- 已知的 root 工具路径是否存在（即使被隐藏，通过直接系统调用绕过）
- /proc、/sys 等关键文件系统是否有异常挂载
- tmpfs 挂载中是否包含工具二进制文件

#### 第五层 · 侧信道时序分析

这是本项目最具技术深度的检测维度之一。侧信道攻击利用系统的物理实现特性来泄露信息——我们反过来利用这些特性来检测异常。当 APatch、KernelSU 等方案拦截系统调用时，即使是纳秒级的延迟变化也可以被检测到。

本层的检测内容包括：
- truncate() 系统调用的时序精确测量（对 APatch 特别有效）
- binder 事务的延迟分析
- CPU 缓存命中/未命中时序差异
- 中断响应时间的统计异常检测
- 系统调用返回值的时序稳定性分析

这一层使用高精度计时器和统计分析方法，能够在其他检测维度都失败时提供决定性的证据。

#### 第六层 · Root 守护进程检测（原内核完整性，已迁移到 Ring3）

> **重要变更**：原"内核完整性"层依赖 `/proc/kallsyms`、`/proc/modules`、`/proc/sys/kernel/tainted` 等 Ring0（内核态）信息源。这些检测在现代 GKI 内核上受 `kptr_restrict` 限制，且需要 `CAP_SYS_MODULE` 等特权，与本项目"仅 root 级检测"的定位不符。已全部移除。

现改为：**Root 守护进程 / service 脚本 / 配置 DB** 三类 root 级检测。这是真正稳定、跨内核版本通用的检测手段。

本层的检测内容包括：
- Magisk / KernelSU / APatch 守护进程（magiskd / ksud / apd / kitana / sukid 等）在 `/proc/*/cmdline` 中的痕迹
- Root 方案的 service / post-fs-data / boot-completed 脚本目录
- Root 方案的配置 DB（`/data/adb/magisk.db`、`/data/adb/ksu/db`、`/data/adb/ap/db` 等）
- Root 方案的二进制 / 工具链 / 日志文件

#### 第七层 · 启动链完整性

设备的信任根始于 SoC 中的 BootROM，经过 Bootloader、Boot 映像、系统映像，最终到达用户空间。这一链条中的任何一个环节被破坏，都会让整个信任体系崩塌。

本层的检测内容包括：
- Bootloader 锁定状态检测
- Android Verified Boot (AVB) 状态验证
- dm-verity 设备映射器的完整性检查
- vbmeta 分区的签名验证
- 启动映像（boot.img）的完整性校验
- 系统分区的只读挂载状态

#### 第八至十层 · Root 方案指纹识别

这三层专门针对当前主流的三种 root 方案：Magisk、KernelSU 和 APatch。每种方案都有其独特的架构和可检测特征。

**Magisk 检测**：MagiskInit、MagiskSU、MagiskPolicy、Zygisk、MagiskHide、模块系统——每个组件的存在与否都能提供有价值的信息。

**KernelSU 检测**：内核模块方法、安全挂载模式、WebUI 管理器、非对称的隐藏能力。

**APatch 检测**：KPM 内核补丁模块、APD 守护进程、侧信道时序特征、特殊的文件系统交互模式。

#### 第十一层 · Hook 框架检测

Hook 框架能够在不修改磁盘文件的情况下改变应用和系统行为。Xposed、LSPosed、Frida、Substrate——它们有不同的工作机制，但都会在运行时留下可检测的痕迹。

#### 第十二层 · 自定义 ROM 检测

超过 30 种已知的自定义 ROM 具有独特的系统签名。这一层通过多种特征（build.prop 属性、特有的系统应用、独特的 SELinux 策略、特有的服务等）来识别 ROM 类型。

#### 第十三层 · 固件完整性

这一层检测固件级别的篡改，包括 TEE（Trusted Execution Environment）的完整性和 Modem 固件的状态。

#### 第十四层 · 虚拟框架 / 双开分身检测

VirtualXposed、太极、平行空间等虚拟化框架可在不 root 的情况下注入 Xposed 模块到目标进程，绕过传统 root 检测。本层覆盖 io.va.exposed 系列、me.weishu.exp 系列、平行空间 / 分身大师 / 双开大师 / 360 分身 / 微信分身、Island / Shelter / Insular / App Cloner 等典型包名与进程痕迹。

#### 第十五层 · 危险应用检测

检测常见的游戏作弊 / 系统篡改工具，包括 GameGuardian、CheatEngine、Lucky Patcher、GameKiller、SB Game Hacker、GameCIH、Freedom、CreeHack、LocalIAPStore、八门神器等。这些应用通常需要 root 或本身就是 root 滥用工具。

#### 第十六层 · Magisk 扩展生态检测

Magisk 生态衍生品的覆盖检测，包括：
- **DenyList**：替代 MagiskHide 的应用隐藏配置
- **Zygisk 模块**：Zygisk / ZygiskNext / ReZygisk 的路径与内存痕迹
- **LSPosed Manager**：LSPosed Manager APP 与 LSPatch（免 root patch）
- **Riru 模块**：Riru 核心 + edxp / sandhook 等历史模块
- **现代 fork**：Magisk Delta / Kitsune / Kitana / Alpha / Beta

### English

The detection architecture of this project is built upon **sixteen detection layers**, each targeting a specific abstraction level of the Android security stack. This layered approach is not arbitrary — it is grounded in a deep understanding of modern root hiding techniques. Every hiding technique must operate on one or more layers of the stack, and our strategy is to conduct independent, cross-validating detection at every layer.

#### Layer 1 · System Property Integrity

System properties are the most fundamental information carriers in the Android system. Any root solution attempting to hide itself must modify or shield certain critical system properties — otherwise even the most basic `ro.debuggable` and `ro.secure` checks would expose its presence.

Detection at this layer includes:
- Verification of critical security properties (ro.debuggable, ro.secure, ro.build.tags, etc.)
- Detection of property values that appear tampered yet attempt to masquerade as valid
- Detection of property retrieval path hooking or interception
- Detection of hidden property directories used to shield detection

This layer operates with extreme speed (typically milliseconds) and serves as the first step in every assessment.

#### Layer 2 · Runtime Injection Detection

ART (Android Runtime) is the execution engine for Android applications. When frameworks like Xposed or LSPosed need to hook application methods, they must operate at the ART level. These operations leave detectable traces — whether through method entry point modifications, JNI function table replacements, or anomalies in the class loader chain.

Detection at this layer includes:
- Integrity verification of critical ART runtime structures
- Detection of JNI function table hooks
- Detection of anomalous entries in the class loader chain
- Verification of critical system Framework methods for replacement

#### Layer 3 · Memory Fingerprint Forensics

Every running process leaves unique fingerprints in memory. Magisk's daemon processes, Zygisk's injection modules, Frida's agents — all have identifiable memory signatures. This layer conducts deep scans of process address spaces to locate these known characteristics.

Detection at this layer includes:
- Scanning executable memory segments for known tool code signatures
- Analysis of memory map files for hidden anonymous mappings
- Scanning shared library load lists for disguised or hidden libraries
- Detection of anomalous regions in inter-process shared memory

This layer scans primary processes in "Standard" mode and all active processes in "Deep" mode.

#### Layer 4 · Filesystem & Mount Analysis

Root hiding solutions frequently manipulate mount namespaces to conceal the true state of the filesystem. Magisk's MagiskHide, KernelSU's safe mount mode, Shamiko's namespace isolation — all attempt to present a "clean" filesystem to detection tools.

Detection at this layer includes:
- Verification of mount namespace divergence from the initial namespace
- Detection of overlayfs layers covering critical system paths
- Direct syscall-based probing for known root tool paths (bypassing hooks)
- Detection of anomalous mounts on /proc, /sys, and other critical filesystems
- Inspection of tmpfs mounts for tool binaries

#### Layer 5 · Side-Channel Timing Analysis

This represents one of the most technically profound detection dimensions in this project. Side-channel attacks exploit physical implementation characteristics to leak information — we reverse this paradigm to detect anomalies. When APatch, KernelSU, and similar solutions intercept system calls, even nanosecond-scale latency variations become detectable.

Detection at this layer includes:
- Precision timing measurement of truncate() syscalls (particularly effective against APatch)
- Binder transaction latency analysis
- CPU cache hit/miss timing differentials
- Statistical anomaly detection in interrupt response times
- Temporal stability analysis of syscall return values

This layer employs high-precision timers and statistical analysis methods to provide decisive evidence when all other detection dimensions fail.

#### Layer 6 · Root Daemon Detection (formerly Kernel Integrity, migrated to Ring3)

> **Important change**: The former "Kernel Integrity" layer relied on Ring0 (kernel-space) sources such as `/proc/kallsyms`, `/proc/modules`, and `/proc/sys/kernel/tainted`. These sources are restricted by `kptr_restrict` on modern GKI kernels and require privileges such as `CAP_SYS_MODULE`, conflicting with this project's "root-level only" positioning. All such checks have been removed.

The layer now performs three categories of root-level detection: **root daemons**, **service scripts**, and **config DBs**. These are stable, cross-kernel-version detection methods.

Detection at this layer includes:
- Magisk / KernelSU / APatch daemon processes (magiskd / ksud / apd / kitana / sukid) in `/proc/*/cmdline`
- Root-solution service / post-fs-data / boot-completed script directories
- Root-solution config DBs (`/data/adb/magisk.db`, `/data/adb/ksu/db`, `/data/adb/ap/db`, etc.)
- Root-solution binaries, toolchains and log files

#### Layer 7 · Boot Chain Integrity

The device's root of trust begins in the SoC's BootROM, traverses the Bootloader, Boot Image, System Image, and finally reaches userspace. A compromise at any link in this chain collapses the entire trust framework.

Detection at this layer includes:
- Bootloader lock status detection
- Android Verified Boot (AVB) status verification
- dm-verity device mapper integrity checking
- vbmeta partition signature verification
- Boot image (boot.img) integrity validation
- System partition read-only mount status verification

#### Layers 8-10 · Root Solution Fingerprinting

These three layers target the three mainstream root solutions: Magisk, KernelSU, and APatch. Each solution possesses a unique architecture and detectable characteristics.

**Magisk Detection**: MagiskInit, MagiskSU, MagiskPolicy, Zygisk, MagiskHide, module system — the presence or absence of each component provides valuable information.

**KernelSU Detection**: Kernel module approach, safe mount mode, WebUI manager, asymmetric hide capabilities.

**APatch Detection**: KPM kernel patch modules, APD daemon, side-channel timing signatures, distinctive filesystem interaction patterns.

#### Layer 11 · Hook Framework Detection

Hook frameworks can alter application and system behaviour without modifying disk files. Xposed, LSPosed, Frida, Substrate — each employs different mechanisms, yet all leave detectable traces at runtime.

#### Layer 12 · Custom ROM Detection

Over 30 known custom ROMs possess unique system signatures. This layer identifies ROM types through multiple characteristics (build.prop properties, distinctive system applications, unique SELinux policies, unique services, etc.).

#### Layer 13 · Firmware Integrity

This layer detects firmware-level tampering, including TEE (Trusted Execution Environment) integrity and modem firmware status.

#### Layer 14 · Virtual Framework / Dual Space Detection

VirtualXposed, TaiChi, Parallel Space and similar virtualisation frameworks can inject Xposed modules into target processes without root, bypassing traditional root detection. This layer covers io.va.exposed series, me.weishu.exp series, Parallel Space / Dual Master / 360 Clone / WeChat Clone, and Island / Shelter / Insular / App Cloner.

#### Layer 15 · Dangerous Apps Detection

Detects common game cheating / system tampering tools including GameGuardian, CheatEngine, Lucky Patcher, GameKiller, SB Game Hacker, GameCIH, Freedom, CreeHack, LocalIAPStore, Bamen Shenqi, etc. These apps typically require root or are themselves root-abuse tools.

#### Layer 16 · Magisk Extension Ecosystem Detection

Coverage for Magisk ecosystem derivatives:
- **DenyList**: app hiding configuration replacing MagiskHide
- **Zygisk Modules**: paths and memory traces of Zygisk / ZygiskNext / ReZygisk
- **LSPosed Manager**: LSPosed Manager APP + LSPatch (root-free patch)
- **Riru Modules**: Riru core + edxp / sandhook historical modules
- **Modern Forks**: Magisk Delta / Kitsune / Kitana / Alpha / Beta

<br>

---

<br>

## 评估模式
## Assessment Modes

<br>

### 中文

**环境检测** 提供四种评估模式，覆盖从快速检查到法医级分析的完整需求谱系。

#### 🚀 快速模式 · Quick Mode

**持续时间：** < 500 毫秒
**目标：** 即时分诊——设备是否明显存在问题？
**覆盖：** 第一层（属性）、第三层（内存，仅主进程）、第八至十层（已知 root 方案的基本特征）

快速模式适用于日常检查。当你只是想确认设备是否"看起来正常"时，这个模式能在眨眼之间给出答案。它不会进行深度分析，但如果它发现问题，那么问题通常是明确的。

#### ⚡ 标准模式 · Standard Mode

**持续时间：** < 3 秒
**目标：** 全面的常规评估
**覆盖：** 第一至六层、第八至十二层（完整的检测维度，但以标准精度运行）

标准模式是推荐的日常使用模式。它在速度和深度之间取得了最佳平衡，能够在几秒钟内提供设备安全态势的全面快照。对于大多数用户来说，这是最合适的选择。

#### 🔬 深度模式 · Deep Mode

**持续时间：** < 10 秒
**目标：** 完整的法医分析，覆盖所有层次
**覆盖：** 所有十三层，以最高精度运行，包括侧信道统计分析和内存深度取证

深度模式适用于当你需要绝对确定的时候。它会对所有进程进行内存扫描，执行多次侧信道测量以获得统计显著性，并验证每一个可验证的检测点。这种模式产生的信息量最大，但也会消耗更多资源和时间。

#### 🧪 法医模式 · Forensic Mode

**持续时间：** 可配置（通常 30-120 秒）
**目标：** 详尽的离线分析数据收集
**覆盖：** 所有层，多次迭代，包括基线比较和趋势分析

法医模式专为安全研究人员和高级用户设计。它不仅进行检测，还会收集详尽的原始数据用于离线分析。这些数据可以导出、比较、并用于识别随时间变化的趋势。

### English

**Environment Detection** offers four assessment modes, covering the complete spectrum from quick checks to forensic-grade analysis.

#### 🚀 Quick Mode

**Duration:** < 500 ms
**Objective:** Immediate triage — is the device obviously compromised?
**Coverage:** Layer 1 (Properties), Layer 3 (Memory, main process only), Layers 8-10 (basic root solution characteristics)

Quick mode is suitable for daily checks. When you just want to confirm that a device "looks normal", this mode delivers answers in the blink of an eye. It does not perform deep analysis, but if it flags an issue, that issue is typically unambiguous.

#### ⚡ Standard Mode

**Duration:** < 3 seconds
**Objective:** Comprehensive routine assessment
**Coverage:** Layers 1-6, 8-12 (complete detection dimensions at standard precision)

Standard mode is the recommended daily-use mode. It achieves an optimal balance between speed and depth, providing a comprehensive snapshot of device security posture in seconds. For most users, this is the most appropriate choice.

#### 🔬 Deep Mode

**Duration:** < 10 seconds
**Objective:** Complete forensic analysis across all layers
**Coverage:** All 13 layers, operating at maximum precision, including side-channel statistical analysis and deep memory forensics

Deep mode is for when you need absolute certainty. It scans all processes for memory signatures, performs multiple side-channel measurements for statistical significance, and verifies every verifiable detection point. This mode generates the most information while consuming more resources and time.

#### 🧪 Forensic Mode

**Duration:** Configurable (typically 30-120 seconds)
**Objective:** Exhaustive offline analysis data collection
**Coverage:** All layers, multiple iterations, including baseline comparison and trend analysis

Forensic mode is designed for security researchers and advanced users. It not only performs detection but collects exhaustive raw data for offline analysis. This data can be exported, compared, and used to identify trends over time.

<br>

---

<br>

## 风险分级模型
## Risk Classification Model

<br>

### 中文

每一个检测发现都被映射到一个五级风险模型之中。这个模型不是简单的加权汇总——它考虑了检测维度之间的交互和验证关系。

```
🟢 安全 · CLEAR
    └── 未检测到任何异常指标
    └── 设备处于预期的安全状态
    └── 无需采取任何措施

🟡 低风险 · LOW
    └── 检测到轻微异常
    └── 通常由良性修改引起（如开发者选项）
    └── 建议关注，但无需立即行动

🟠 中风险 · MEDIUM
    └── 检测到可疑指标
    └── 可能表示系统修改，也可能是误报
    └── 建议使用深度模式重新评估
    └── 需要用户判断

🔴 高风险 · HIGH
    └── 检测到系统修改的有力证据
    └── 多个检测维度交叉验证确认
    └── 设备安全状态受到显著影响
    └── 建议采取相应行动

⚫ 严重 · CRITICAL
    └── 检测到主动隐藏或恶意入侵
    └── 系统完整性受到根本性破坏
    └── 隐藏行为本身即是最危险的信号
    └── 强烈建议立即处理
```

风险评分算法综合考虑以下因素：
- **检测数量**：有多少个独立的检测点触发了告警
- **检测层分布**：告警是否覆盖多个安全层次（跨层告警比单层告警更严重）
- **检测强度**：每个告警的确信度（可能的、可能的、确定的）
- **隐藏证据**：是否存在主动隐藏的迹象（这是最重要的加重因素）
- **交叉验证**：多个独立的检测路径是否得出一致的结论

### English

Every detection finding is mapped to a five-tier risk model. This model is not a simple weighted sum — it considers the interactions and cross-validation relationships between detection dimensions.

```
🟢 CLEAR
    └── No indicators of compromise detected
    └── Device is in its expected security state
    └── No action required

🟡 LOW
    └── Minor anomalies detected
    └── Typically caused by benign modifications (e.g. developer options)
    └── Worth monitoring, but no immediate action required

🟠 MEDIUM
    └── Suspicious indicators detected
    └── May indicate system modification or may be a false positive
    └── Recommend re-evaluation using Deep Mode
    └── Requires user judgement

🔴 HIGH
    └── Strong evidence of system modification
    └── Cross-validated across multiple detection dimensions
    └── Device security posture is significantly affected
    └── Appropriate action recommended

⚫ CRITICAL
    └── Active concealment or malicious compromise detected
    └── System integrity is fundamentally undermined
    └── The hiding behaviour itself is the most dangerous signal
    └── Immediate action strongly advised
```

The risk scoring algorithm comprehensively considers:
- **Detection Count**: How many independent detection points triggered alerts
- **Layer Distribution**: Whether alerts span multiple security layers (cross-layer alerts are more severe than single-layer alerts)
- **Detection Confidence**: The certainty level of each alert (possible, probable, confirmed)
- **Concealment Evidence**: Whether there are signs of active hiding (this is the most significant aggravating factor)
- **Cross-Validation**: Whether multiple independent detection paths yield consistent conclusions

<br>

---

<br>

## 评分算法哲学
## Scoring Algorithm Philosophy

<br>

### 中文

**环境检测** 的评分算法不是简单的"找到了 X 个指标，得分 Y"。我们的评分哲学基于以下几个核心原则：

**原则一：隐藏比修改更严重**

如果一个设备只是被 root，但没有使用任何隐藏方案，这反而是一种"诚实"的状态。虽然 root 带来的安全风险客观存在，但至少用户和检测工具都能清楚地知道设备的真实状态。

相比之下，一个使用隐藏方案（MagiskHide、Zygisk 的排除列表、Shamiko、ZygiskNext、APatch 的隐藏模式等）的设备，其风险评分会显著更高。原因很简单：**隐藏本身就是一个安全决策**——它表明有人在刻意掩盖某些行为，而这些行为可能超出了简单的 root 范畴。

**原则二：跨层告警比单层告警更严重**

一个只在"系统属性"层触发告警的检测结果，与一个同时在"系统属性"、"内存指纹"、"内核完整性"、"侧信道时序"四个层触发告警的检测结果，有着本质的区别。后者表明系统修改的广度和深度都远超前者。

我们的评分算法会对覆盖更多层次的告警给予指数级更高的权重。

**原则三：确定性优先于数量**

一百个"可能"级别的告警，也不如一个"确定"级别的告警有价值。我们优先确保高确定性检测点得到适当的权重，低确定性检测点则被用于补充和交叉验证，而不是直接驱动评分。

**原则四：证据链优于孤立指标**

评分算法会在检测点之间寻找关联。如果一个"可疑的系统属性"与"已知的 Magisk 内存指纹"同时出现在同一个进程中，这比两个无关的告警要严重得多。检测点之间的关联性大大增强了结论的可信度。

### English

The scoring algorithm of **Environment Detection** is not a simple "found X indicators, score Y". Our scoring philosophy is based on the following core principles:

**Principle One: Concealment Is More Serious Than Modification**

If a device is simply rooted without employing any hiding solution, this is, paradoxically, an "honest" state. While the security risks of root access objectively exist, at least the user and the detection tool can both clearly perceive the device's true state.

By contrast, a device employing hiding solutions (MagiskHide, Zygisk denylist, Shamiko, ZygiskNext, APatch hide mode, etc.) receives a significantly elevated risk score. The reason is straightforward: **the act of hiding itself is a security decision** — it signals that someone is deliberately concealing certain activities, which may extend beyond simple root access.

**Principle Two: Cross-Layer Alerts Are More Severe Than Single-Layer Alerts**

There is a fundamental difference between an alert triggered solely at the "System Properties" layer and one triggered simultaneously across "System Properties", "Memory Fingerprints", "Kernel Integrity", and "Side-Channel Timing" layers. The latter indicates a breadth and depth of system modification far exceeding the former.

Our scoring algorithm assigns exponentially greater weight to alerts spanning more layers.

**Principle Three: Certainty Over Quantity**

One hundred "possible"-level alerts are worth less than a single "confirmed"-level alert. We prioritise ensuring that high-certainty detection points receive appropriate weight, while low-certainty detection points serve supplementary and cross-validation roles rather than directly driving scores.

**Principle Four: Chain of Evidence Over Isolated Indicators**

The scoring algorithm searches for correlations between detection points. A "suspicious system property" appearing alongside a "known Magisk memory fingerprint" within the same process is significantly more serious than two unrelated alerts. The correlation between detection points greatly enhances the credibility of the conclusion.

<br>

---

<br>

## 报告系统
## Report System

<br>

### 中文

每次检测完成后，**环境检测** 都会生成一份详尽的检测报告。报告不是一个简单的分数或状态标签，而是一份完整的、结构化的、可操作的安全评估文档。

#### 报告结构

一份完整的检测报告包含以下部分：

1. **执行摘要** —— 用简洁的语言描述检测的整体结论，适合快速阅读
2. **风险总览** —— 风险等级、总得分、关键发现的数量和分布
3. **层次分析** —— 每一层的详细检测结果，包括检测项、状态、证据和确信度
4. **关键发现** —— 最重要的告警的深入分析，包括技术解释和潜在影响
5. **证据详情** —— 每个告警背后的原始数据和技术细节（适用于高级用户）
6. **行动建议** —— 基于检测结果的个性化改进建议
7. **时间线** —— 检测的时间戳和耗时统计
8. **设备快照** —— 检测时的设备基本信息（型号、Android 版本、内核版本等）

#### 报告格式

报告可以以多种格式导出：
- **JSON** —— 机器可读的结构化数据，适用于自动化处理
- **PDF** —— 格式化的可打印文档
- **HTML** —— 交互式网页报告，带有搜索和过滤功能
- **纯文本摘要** —— 简洁的文本摘要，适用于快速分享

#### 报告签名

深度检测和法医检测生成的报告可以附带加密签名，确保报告在生成后的完整性和真实性。这一特性对于需要在组织内部分享或存档的检测报告尤为重要。

### English

After each assessment, **Environment Detection** generates a comprehensive detection report. The report is not a simple score or status label — it is a complete, structured, actionable security assessment document.

#### Report Structure

A complete detection report comprises the following sections:

1. **Executive Summary** — A concise description of the overall assessment conclusion, suitable for rapid reading
2. **Risk Overview** — Risk level, total score, quantity and distribution of key findings
3. **Layer Analysis** — Detailed detection results for each layer, including detection items, status, evidence, and confidence levels
4. **Key Findings** — In-depth analysis of the most important alerts, including technical explanations and potential impact
5. **Evidence Details** — Raw data and technical details behind each alert (for advanced users)
6. **Action Recommendations** — Personalised improvement suggestions based on detection results
7. **Timeline** — Detection timestamps and timing statistics
8. **Device Snapshot** — Basic device information at the time of detection (model, Android version, kernel version, etc.)

#### Report Formats

Reports can be exported in multiple formats:
- **JSON** — Machine-readable structured data, suitable for automated processing
- **PDF** — Formatted printable document
- **HTML** — Interactive web-based report with search and filter capabilities
- **Plain Text Summary** — Concise text summary, suitable for rapid sharing

#### Report Signing

Reports generated by Deep and Forensic mode assessments can include cryptographic signatures, ensuring the integrity and authenticity of the report after generation. This capability is particularly important for reports intended for sharing within organisations or for archival purposes.

<br>

---

<br>

## 用户界面理念
## UI Philosophy

<br>

### 中文

**环境检测** 的用户界面基于"液体玻璃"（Liquid Glass）设计语言构建。这不是一个随意的设计选择——它反映了我们对工具的本质理解：一件工具应该既强大又透明，既复杂又清晰。

#### 设计核心原则

**清晰优先于花哨** —— 每一个像素的存在都有其功能意义。动画和视觉效果服务于信息传达，而非装饰。

**渐进式信息揭示** —— 用户首先看到的是最关键的结论。如果需要深入了解，可以逐层展开，查看越来越多的细节。新手看到的是一个清晰的"安全/警告/危险"标签；专家看到的是完整的检测数据。

**黑暗与光明** —— 完整的深色模式和浅色模式支持，在不同光照环境下都能提供舒适的阅读体验。

**可访问性** —— 支持字体缩放、高对比度模式和屏幕阅读器友好标记。

#### 主屏幕 · Dashboard

主屏幕以卡片式布局展示检测概览。中心的"扫描"按钮使用液态动画，在检测过程中流动变形，提供即时的视觉反馈。

检测完成后，主屏幕会显示：
- 风险等级（带有醒目的颜色标识）
- 各层检测状态的概览环形图
- 关键发现的简要列表
- 一键执行更深度扫描的入口

#### 报告屏幕 · Report Screen

报告屏幕以树形结构展示完整的检测结果。用户可以：
- 展开/折叠每一层的详细信息
- 点击任何发现查看技术细节和证据
- 使用搜索功能快速定位特定检测项
- 一键导出报告为多种格式

#### 设置屏幕 · Settings Screen

设置屏幕提供对检测行为的精细控制：
- 默认检测模式
- 检测维度的启用/禁用
- 报告生成偏好
- 通知设置
- 数据导出配置
- 主题选择

### English

The user interface of **Environment Detection** is built upon the "Liquid Glass" design language. This is not an arbitrary design choice — it reflects our understanding of the essence of a tool: an instrument should be both powerful and transparent, both complex and clear.

#### Core Design Principles

**Clarity Over Ornamentation** — Every pixel exists with functional purpose. Animations and visual effects serve information delivery, not decoration.

**Progressive Information Disclosure** — Users first see the most critical conclusions. If deeper understanding is needed, they can expand layers progressively to view increasing detail. A newcomer sees a clear "Safe/Warning/Danger" label; an expert sees complete detection data.

**Dark and Light** — Full dark mode and light mode support, providing comfortable reading experiences across different lighting environments.

**Accessibility** — Font scaling, high contrast mode, and screen reader-friendly markup are supported.

#### Dashboard

The main screen presents a detection overview in a card-based layout. The central "Scan" button uses liquid animation, flowing and transforming during the assessment process, providing immediate visual feedback.

Upon assessment completion, the dashboard displays:
- Risk level (with prominent colour coding)
- Overview donut chart of each layer's detection status
- Brief list of key findings
- One-tap entry point for deeper scans

#### Report Screen

The report screen presents complete detection results in a tree structure. Users can:
- Expand/collapse detailed information for each layer
- Tap any finding to view technical details and evidence
- Use the search function to quickly locate specific detection items
- Export reports in multiple formats with one tap

#### Settings Screen

The settings screen provides fine-grained control over assessment behaviour:
- Default assessment mode
- Enable/disable specific detection dimensions
- Report generation preferences
- Notification settings
- Data export configuration
- Theme selection

<br>

---

<br>

## 兼容性矩阵
## Compatibility Matrix

<br>

### 中文

**环境检测** 的设计目标是覆盖尽可能广泛的 Android 设备生态系统。以下矩阵显示了在不同环境中的兼容性状态。

| 环境 | 支持程度 | 备注 |
|---|---|---|
| **AOSP / 原生 Android** | ✅ 完全支持 | 基线参考实现 |
| **Google Pixel (Tensor)** | ✅ 完全支持 | 在 Pixel 6/7/8/9 系列上验证 |
| **Samsung One UI** | ✅ 完全支持 | 额外 Knox 感知能力 |
| **Xiaomi HyperOS / MIUI** | ✅ 完全支持 | 额外厂商属性感知 |
| **OPPO ColorOS** | ✅ 完全支持 | 额外兼容性适配 |
| **vivo OriginOS** | ✅ 完全支持 | 额外兼容性适配 |
| **OnePlus OxygenOS** | ✅ 完全支持 | 在多个版本上验证 |
| **Huawei HarmonyOS / EMUI** | ⚠️ 部分支持 | 某些检测维度受限 |
| **Honor MagicOS** | ⚠️ 部分支持 | 某些检测维度受限 |
| **LineageOS / 自定义 ROM** | ✅ 完全支持 | ROM 指纹识别已启用 |
| **crDroid** | ✅ 完全支持 | ROM 指纹识别已启用 |
| **PixelExperience** | ✅ 完全支持 | ROM 指纹识别已启用 |
| **Paranoid Android** | ✅ 完全支持 | ROM 指纹识别已启用 |
| **GSI / Project Treble** | ⚠️ 部分支持 | 某些厂商 HAL 路径可能不可用 |
| **Android 模拟器** | ⚠️ 有限支持 | 模拟环境中无法进行内核级探测 |
| **WSA (Windows 子系统 for Android)** | ❌ 不支持 | 不是有效的安全评估目标 |

#### Android 版本兼容性

| Android 版本 | API 级别 | 支持状态 |
|---|---|---|
| Android 10 | API 29 | ✅ 完全支持 |
| Android 11 | API 30 | ✅ 完全支持 |
| Android 12 | API 31-32 | ✅ 完全支持 |
| Android 13 | API 33 | ✅ 完全支持 |
| Android 14 | API 34 | ✅ 完全支持（目标版本） |
| Android 15 | API 35 | ✅ 完全支持 |

#### 内核版本兼容性

| 内核版本 | 支持状态 | 备注 |
|---|---|---|
| Linux 4.9 | ✅ 支持 | 基本功能完整 |
| Linux 4.14 | ✅ 支持 | 基本功能完整 |
| Linux 4.19 | ✅ 完全支持 | 推荐的最低版本 |
| Linux 5.4 | ✅ 完全支持 | 所有功能可用 |
| Linux 5.10 | ✅ 完全支持 | 所有功能可用 |
| Linux 5.15 | ✅ 完全支持 | 所有功能可用 |
| Linux 6.1 | ✅ 完全支持 | 所有功能可用 |
| Linux 6.6+ | ✅ 完全支持 | 所有功能可用 |

### English

**Environment Detection** is designed for maximum coverage across the Android device ecosystem. The following matrix shows compatibility status across different environments.

| Environment | Support | Notes |
|---|---|---|
| **AOSP / Stock Android** | ✅ Full | Baseline reference implementation |
| **Google Pixel (Tensor)** | ✅ Full | Verified on Pixel 6/7/8/9 series |
| **Samsung One UI** | ✅ Full | Additional Knox awareness |
| **Xiaomi HyperOS / MIUI** | ✅ Full | Additional vendor property awareness |
| **OPPO ColorOS** | ✅ Full | Additional compatibility shims |
| **vivo OriginOS** | ✅ Full | Additional compatibility shims |
| **OnePlus OxygenOS** | ✅ Full | Verified across multiple versions |
| **Huawei HarmonyOS / EMUI** | ⚠️ Partial | Certain detection dimensions limited |
| **Honor MagicOS** | ⚠️ Partial | Certain detection dimensions limited |
| **LineageOS / Custom ROMs** | ✅ Full | ROM fingerprinting enabled |
| **crDroid** | ✅ Full | ROM fingerprinting enabled |
| **PixelExperience** | ✅ Full | ROM fingerprinting enabled |
| **Paranoid Android** | ✅ Full | ROM fingerprinting enabled |
| **GSI / Project Treble** | ⚠️ Partial | Some vendor HAL paths may be unavailable |
| **Android Emulator** | ⚠️ Limited | Kernel-level probing unavailable in emulated environments |
| **WSA (Windows Subsystem for Android)** | ❌ No | Not a valid security assessment target |

#### Android Version Compatibility

| Android Version | API Level | Support Status |
|---|---|---|
| Android 10 | API 29 | ✅ Full |
| Android 11 | API 30 | ✅ Full |
| Android 12 | API 31-32 | ✅ Full |
| Android 13 | API 33 | ✅ Full |
| Android 14 | API 34 | ✅ Full (Target) |
| Android 15 | API 35 | ✅ Full |

#### Kernel Version Compatibility

| Kernel Version | Support Status | Notes |
|---|---|---|
| Linux 4.9 | ✅ Supported | Full basic functionality |
| Linux 4.14 | ✅ Supported | Full basic functionality |
| Linux 4.19 | ✅ Full | Recommended minimum |
| Linux 5.4 | ✅ Full | All features available |
| Linux 5.10 | ✅ Full | All features available |
| Linux 5.15 | ✅ Full | All features available |
| Linux 6.1 | ✅ Full | All features available |
| Linux 6.6+ | ✅ Full | All features available |

<br>

---

<br>

## 性能基准
## Performance Benchmarks

<br>

### 中文

以下性能基准数据在 Pixel 7 Pro (Tensor G2, Android 14) 上测得。实际性能可能因设备型号、处理器、系统负载和 Android 版本而有所差异。

| 评估模式 | 平均耗时 | 内存峰值 | CPU 使用率 |
|---|---|---|---|
| 快速模式 | 350 ms | 42 MB | 15% |
| 标准模式 | 2.1 s | 78 MB | 28% |
| 深度模式 | 7.8 s | 156 MB | 45% |
| 法医模式 | 55 s | 284 MB | 60% |

#### 各层检测耗时分摊

| 检测层 | 平均耗时 | 说明 |
|---|---|---|
| 第一层 · 系统属性 | 15 ms | 极速，仅内存操作 |
| 第二层 · 运行时注入 | 45 ms | 主要是 JNI 调用 |
| 第三层 · 内存指纹 | 420 ms | 内存扫描，随进程数增加 |
| 第四层 · 文件系统 | 180 ms | 挂载点和路径检查 |
| 第五层 · 侧信道时序 | 850 ms | 多次采样，统计分析 |
| 第六层 · 内核完整性 | 200 ms | 系统调用和内核检查 |
| 第七层 · 启动链 | 100 ms | 固件接口查询 |
| 第八层 · Magisk | 50 ms | 特征匹配 |
| 第九层 · KernelSU | 40 ms | 特征匹配 |
| 第十层 · APatch | 60 ms | 含侧信道辅助 |
| 第十一层 · Hook | 80 ms | 多种框架检测 |
| 第十二层 · 自定义 ROM | 30 ms | 属性比对 |
| 第十三层 · 固件 | 150 ms | TEE 状态查询 |

#### 电池影响

在标准模式下进行一次完整检测的电池消耗约为 **0.3 mAh**，相当于 Pixel 7 Pro 电池总容量的约 **0.003%**。即使每天进行 10 次标准模式检测，对电池寿命的影响也可以忽略不计。

### English

The following performance benchmark data was measured on a Pixel 7 Pro (Tensor G2, Android 14). Actual performance may vary based on device model, processor, system load, and Android version.

| Assessment Mode | Average Duration | Peak Memory | CPU Usage |
|---|---|---|---|
| Quick Mode | 350 ms | 42 MB | 15% |
| Standard Mode | 2.1 s | 78 MB | 28% |
| Deep Mode | 7.8 s | 156 MB | 45% |
| Forensic Mode | 55 s | 284 MB | 60% |

#### Per-Layer Timing Breakdown

| Detection Layer | Average Time | Notes |
|---|---|---|
| Layer 1 · System Properties | 15 ms | Extremely fast, memory-only operations |
| Layer 2 · Runtime Injection | 45 ms | Primarily JNI calls |
| Layer 3 · Memory Fingerprints | 420 ms | Memory scanning, scales with process count |
| Layer 4 · Filesystem | 180 ms | Mount point and path checks |
| Layer 5 · Side-Channel Timing | 850 ms | Multiple samples, statistical analysis |
| Layer 6 · Kernel Integrity | 200 ms | Syscall and kernel checks |
| Layer 7 · Boot Chain | 100 ms | Firmware interface queries |
| Layer 8 · Magisk | 50 ms | Signature matching |
| Layer 9 · KernelSU | 40 ms | Signature matching |
| Layer 10 · APatch | 60 ms | Includes side-channel assistance |
| Layer 11 · Hook | 80 ms | Multi-framework detection |
| Layer 12 · Custom ROM | 30 ms | Property comparison |
| Layer 13 · Firmware | 150 ms | TEE status queries |

#### Battery Impact

A complete assessment in Standard Mode consumes approximately **0.3 mAh** of battery, representing roughly **0.003%** of a Pixel 7 Pro's total battery capacity. Even performing 10 Standard Mode assessments daily results in negligible impact on battery life.

<br>

---

<br>

## 生态与愿景
## Ecosystem & Vision

<br>

### 中文

**环境检测** 不仅仅是一个孤立的工具。它是一个更大的愿景的一部分——一个让 Android 用户能够真正了解自己设备安全状态的生态系统。

#### 短期愿景 (已完成)

- 建立完整的十三层检测体系
- 实现四种评估模式
- 构建精致的 Liquid Glass 用户界面
- 支持主流的 root 方案和 hook 框架检测
- 实现详细的报告生成和导出功能

#### 中期愿景 (进行中)

- 检测规则的云端同步和实时更新
- 社区驱动的检测规则贡献平台
- 跨设备的安全状态比较和趋势分析
- 企业级批量检测和管理功能
- 更广泛的国际化支持

#### 长期愿景

- 建立一个开放的 Android 安全检测标准
- 成为 Android 安全评估的事实参考工具
- 推动 Android 生态系统向更高的透明度发展
- 培育一个活跃的安全研究社区

### English

**Environment Detection** is not merely an isolated tool. It is part of a larger vision — an ecosystem that empowers Android users to truly understand the security state of their devices.

#### Short-Term Vision (Achieved)

- Establishment of a complete thirteen-layer detection system
- Implementation of four assessment modes
- Construction of an exquisite Liquid Glass user interface
- Support for mainstream root solutions and hook framework detection
- Implementation of detailed report generation and export functionality

#### Mid-Term Vision (In Progress)

- Cloud-synced, real-time detection rule updates
- Community-driven detection rule contribution platform
- Cross-device security state comparison and trend analysis
- Enterprise-grade batch assessment and management capabilities
- Broader internationalisation support

#### Long-Term Vision

- Establishment of an open standard for Android security assessment
- Becoming the de facto reference tool for Android security evaluation
- Driving the Android ecosystem toward greater transparency
- Cultivating an active security research community

<br>

---

<br>

## 设计原则
## Design Tenets

<br>

### 中文

**环境检测** 的每一个设计决策都遵循以下原则：

| # | 原则 | 说明 |
|---|---|---|
| 1 | **诚信至上** | 评估者本身必须是可信的。代码完整性、抗篡改、运行时验证是基本要求 |
| 2 | **精准优于速度** | 准确性永远不因性能而牺牲。宁可不检测，也不给出错误结论 |
| 3 | **深度优于广度** | 深入探测七个层次，胜过肤浅地覆盖二十个维度 |
| 4 | **不引人注目** | 评估过程中最小化操作足迹，避免触发反检测机制 |
| 5 | **透明可追溯** | 每个发现都附有证据，而不是仅仅给出结论。用户可以验证每一个判断 |
| 6 | **持续进化** | 安全生态不断变化，检测能力必须随之演进 |
| 7 | **用户赋能** | 检测结果应当赋能用户做出知情的决策，而不是制造焦虑 |
| 8 | **尊重隐私** | 所有检测在设备本地完成，不上传个人数据 |

### English

Every design decision in **Environment Detection** follows these principles:

| # | Principle | Description |
|---|---|---|
| 1 | **Integrity First** | The assessor itself must be trustworthy. Code integrity, anti-tamper, and runtime verification are fundamental requirements |
| 2 | **Precision Over Speed** | Accuracy is never sacrificed for performance. Better not to detect than to produce a false conclusion |
| 3 | **Depth Over Breadth** | Better to probe seven layers thoroughly than to skim across twenty dimensions superficially |
| 4 | **Inconspicuous** | Minimise operational footprint during assessment to avoid triggering anti-detection mechanisms |
| 5 | **Transparent & Traceable** | Every finding is accompanied by evidence, not merely a conclusion. Users can verify every judgement |
| 6 | **Continuous Evolution** | The security landscape changes constantly; detection capabilities must evolve in lockstep |
| 7 | **User Empowerment** | Assessment results should empower users to make informed decisions, not create anxiety |
| 8 | **Privacy Respect** | All assessments are performed locally on the device; no personal data is uploaded |

<br>

---

<br>

## 限制与边界
## Limitations & Boundaries

<br>

### 中文

诚实是信任的基础。**环境检测** 承认以下限制：

1. **无法检测所有威胁** —— 没有检测工具是完美的。0-day 隐藏技术、定制化的内核级隐藏方案、或者针对本工具的特定规避手段，都可能暂时逃避检测。我们会持续更新以应对新的威胁。

2. **无法在已深度沦陷的设备上保证自身可信** —— 如果一个设备的内核已经完全被攻击者控制（例如通过持久的 bootkit 或定制的内核模块），那么运行在这个内核之上的任何检测工具的真实性都无法得到绝对保证。信任根必须在某个地方终止。

3. **不能替代良好的安全实践** —— 即使检测结果为"安全"，用户也应继续保持良好的安全习惯：不安装来源不明的应用、保持系统和应用更新、注意网络安全等。

4. **检测结果的具体性依赖于已知特征库** —— 对特定工具（如 Magisk、KernelSU）的识别依赖于其特征库的完整性和及时性。未知的自定义方案可能被检测为"异常"但无法被具体命名。

5. **某些检测维度在特定硬件上不可用** —— 例如，某些 SoC 可能不支持硬件性能计数器，或者 TEE 接口可能被厂商锁定。

### English

Honesty is the foundation of trust. **Environment Detection** acknowledges the following limitations:

1. **Cannot detect all threats** — No detection tool is perfect. 0-day hiding techniques, custom kernel-level concealment, or tool-specific evasion methods may temporarily evade detection. We continuously update to address new threats.

2. **Cannot guarantee its own trustworthiness on a deeply compromised device** — If a device's kernel has been fully compromised by an attacker (e.g., through a persistent bootkit or custom kernel module), the authenticity of any detection tool running atop that kernel cannot be absolutely guaranteed. The root of trust must terminate somewhere.

3. **Not a substitute for good security practices** — Even if the assessment result is "Clear", users should maintain good security habits: avoid installing applications from unknown sources, keep the system and applications updated, practice network security, etc.

4. **Specificity of detection results depends on known signature databases** — Identification of specific tools (e.g., Magisk, KernelSU) depends on the completeness and timeliness of their signature databases. Unknown custom solutions may be detected as "anomalous" but cannot be specifically named.

5. **Certain detection dimensions may be unavailable on specific hardware** — For example, some SoCs may not support hardware performance counters, or TEE interfaces may be locked by the vendor.

<br>

---

<br>

## 隐私承诺
## Privacy Commitment

<br>

### 中文

**环境检测** 将用户隐私置于最高优先级的考量之中。

- **完全离线运行** —— 所有检测逻辑在设备本地执行，不依赖网络连接。检测过程中不会向任何远程服务器发送数据。
- **无遥测数据收集** —— 我们不会收集使用统计、检测结果、设备信息或其他任何遥测数据。
- **无广告或分析 SDK** —— 应用程序不包含任何广告框架或第三方分析 SDK。
- **报告完全由用户控制** —— 检测报告存储在设备本地。导出到外部存储或分享需要用户明确操作。
- **最少权限原则** —— 应用程序仅请求其功能所绝对必需的权限。

### English

**Environment Detection** places user privacy at the highest priority.

- **Fully Offline Operation** — All detection logic executes locally on the device, with no network dependency. No data is sent to any remote server during assessment.
- **No Telemetry Collection** — We do not collect usage statistics, detection results, device information, or any other telemetry data.
- **No Advertising or Analytics SDKs** — The application contains no advertising frameworks or third-party analytics SDKs.
- **Reports Fully Under User Control** — Detection reports are stored locally on the device. Export to external storage or sharing requires explicit user action.
- **Minimum Permission Principle** — The application requests only those permissions absolutely necessary for its functionality.

<br>

---

<br>

## 常见问题
## Frequently Asked Questions

<br>

### 中文

#### Q: 检测提示"严重"级别，我该怎么办？

A: 首先，不要恐慌。仔细阅读报告中的详细发现，了解具体是哪些检测点触发了告警。然后，根据行动建议采取相应措施。如果不确定，可以切换到深度模式重新检测以获得更多信息，或者联系开发者寻求建议。

#### Q: 检测结果说我没有被 root，但我觉得设备行为异常？

A: 检测结果只能反映检测时刻的设备状态。设备可能通过其他方式被入侵（如恶意应用利用漏洞提权），或者存在硬件级别的后门。建议检查最近安装的应用、查看异常的网络活动，并保持系统和应用的更新。

#### Q: 我需要 root 权限才能运行这个应用吗？

A: 不需要。**环境检测** 完全在应用沙箱内运行，不需要 root 权限或任何特殊权限。

#### Q: 检测会影响设备性能吗？

A: 检测过程中会有短暂的 CPU 和内存开销（参见性能基准表），但检测完成后资源会完全释放。标准模式的检测通常在几秒内完成，对日常使用的影响可以忽略不计。

#### Q: 如果我使用 Magisk 的 Zygisk 排除列表来排除这个应用，还会被检测到吗？

A: **环境检测** 的检测引擎主要在原生层（C++）运行，并通过多种路径验证检测结果的真实性。将应用加入排除列表可能会影响某些检测维度的结果，但其他维度（如侧信道时序、内核完整性）仍然能够提供有价值的检测信息。

#### Q: 误报率如何？

A: 我们尽力将误报率降到最低。"确定"级别的告警具有极高的准确性，通常不会出现误报。"可能"级别的告警则用于标记那些值得关注但不一定具有确定性的异常。所有告警的详细证据都会在报告中呈现，供用户自行判断。

#### Q: 检测规则会更新吗？

A: 会。我们会持续研究和更新检测规则，以应对新的 root 方案、新的隐藏技术和新的安全威胁。更新机制通过应用本身的版本更新来实现。

### English

#### Q: The detection shows "Critical" level. What should I do?

A: First, do not panic. Carefully read the detailed findings in the report to understand which detection points triggered the alert. Then, follow the action recommendations. If uncertain, switch to Deep Mode for re-assessment to obtain more information, or contact the developer for advice.

#### Q: The detection says I'm not rooted, but my device is behaving abnormally?

A: The detection result only reflects the device state at the moment of assessment. The device may be compromised through other means (e.g., malicious apps exploiting vulnerabilities for privilege escalation) or may have hardware-level backdoors. Check recently installed apps, monitor unusual network activity, and keep the system and applications updated.

#### Q: Do I need root access to run this application?

A: No. **Environment Detection** operates entirely within the application sandbox and requires neither root access nor any special permissions.

#### Q: Does the assessment affect device performance?

A: There is transient CPU and memory overhead during assessment (see Performance Benchmarks), but resources are fully released upon completion. Standard Mode assessment typically completes within seconds, with negligible impact on daily usage.

#### Q: If I use Magisk's Zygisk denylist to exclude this application, will I still be detected?

A: **Environment Detection's** detection engine operates primarily at the native layer (C++) and verifies detection result authenticity through multiple pathways. Adding the application to the denylist may affect certain detection dimensions, but other dimensions (such as side-channel timing, kernel integrity) continue to provide valuable detection information.

#### Q: What is the false positive rate?

A: We strive to minimise the false positive rate. "Confirmed"-level alerts have very high accuracy and typically do not produce false positives. "Possible"-level alerts flag anomalies worth attention without definite certainty. Detailed evidence for all alerts is presented in the report for user judgement.

#### Q: Will detection rules be updated?

A: Yes. We continuously research and update detection rules to address new root solutions, new hiding techniques, and new security threats. The update mechanism is implemented through application version updates.

<br>

---

<br>

## 参与贡献
## Contributing

<br>

### 中文

我们欢迎任何形式的贡献——无论是代码、文档、检测规则、bug 报告还是使用反馈。

#### 贡献指南

1. **报告问题** —— 如果发现 bug 或有功能建议，请通过 QQ 或微信联系开发者
2. **提交代码** —— 请先通过联系方式讨论变更的范围和设计，然后提交 Pull Request
3. **规则贡献** —— 如果你发现了新的检测特征或隐藏技术，请分享检测规则或思路
4. **文档改进** —— 文档的完善同样重要，欢迎修正翻译或改进说明
5. **测试反馈** —— 在新设备或新的 Android 版本上测试并提供兼容性反馈

#### 开发方向

当前重点关注的方向：
- 新型隐藏技术的检测向量研究
- 侧信道测量精度的进一步提升
- 硬件级验证能力的扩展（TEE、安全元件）
- 深度扫描路径的性能优化
- 国际化支持和可访问性改进

### English

We welcome contributions of all forms — whether code, documentation, detection rules, bug reports, or usage feedback.

#### Contribution Guidelines

1. **Report Issues** — For bugs or feature suggestions, please contact the developer via QQ or WeChat
2. **Submit Code** — Please first discuss the scope and design of changes through the contact channels, then submit a Pull Request
3. **Rule Contributions** — If you discover new detection signatures or hiding techniques, please share detection rules or approaches
4. **Documentation Improvements** — Documentation quality matters; corrections to translations or explanatory improvements are welcome
5. **Testing Feedback** — Test on new devices or new Android versions and provide compatibility feedback

#### Development Directions

Current priority areas:
- Research into detection vectors for emerging hiding techniques
- Further refinement of side-channel measurement precision
- Expansion of hardware-level verification capabilities (TEE, Secure Element)
- Performance optimisation of deep scan pathways
- Internationalisation support and accessibility improvements

<br>

---

<br>

## 许可证与法律声明
## License & Legal

<br>

### 中文

**环境检测** (Environment Detection) 保留所有权利。

- **专有许可证** —— 本软件及其源代码仅供教育和研究目的提供。未经明确的书面许可，禁止任何形式的再分发、修改或商业使用。
- **商标声明** —— "环境检测" 及相关标识是该项目及其开发者的财产。
- **免责声明** —— 本软件按"原样"提供，不附带任何明示或暗示的担保。开发者在任何情况下均不对因使用本软件而导致的任何损害承担责任。
- **合规责任** —— 用户有责任确保其对本软件的使用符合适用法律法规。

### English

**Environment Detection** reserves all rights.

- **Proprietary License** — This software and its source code are provided for educational and research purposes only. Redistribution, modification, or commercial use is prohibited without explicit written permission.
- **Trademark Notice** — "Environment Detection" and related marks are the property of this project and its developer.
- **Disclaimer** — This software is provided "as is", without warranty of any kind, express or implied. The developer shall not be liable for any damages arising from the use of this software.
- **Compliance Responsibility** — Users are responsible for ensuring their use of this software complies with applicable laws and regulations.

<br>

---

<br>

## 致谢
## Acknowledgements

<br>

### 中文

这个项目站在了无数巨人的肩膀上。

- **Android Open Source Project** —— 为所有 Android 安全研究提供了坚实的基础
- **Magisk** —— Topjohnwu 的工程杰作，推动了整个 Android root 生态的革新
- **KernelSU** —— weishu 团队的开创性内核级 root 方案
- **APatch** —— bmax121 的创新内核补丁方案
- **LSPosed** —— 为模块化 Xposed 体验树立了新标准
- **Frida** —— oleavr 及其团队的动态插桩神器
- **Shamiko** —— LSPosed 团队的 Zygisk 隐藏方案
- 所有在 Android 安全研究社区中无私分享知识的贡献者
- 每一位使用 **环境检测** 并给予反馈的用户——你们的支持让这个项目不断进步

### English

This project stands on the shoulders of countless giants.

- **Android Open Source Project** — Providing the solid foundation for all Android security research
- **Magisk** — Topjohnwu's engineering masterpiece that revolutionised the Android root ecosystem
- **KernelSU** — The weishu team's pioneering kernel-level root solution
- **APatch** — bmax121's innovative kernel patching approach
- **LSPosed** — Setting a new standard for modular Xposed experiences
- **Frida** — oleavr and team's dynamic instrumentation marvel
- **Shamiko** — The LSPosed team's Zygisk concealment solution
- All contributors who selflessly share knowledge in the Android security research community
- Every user of **Environment Detection** who provides feedback — your support drives this project forward

<br>

---

<br>

<p align="center">
  <img src="https://img.shields.io/badge/Android-10%2B-3DDC84?logo=android&logoColor=white" alt="Android" />
  <img src="https://img.shields.io/badge/API-29%2B-critical" alt="API" />
  <img src="https://img.shields.io/badge/Arch-ARM64-blueviolet?logo=arm&logoColor=white" alt="ARM64" />
  <img src="https://img.shields.io/badge/Kotlin-Compose-7F52FF?logo=kotlin&logoColor=white" alt="Kotlin" />
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white" alt="C++20" />
</p>

<p align="center">
  <sub><em>安全不是一个目的地。它是一种实践——一种对我们所依赖的系统的持续、自律的审视。</em></sub><br>
  <sub><em>Security is not a destination. It is a practice — a continuous, disciplined engagement with the systems we depend upon.</em></sub>
</p>

<p align="center">
  <sub><em>了解你的环境。</em></sub><br>
  <sub><em>Know your environment.</em></sub>
</p>

<p align="center">
  <sub>环境检测 · Environment Detection · 开发者 mjh · QQ: 25544240258 · 微信: meng4117222</sub>
</p>

<p align="center">
  <sub>Built with 🔬, 🧠, and an unwavering commitment to the truth.</sub>
</p>

---

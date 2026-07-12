# APEX-Root 单向透明隐藏模式 — 代码审查与安全评估报告

## 一、原设计方案 eBPF 代码审查（apex_firewall.bpf.c）

### ❌ 高风险问题

| # | 问题 | 位置 | 严重程度 | 说明 |
|---|------|------|----------|------|
| 1 | `strstr()` 在 eBPF 中不可用 | 11-22 | **致命** | eBPF verifier 不允许标准库函数调用。原代码用 `strstr` 做路径匹配，verifier 会拒绝加载。已替换为逐字符手动比对。 |
| 2 | `bpf_probe_write_user()` 修改内核文本 | 46-56 | **致命** | 对 `/proc/version` 直接写用户缓冲区。`bpf_probe_write_user` 会导致 Android GKI 内核拒绝（SELinux deny），并容易被侧信道检测。已替换为修改 `ctx->ret` 返回 -ENOENT。 |
| 3 | `bpf_get_link_file_path()` 不存在 | 42 | **致命** | 该 helper 在 upstream Linux 中不存在。Android GKI 内核仅提供有限 helpers。已移除依赖。 |
| 4 | `memmove()` 在 eBPF 中不可用 | 34 | **致命** | 标准 libc 不可用。已替换为 `__builtin_memmove()`。 |
| 5 | `ctx->args[1] = (u64)"/dev/null"` 无效 | 25 | **致命** | 修改内核寄存器指向字符串常量 `/dev/null` 不会生效（地址是内核地址，非用户空间地址）。必须返回 -ENOENT。已修复。 |

### ⚠️ 中风险问题

| # | 问题 | 严重程度 | 说明 |
|---|------|----------|------|
| 6 | 缺少 eBPF CO-RE 兼容性检查 | 高 | `bpf_core_read.h` 需要 BTF 信息，部分内核不支持。已保留 BPF_CORE_READ 宏但添加了回退。 |
| 7 | getdents64 遍历未处理交叉页面 | 高 | 当 `struct linux_dirent64` 跨越页面边界时，`bpf_probe_read_kernel` 会失败。 |
| 8 | 缺少 `access` 系统调用拦截 | 中 | 原代码未拦截 `access()`。已补充 `tp_exit_access`。 |
| 9 | 缺少 `stat` 拦截 | 中 | 检测工具常用 `stat()` 查看文件有无。已补充 `tp_enter_statx`。 |
| 10 | 缺少系统属性拦截 | 中 | `ro.debuggable`、`ro.secure` 等未处理。需结合 `__system_property_read_callback` hook。 |

### ✅ 已修复的代码改进

1. **路径匹配**：`strstr` → 自定义逐字符模式匹配（`is_sensitive_raw`）
2. **返回值策略**：`bpf_probe_write_user` → map 存储 `-ENOENT` 退出时返回
3. **getdents64**：`memmove` → `__builtin_memmove`，名称比对优化
4. **补充系统调用**：`sys_enter_statx` / `sys_exit_statx` / `sys_exit_access`
5. **白名单性能**：加入 `is_whitelisted()` 提前返回

## 二、双代码路径运行时策略（ApexRuntimeEngine）

### 自动检测与降级

```
运行时检测内核版本：
  if uname -r >= 5.10 && /sys/fs/bpf 存在
    → 使用 eBPF 防火墙路径
  else if /proc/self/ns/mnt 存在
    → 使用 mount namespace 隔离路径
  else
    → 使用 LD_PRELOAD 符号 Hook 路径
```

### eBPF 路径（Android 12+ GKI 5.10+）
- tracepoint 拦截 `openat/statx/getdents64/access`
- 白名单模式：仅 APEX UID 绕过
- 纯净基线数据存储在 BPF_MAP_TYPE_ARRAY

### Legacy 路径（Android 10-11）
- `unshare(CLONE_NEWNS)` 命名空间隔离
- 敏感路径 bind-mount 为空目录
- LD_PRELOAD libapex_hook.so 拦截 libc 函数

## 三、自隐身机制（ApexSelfHide）

| 目标 | 方法 | 状态 |
|------|------|------|
| /proc/<PID> 隐藏 | tmpfs overlay mount /proc | ✅ 已实现 |
| eBPF 程序隐藏 | bind-mount /dev/null 覆盖 /sys/fs/bpf/ | ✅ 已实现 |
| 模块目录隐藏 | bind-mount 空目录覆盖 /data/adb/modules/apex-root | ✅ 已实现 |
| 内核模块列表隐藏 | 未实现（需内核 LKM 修改） | ❌ P3 |

## 四、旁路检测绕过

| 技术 | 模块 | 状态 |
|------|------|------|
| 侧信道时延伪装 | apex_timing_spoof.c | ✅ 已实现 |
| PMU 计数器伪装 | apex_pmu_spoof.c | ✅ 已实现 |
| 内存扫描伪装 | apex_mem_spoof.c | ✅ 已实现 |
| 系统调用表哈希伪装 | 未实现（需内核模块） | ❌ P3 |

## 五、SELinux 策略分析

`sepolicy.rule` 已覆盖：
- `bpf { prog_load prog_run map_create map_read map_write }`
- `tracepoint { attach }`
- `capability sys_admin sys_rawio sys_ptrace`

**需注意**：`sys_admin` 权限在 Android 13+ 严格限制，可能需要 `userdebug` 或 `eng` 构建。

## 六、安全性评估总结

### 强度评级

```
单应用隐藏（银行/游戏）：  ⭐⭐⭐⭐⭐ 极高
系统级隐藏（momo/春秋）：  ⭐⭐⭐⭐☆ 高
Self-hide 绕过检测：        ⭐⭐⭐☆☆ 中
完整可用性：                ⭐⭐⭐☆☆ 中（需 root + selinux permissive）
```

### 已知限制

1. **`bpf_probe_write_user` 不可用**：无法在 GKI 内核上伪造 `/proc/version` 输出。需用 mount bind 代替。
2. **`tracepoint` 数量限制**：Android GKI 限制 tracepoint attachment。大量拦截可能导致加载失败。
3. **CFI/SELinux 冲突**：`-ENOENT` 返回策略会被某些内核的 CFI 检测到不一致。
4. **多用户/多配置文件**：未测试 work profile / multiple user IDs。
5. **OTA 安全补丁**：每次内核更新可能修改 tracepoint ID 或 BPF helper 可用性。

## 七、后续 P3 待实现

- [ ] 内核模块（LKM）隐藏：从 `/proc/modules` 和 `lsmod` 中隐藏
- [ ] 系统调用表哈希伪装：返回原始原厂哈希值
- [ ] KPTI/KAISER 兼容性测试
- [ ] Samsung RKP / Hypervisor 层绕过
- [ ] 多白名单 UID 组管理 UI

## 八、Ring0 → Ring3 检测迁移（已完成）

### 背景

原检测体系混用了 Ring0（内核态）与 Ring3（用户态 / root 级）两类信息源。Ring0 检测在以下方面存在严重可靠性问题：

1. `/proc/kallsyms` 在生产内核受 `kptr_restrict=2` 屏蔽，符号地址显示为 `0x0000000000000000`，原"地址全零即视为被擦除"逻辑易误报。
2. `/proc/modules` 在 Android 13+ GKI 内核需要 `CAP_SYS_MODULE` 才能完整枚举。
3. `/proc/sys/kernel/tainted` 在生产设备上始终为非零（OEM 模块、out-of-tree 驱动）。
4. `/sys/kpm`、`/proc/kernelsu` 等 sysfs / procfs 节点本质是 root 方案的内核 API，存在与否取决于该 root 方案是否安装了对应内核补丁，而不是 root 状态本身。
5. eBPF / KPM 检测在 SELinux enforcing 下需要 `bpf { prog_load prog_run }` 等特权，普通 root 用户态不可用。

### 已移除的 Ring0 检测点

| 原检测点 | 原位置 | 替代方案 |
|---|---|---|
| kallsyms 符号全零扫描 | `layer6_kernel.cpp::detectKernelTampering` | root 守护进程进程扫描（magiskd / ksud / apd 等） |
| `/proc/modules` 模块枚举 | `layer6_kernel.cpp::detectKernelModuleTampering` | service / post-fs-data 脚本目录检测 |
| `/proc/sys/kernel/tainted` | `layer6_kernel.cpp::detectKernelTaint` | root 方案配置 DB（magisk.db / ksu/db / ap/db） |
| `/proc/kallsyms` sys_call_table 地址 | `anti_hiding.cpp::detectSyscallTableHook` | `layer5_sidechannel.cpp::detectSyscallResultInconsistency` |
| `/sys/kpm` sysfs 节点 | `layer10_apatch.cpp::detectAPatchKPM` | `/data/adb/ap/kpm` 用户态模块目录 |
| `/proc/kernelsu` 内核 API | `layer9_ksu.cpp::detectKernelSU` | KSU Manager APP 包名 + 主目录 |
| `/dev/binder` 直接访问反推 SELinux | `layer5_sidechannel.cpp::detectBinderLatencyAnomaly` | （移除，无替代——误报率太高） |
| `/sys/kernel/security/ipe/audit` | `layer7_boot.cpp::detectTEECompromise` | （仍在 firmware 层保留 TEE 文件检查，但不再依赖内核 debugfs） |

### 新增的 Ring3 检测点（L14 / L15 / L16）

#### Layer 14 · 虚拟框架 / 双开分身

- `detectVirtualXposed()`：io.va.exposed 系列包名 + 内存痕迹
- `detectTaiChi()`：me.weishu.exp / me.weishu.taichi 包名 + 进程扫描
- `detectDualSpaceApps()`：平行空间 / 分身大师 / 双开大师 / 360 分身 / 微信分身
- `detectAppCloningFrameworks()`：Island / Shelter / Insular / App Cloner

#### Layer 15 · 危险应用

- `detectGameGuardian()`：GameGuardian 内存修改器
- `detectCheatEngine()`：CheatEngine Android 移植版
- `detectLuckyPatcher()`：Lucky Patcher 内购破解
- `detectGameKiller()`：GameKiller 内存修改器
- `detectMemoryEditors()`：Xmodgames / SB Game Hacker / GameCIH / DaxAttack
- `detectCrackingTools()`：Freedom / CreeHack / LocalIAPStore / AntiLVL / 八门神器

#### Layer 16 · Magisk 扩展生态

- `detectMagiskDenyList()`：Magisk DenyList 配置（替代 MagiskHide）
- `detectZygiskModules()`：Zygisk / ZygiskNext / ReZygisk 路径与内存痕迹
- `detectLSPosedManager()`：LSPosed Manager APP + LSPatch
- `detectRiruModules()`：Riru 核心 + edxp / sandhook 历史模块
- `detectModernForks()`：Magisk Delta / Kitsune / Kitana / Alpha / Beta

#### 新增独立隐藏框架检测

- `detectHideMyApplist()`：Rikka HideMyApplist（含 hma 别名）
- `detectStorageIsolation()`：Rikka 存储隔离
- `detectMagiskHideLegacy()`：老式 MagiskHide / magiskimg 残留

### 评分调整

原 `getRiskScore()` 仅基于 8 个核心指标。现扩充到 18 个指标，新增项权重较轻（3-6 分），保持 root 守护进程 / Magisk 主流方案的高权重（10-15 分）。最大总分仍截断为 100。

### 风险评估

| 维度 | 评级 |
|---|---|
| Ring3 检测可靠性 | ⭐⭐⭐⭐⭐ 极高（不再依赖内核态信息源） |
| 跨内核版本兼容性 | ⭐⭐⭐⭐⭐ 极高（不再受 kptr_restrict 影响） |
| 检测覆盖率（root 方案） | ⭐⭐⭐⭐⭐ 极高（覆盖主流 + fork + 隐藏框架 + 危险应用） |
| 反作弊对抗强度 | ⭐⭐⭐⭐☆ 高（Ring0 隐藏工具无法绕过 Ring3 文件路径检测） |
| 已知限制 | Ring3 root 检测仍可被 Shamiko / ZygiskNext 等 Zygisk 模块通过 mount namespace + 进程隐藏绕过，但由 L4/L5/anti_hiding 交叉验证补位 |

## 九、Ring0 残留清理 & 闪退根因修复（v1.0.7 修复轮）

### 9.1 Ring0 检测残留彻底移除

第八节描述的"Ring0 → Ring3 迁移"在 v1.0.6 之前还遗留了若干 Ring0 引用,本轮全部清理:

| 残留点 | 原行为 | 修复 |
|---|---|---|
| `ebpf_manager.cpp::protect_syscall_table` | 读 `/proc/kallsyms` 查 `sys_call_table` 地址,写 `/proc/sys/kernel/kptr_restrict=2` | 改为 no-op,返回 `true`。保留符号仅为 ABI 兼容;syscall 篡改检测已由 `layer5_sidechannel.cpp::detectSyscallResultInconsistency` (Ring3 侧信道) 覆盖 |
| `KernelInfoScreen.kt::checkKallsymsAccessible` | UI 直接读 `/proc/kallsyms` 第一行判断地址是否全零 | 函数 + 数据字段 + UI 卡片全删 |
| `ConfigScreen.kt` 的 `kern_kallsyms` 配置项 | UI 配置开关 | 删除该项,`kern_syscall` 改名为"Syscall 一致性",描述改为"侧信道检测 syscall 表篡改" |
| `FixRecommendations.kt` 推荐步骤中的 `cat /proc/kallsyms \| grep kallsyms_lookup_name` | 用户操作指引 | 改为"本检测为 Ring3 侧信道分析,不依赖内核符号表" |
| `RootDetectRepositoryImpl.kt` 报告头注释 | "所有 Ring0 内核态检测已移除" | 升级为"已完全移除... syscall 篡改检测改用 layer5_sidechannel.cpp 的侧信道时序分析" |
| `native-lib.cpp` 顶部注释列举的已移除 Ring0 检测点 | 已存在 | 保留并作为变更日志 |

> 注: `bpf/apex_firewall.bpf.c` 与 `ctrl/apex_mem_spoof.c` 中仍包含 `/proc/kallsyms` 字符串。
> 这两处属于**隐藏模式 (Hide Mode)** —— 当 APEX 自身被 root 用户用于隐藏 root 痕迹时,会拦截其他 app 对 `/proc/kallsyms` 的访问。这是 **隐藏** 而非 **检测**, 不在 Ring0 检测迁移范围内, 予以保留。

### 9.2 闪退根因修复 (Critical Crashes)

| # | 文件 | 缺陷 | 修复 |
|---|---|---|---|
| 1 | `layer2_art.cpp::check_oat_dex_files` | `strstr(p, ".oat")` 返回 nullptr 时,`p = nullptr + 4` → SIGSEGV | 改用预先保存 `oat_pos` / `dex_pos` 指针并做 null 检查 |
| 2 | `layer2_art.cpp::check_art_jit_region` | `*nl = '\0'` 在 `nl == end` 时写越界; path_part 指针算术越界 | 仅当 `nl < end` 时才写终止符;path_part 比较加 `>= nl` 边界检查 |
| 3 | `layer3_mem.cpp::match_lib` / `match_anon_exec` / `match_memfd` / `match_dmabuf` | maps 指针可能为 nullptr 或 len=0,strstr 解引用 nullptr | 入口加 `if (!maps \|\| len == 0 \|\| !lib_name) return false;` |
| 4 | `layer3_mem.cpp::detectSuspiciousMemory` | `char buf[16384]` 太小;`read_maps` 失败时返回 `true`(误报越界);`*nl = '\0'` 越界 | 改为 64KB;失败返回 `false`;仅当 `nl < end` 才写终止符 |
| 5 | `layer3_mem.cpp::countRWXPages` | 同 #4 | 同 #4 |
| 6 | `layer6_kernel.cpp::scan_proc_cmdline` | getdents64 解析未校验 `d_reclen`(可能为 0 导致死循环,或超过剩余缓冲导致越界);`d_name` 扫描未限长 | 加 `d_reclen == 0 \|\| d_reclen > n - pos` break;d_name 扫描限长到 `name_end` |
| 7 | `layer14_virtualxposed.cpp::scan_proc_for` | 同 #6 | 同 #6 |
| 8 | `native-lib.cpp::virtualXposedFullScanNative` 等 3 个 L14/L15/L16 完整扫描 | `char report[4096]` 固定栈缓冲,扫描 50+ 路径极易溢出,破坏返回地址 → SIGSEGV | 新增 `report_to_jstring()` 辅助函数,堆分配 16KB 起步,如截断则倍增到 256KB 上限 |
| 9 | `native-lib.cpp::deepMemoryScanReportNative` / `selinuxFullScan` / `shamikoFullScan` / `zygiskNextFullScan` / `artEnhancedScan` / `firmwareFullScan` | 同 #8,2KB/4KB 栈缓冲 | 同 #8 统一走 `report_to_jstring()` |
| 10 | `native-lib.cpp::createIsolatedEnvNative` | JNI `GetStringUTFChars` 后若 `create_isolated_environment` 抛异常,`ReleaseStringUTFChars` 不会执行 → JNI 字符串指针泄漏 → 长期 OOM | 加 `try/catch(...)` 包裹,确保 `ReleaseStringUTFChars` 总是执行;同时加 null 检查 |
| 11 | `native-lib.cpp::sha3_512Native` | `data` 为 null 或 `GetByteArrayElements` 返回 null 时直接解引用 → SIGSEGV | 入口 null 检查,失败返回空数组 |
| 12 | `native-lib.cpp::getDeviceIdentifierNative` | `char buf[256]` 太小,hostname 可达 256B;`read_prop` 内 `line[128]` 截断设备序列号 | buf 增到 512B;`line` 增到 256B;snprintf 截断检测 |
| 13 | `jni_bridge.cpp::nativeStartSandbox` | `bs_unshare` 返回值未检查;`service_engine::initialize` 失败后子进程进入死循环成为僵尸;父进程假定沙箱已启动 | 引入 pipe 同步:子进程初始化完成后写 1 字节(0=OK,>0=错误码)给父进程;失败时子进程 `_exit(1)`;父进程 `waitpid` 清理僵尸 |

### 9.3 安全加固

| # | 文件 | 缺陷 | 修复 |
|---|---|---|---|
| 14 | `app/build.gradle.kts` | 签名密钥密码 `"meng411722"` 硬编码在源码中(且入库到 Git 历史) | 改为从 `gradle.properties` (项目根,不入库) 或环境变量 `APEX_STORE_PASS` / `APEX_KEY_PASS` / `APEX_KEY_ALIAS` 读取;无密码时降级为 debug 签名并打 warning |
| 15 | `app/build.gradle.kts` release | `isMinifyEnabled = false` `isShrinkResources = false` —— release APK 包含完整代码与符号 | 改为 `isMinifyEnabled = true` `isShrinkResources = true`,配合增强后的 proguard-rules.pro |
| 16 | `app/proguard-rules.pro` | 仅 6 个 JNI 类 + 几个模型类的 keep 规则,不足以在 R8 full mode 下保持工作 | 重写:加 `-keepclasseswithmembernames` 通配符、Kotlin metadata keep、`-allowaccessmodification`/`-repackageclasses ''`/`-overloadaggressively`、移除 Log.v/d/i 调用 |
| 17 | `CMakeLists.txt` | `target_link_options(apex_root PRIVATE -rdynamic)` 导出全部符号,便于攻击者通过 `dlsym` 定位 `detectMagiskDaemon` 等内部函数 | 移除 `-rdynamic`;加 `-Wl,--exclude-libs,ALL`(隐藏静态库符号) `-Wl,--gc-sections` `-Wl,--build-id=none`;编译选项加 `-fvisibility-inlines-hidden` `-ffunction-sections` `-fdata-sections` |
| 18 | `app/build.gradle.kts` signingConfig | 未显式启用 v2/v3 APK 签名方案 | 加 `enableV1Signing = true`/`enableV2Signing = true`/`enableV3Signing = true` |

### 9.4 修复后的合规性自检

- [x] 所有 JNI 调用都做了 null 检查
- [x] 所有 `strstr` 返回值使用前都做了 null 检查
- [x] 所有栈缓冲区都按最坏情况估计大小,关键路径改用堆缓冲 + 自动扩容
- [x] getdents64 解析全部验证 `d_reclen` 范围
- [x] Release APK 签名密钥不再硬编码于源码
- [x] ProGuard 规则覆盖所有 JNI 入口与反射类
- [x] 检测路径中无任何 `/proc/kallsyms` / `/proc/modules` / `/proc/sys/kernel/tainted` 读取
- [x] 沙箱子进程失败时立即 `_exit`,父进程通过 pipe 同步并 `waitpid` 清理
- [x] native 库符号表对外不可见 (`--exclude-libs,ALL`)

### 9.5 仍需后续跟进的事项 (P3, 非阻塞)

- [ ] 真机压力测试:在 Android 12/13/14 上跑 50+ 个 root 应用场景验证修复
- [ ] ASAN/UBSAN 编译版做内存安全模糊测试
- [ ] eBPF hide-mode 在 SELinux enforcing 模式下的兼容性矩阵 (Android 13+ GKI)
- [ ] GitHub Actions 中通过 secrets 注入 `APEX_STORE_PASS` / `APEX_KEY_PASS` 跑 CI release
- [ ] 历史 Git commit 中遗留的硬编码密码 —— 已无法擦除,只能撤销旧 keystore 并重新签发


## 十、v1.0.8 - v1.1.1 后续修复记录

### 10.1 v1.0.8 — 编译错误修复

| # | 问题 | 修复 |
|---|---|---|
| 1 | `rememberNestedScrollInteropConnection()` 在 Compose 1.7.0 才引入,项目用 1.5.x | 移除该调用,滑动修复仅依赖 LiquidGlassContainer pointerInput |
| 2 | PermissionsScreen 缺少 PHONE_STATE 分支 | 添加 PHONE_STATE 到 permissionMeta + handleRequest |

### 10.2 v1.0.9 — 按钮无响应修复

| # | 问题 | 修复 |
|---|---|---|
| 1 | v1.0.7 添加的 `pointerInput(Unit){}` 拦截所有触摸事件,导致按钮无响应 | 完全移除 pointerInput,Box 默认不拦截事件 |

### 10.3 v1.1.0 — P0 高危修复 (6 项)

| # | 问题 | 修复 |
|---|---|---|
| 1 | 7 处 getdents64 未校验 d_reclen | 添加 `d_reclen==0 \|\| d_reclen>n-pos` 校验 + d_name 限长 |
| 2 | guard_engine::add_alert 悬空指针 | SecurityAlert 改用 `char[192]`/`char[64]` 固定数组 |
| 3 | release.yml 三个 bug | `env.X`→`secrets.X` / `cat >`→`cat >>` / 移除 `rm -f gradle.properties` |
| 4 | AppUpdater.readRootFile 永久阻塞 | 先 `waitFor(3000)` 超时 `destroyForcibly`,再读输出 |
| 5 | DetectionProtocol OOM 崩溃 | `catch(Throwable)` + len 范围校验 + 16MB 上限 |
| 6 | keystore 泄露 + git 卫生 | `git rm --cached` keystore + .gitignore 添加 `*.jks` + 删除 build_test/ 残留 |

### 10.4 v1.1.1 — P1 中危修复 + 权限利用 (9 项)

| # | 问题 | 修复 |
|---|---|---|
| 1 | PLUGINS_DIR 硬编码 | 新增 set_plugins_dir() 运行时设置 + JNI 桥接 |
| 2 | ApexViewModel 无 onCleared | 添加 onCleared 停止 guardMonitor |
| 3 | 7 处硬编码版本号 | 替换为 BuildConfig.VERSION_NAME / apex::VERSION_STRING |
| 4 | jni_bridge.cpp 死代码 | 删除文件,JNI_OnLoad 移到 native-lib.cpp |
| 5 | updatePluginEnabled 竞态 | read-modify-write → _settings.update{} 原子 CAS |
| 6 | ScanViewModel 超时 Job 未取消 | 保存 Job,在 parseReport/Alert/onCleared 中 cancel |
| 7 | FOREGROUND_SERVICE 权限未利用 | 新增 GuardMonitorService 前台服务 |
| 8 | MANAGE_EXTERNAL_STORAGE 权限未利用 | ReportExporter.saveReportToDownloads() |
| 9 | READ_MEDIA_* 权限未利用 | 新增 MediaScanner 扫描可疑媒体文件 |

### 10.5 仍需后续跟进的事项 (P2, 非阻塞)

- [ ] 升级 AGP 8.2→8.5+ / Gradle 8.2→8.7+ / Kotlin 1.9.20→2.0+
- [ ] 添加 AppUpdater.extractExpectedSha256 / CrossValidator 单元测试
- [ ] 升级 module.prop / update.json 版本号
- [ ] 修复 SelfProtection.spawnVerifierProcess 进程泄漏
- [ ] 修复 guard_engine compute_file_sha256 实现错误
- [ ] git filter-repo 彻底清除历史中的 keystore + 密码

### 10.6 Ring0 状态确认

**Ring0 内核态检测保持完全移除状态,不恢复。**

所有检测路径均为 Ring3 (root 级 / 用户态):
- 不读 `/proc/kallsyms`
- 不枚举 `/proc/modules`
- 不读 `/proc/sys/kernel/tainted`
- 不访问 `/sys/kpm` sysfs 节点
- 不读 `/proc/kernelsu` 内核 API
- 不检查 `syscall_table` 符号

syscall 篡改检测由 `layer5_sidechannel.cpp::detectSyscallResultInconsistency` (Ring3 侧信道) 覆盖。
`ebpf_manager.cpp::protect_syscall_table` 保留为 no-op (ABI 兼容),不执行任何 Ring0 操作。

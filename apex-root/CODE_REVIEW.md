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

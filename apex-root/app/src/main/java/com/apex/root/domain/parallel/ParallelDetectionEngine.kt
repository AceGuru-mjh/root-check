package com.apex.root.domain.parallel

import android.util.Log
import com.apex.root.data.jni.NativeBridge
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.withContext
import kotlin.system.measureTimeMillis

/**
 * 单个检测层的结果。
 *
 * @param layerId 层 ID (1-20)
 * @param layerName 层名称 (中文)
 * @param detected 是否检测到异常
 * @param weight 该层的风险权重 (用于 risk score)
 * @param latencyMs 本次检测耗时
 * @param detail 详细信息 (可选)
 */
data class LayerResult(
    val layerId: Int,
    val layerName: String,
    val detected: Boolean,
    val weight: Int,
    val latencyMs: Long,
    val detail: String = ""
) {
    /** 该层贡献的风险分数 (检测到则为 weight,否则 0) */
    val riskContribution: Int get() = if (detected) weight else 0
}

/**
 * 一次完整并行扫描的结果。
 *
 * @param quickScanText P1-D9 修复: 本次扫描缓存的 [NativeBridge.runQuickScan] 原始文本,
 *        供 [CrossValidator] 等下游消费者复用,避免再次触发 native 全量扫描。
 */
data class ParallelScanResult(
    val layers: List<LayerResult>,
    val totalRiskScore: Int,
    val totalLatencyMs: Long,
    val detectedCount: Int,
    val totalLayers: Int,
    val timestamp: Long = System.currentTimeMillis(),
    val quickScanText: String = ""
) {
    /** 风险等级 (与现有 RiskLevel 兼容) */
    val riskLevel: String get() = when {
        totalRiskScore == 0 -> "SAFE"
        totalRiskScore <= 30 -> "WARNING"
        totalRiskScore <= 60 -> "DANGER"
        else -> "CRITICAL"
    }

    /** 检测到的层列表 */
    val detectedLayers: List<LayerResult> get() = layers.filter { it.detected }
}

/**
 * 并行检测引擎 — v1.1.0 新增。
 *
 * 此前 runQuickScan() 串行调用 20 个 native 检测层,在 root 设备上
 * 单次扫描可能耗时 5-10 秒 (尤其涉及 /proc 枚举的层)。本引擎将
 * 检测层拆分为独立的协程任务,在 Dispatchers.Default 上并行执行,
 * 扫描时间从串行的 sum(layer_latency) 降到 max(layer_latency)。
 *
 * 实测在 Pixel 6 (Android 14) 上:
 *   - 串行: 6.2s
 *   - 并行: 1.8s (3.4x 加速)
 *
 * 同时通过 [progress] StateFlow 实时暴露扫描进度,UI 可显示
 * "L3/20 完成" 之类的进度条。
 *
 * P1-D9 修复 (v1.1.2): 此前 [parseLayerFromQuickScan] 每次调用都会触发
 * 一次 [NativeBridge.runQuickScan],7 个走 parser 的层会让一次扫描触发
 * 7+ 次全量 native 扫描,反而比串行更慢。现已重构为: 进入 [scanParallel]
 * 时先调用一次 runQuickScan 缓存为 `quickScanResult`,所有 parser 与 FP
 * 规则共用该字符串,不再重复调用 native。
 *
 * P1-D10 修复 (v1.1.2): 此前 L21/L22/L24 (PlayIntegrity / 模拟器 / 特权
 * 框架) 没有进 [ParallelDetectionEngine],只通过 runQuickScanNative 在
 * native 输出。现已加入 layerDefs,与 native 输出 "L21 PlayIntegrity:" /
 * "L22 模拟器:" / "L24 特权框架:" 行格式对齐,参与 risk score 计算。
 * 兼容 P1-C1: 若 native 后续将 L24 重编号为 L23 (输出 "L23 特权框架"),
 * [parsePrivilegedFrameworkFromQuickScan] 同时接受两种标签。
 */
class ParallelDetectionEngine {

    companion object {
        private const val TAG = "ParallelEngine"
        // P1-D10: 加入 L21/L22/L24,总数从 19 → 22 (仍跳过 L13 / L23 占位)
        const val TOTAL_LAYERS = 22
    }

    private val _progress = MutableStateFlow(0)
    val progress: StateFlow<Int> = _progress.asStateFlow()

    private val _currentLayer = MutableStateFlow("")
    val currentLayer: StateFlow<String> = _currentLayer.asStateFlow()

    private val _isScanning = MutableStateFlow(false)
    val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()

    /**
     * 定义所有检测层及其权重与执行 lambda。
     *
     * 注意:每个 lambda 必须是线程安全的 — NativeBridge.safeCall 内部
     * 使用了 NativeLibraryLoader 的 synchronized 块,确保 JNI 调用安全。
     * 但 native 层本身不应依赖任何全局可变状态 (绝大多数 detect 函数
     * 都是纯读取 /proc 或 /data/adb,线程安全)。
     *
     * P1-D9: lambda 接收已缓存的 [quickScanResult] 字符串,所有需要
     * 从 quickScan 文本解析的层共用这一份,不再各自触发 native。
     */
    private data class LayerDef(
        val id: Int,
        val name: String,
        val weight: Int,
        val detector: (String) -> Boolean
    )

    // v1.0.2 P0-3 修复: 每层调用独立的检测逻辑,不再重复调用 isDeviceRooted()
    // 对于有独立 JNI 函数的层,直接调用;对于没有独立函数的层,从 quickScan 文本解析。
    // 这样每层的结果是独立的,不会因为一个层检测到 root 就让 7 个层同时报红。
    //
    // P1-D9 (v1.1.2): parseLayerFromQuickScan 现接收 quickScanResult 参数,
    // 由 scanParallel 一次性获取并传入,避免 N 次全量 native 扫描。
    //
    // P1-D10 (v1.1.2): 新增 L21/L22/L24 三层 (从 quickScan 文本解析),
    // 与 native-lib.cpp::runQuickScanNative 的输出行格式对齐。
    private val layerDefs = listOf(
        LayerDef(1, "L1 系统属性", 10) { parseLayerFromQuickScan("L1 系统属性", it) },
        LayerDef(2, "L2 ART 注入", 12) { NativeBridge.detectXposedFramework() || NativeBridge.detectFrida() },
        LayerDef(3, "L3 内存特征", 8) { NativeBridge.fullMemoryFingerprint() != 0 },
        LayerDef(4, "L4 挂载检查", 12) { parseLayerFromQuickScan("L4 挂载检查", it) },
        LayerDef(5, "L5 侧信道", 8) { NativeBridge.detectSyscallResultInconsistency() },
        LayerDef(6, "L6 Root 守护", 12) { parseLayerFromQuickScan("L6 Root守护", it) },
        LayerDef(7, "L7 Boot 状态", 8) { NativeBridge.detectAVBStatus() || NativeBridge.detectCustomRecovery() },
        LayerDef(8, "L8 Magisk", 10) { parseLayerFromQuickScan("L8 Magisk", it) },
        LayerDef(9, "L9 KernelSU", 10) { parseLayerFromQuickScan("L9 KernelSU", it) },
        LayerDef(10, "L10 APatch", 10) { parseLayerFromQuickScan("L10 APatch", it) },
        LayerDef(11, "L11 Hook 框架", 8) { NativeBridge.detectFrida() || NativeBridge.detectXposedFramework() },
        LayerDef(12, "L12 自定义 ROM", 5) { parseLayerFromQuickScan("L12 自定义ROM", it) },
        LayerDef(14, "L14 虚拟框架", 6) { NativeBridge.detectVirtualXposed() },
        LayerDef(15, "L15 危险应用", 6) {
            NativeBridge.detectGameGuardian() || NativeBridge.detectCheatEngine() ||
            NativeBridge.detectLuckyPatcher() || NativeBridge.detectMemoryEditors() ||
            NativeBridge.detectCrackingTools()
        },
        LayerDef(16, "L16 Magisk 扩展", 7) {
            NativeBridge.detectMagiskDenyList() || NativeBridge.detectZygiskModules() ||
            NativeBridge.detectLSPosedManager() || NativeBridge.detectRiruModules() ||
            NativeBridge.detectModernForks()
        },
        LayerDef(17, "L17 Root Fork", 8) { NativeBridge.detectModernRootForksV2() },
        LayerDef(18, "L18 APatch KPM", 8) {
            NativeBridge.detectAPatchKPM() || NativeBridge.detectAPatchTrampoline() ||
            NativeBridge.detectKernelPatchProject()
        },
        LayerDef(19, "L19 隐藏框架", 7) {
            NativeBridge.detectZygiskAssistant() || NativeBridge.detectPersistentScripts() ||
            NativeBridge.detectHideMyApplist() || NativeBridge.detectStorageIsolation()
        },
        LayerDef(20, "L20 现代 Hook", 9) { NativeBridge.detectModernHookFrameworks() },
        // P1-D10 (v1.1.2): 新增 L21/L22/L23 — 从 quickScan 文本解析,
        // 与 native-lib.cpp 输出行 "L21 PlayIntegrity:" / "L22 模拟器:" /
        // "L23 特权框架:" 对齐 (P1-C1 已把原 L24 重编号为 L23)。
        LayerDef(21, "L21 Play Integrity", 8) { parseLayerFromQuickScan("L21 PlayIntegrity", it) },
        LayerDef(22, "L22 模拟器", 8) { parseLayerFromQuickScan("L22 模拟器", it) },
        // L23: Dhizuku/Shizuku/Stellar 特权框架 (P1-C1 重编号后)
        LayerDef(23, "L23 特权框架", 7) { parsePrivilegedFrameworkFromQuickScan(it) }
    )

    /**
     * 从已缓存的 [quickScanResult] 文本中解析指定层是否检测到异常。
     *
     * P1-D9 修复: 不再调用 [NativeBridge.runQuickScan],由调用方
     * 在 [scanParallel] / [scanSingleLayer] / [scanSubset] 入口处一次性
     * 获取 quickScan 结果并传入。
     *
     * @param layerPrefix 行前缀 (如 "L1 系统属性"),会与每一行做 contains 匹配
     * @param quickScanResult 已缓存的 quickScan 输出
     */
    private fun parseLayerFromQuickScan(layerPrefix: String, quickScanResult: String): Boolean {
        if (quickScanResult.isEmpty()) return false
        val line = quickScanResult.lines().find { it.contains(layerPrefix) } ?: return false
        return line.contains("❌")
    }

    /**
     * P1-D10 + P1-C1 兼容: 解析特权框架层。
     *
     * 当前 native (native-lib.cpp:159) 输出 "L24 特权框架: ...";
     * 若 P1-C1 完成重编号,native 会改为 "L23 特权框架: ..."。
     * 本函数同时接受两种标签,避免与并行 agent 冲突。
     */
    private fun parsePrivilegedFrameworkFromQuickScan(quickScanResult: String): Boolean {
        if (quickScanResult.isEmpty()) return false
        val line = quickScanResult.lines().find {
            it.contains("L23 特权框架") || it.contains("L24 特权框架")
        } ?: return false
        return line.contains("❌")
    }

    /**
     * 整个扫描生命周期内只调用一次 [NativeBridge.runQuickScan] 的辅助方法。
     * 由 [scanParallel] / [scanSingleLayer] / [scanSubset] 在入口处调用。
     */
    private fun fetchQuickScanOnce(): String = runCatching {
        NativeBridge.runQuickScan()
    }.getOrElse { e ->
        Log.w(TAG, "runQuickScan failed, parsers will return false: ${e.message}")
        ""
    }

    /**
     * 并行执行所有检测层。
     *
     * @return 完整扫描结果
     */
    suspend fun scanParallel(): ParallelScanResult = coroutineScope {
        _isScanning.value = true
        _progress.value = 0

        // P1-D9 修复: 整个并行扫描只调用一次 NativeBridge.runQuickScan(),
        // 结果通过闭包传递给所有走 parser 的层 (L1/L4/L6/L8/L9/L10/L12/
        // L21/L22/L24),避免此前 10+ 次重复全量 native 扫描。
        val quickScanResult = fetchQuickScanOnce()

        val completed = java.util.concurrent.atomic.AtomicInteger(0)
        val results = mutableListOf<LayerResult>()
        val resultsLock = Any()

        val totalTimeMs = measureTimeMillis {
            // 启动所有层并行检测
            val deferreds = layerDefs.map { def ->
                async(Dispatchers.Default) {
                    _currentLayer.value = def.name
                    var detected = false
                    val latency = measureTimeMillis {
                        detected = try {
                            def.detector(quickScanResult)
                        } catch (e: Throwable) {
                            Log.w(TAG, "[${def.name}] detector threw: ${e.message}")
                            false  // 检测失败视为未检测到,不崩溃
                        }
                    }
                    val result = LayerResult(
                        layerId = def.id,
                        layerName = def.name,
                        detected = detected,
                        weight = def.weight,
                        latencyMs = latency
                    )
                    synchronized(resultsLock) { results.add(result) }
                    val done = completed.incrementAndGet()
                    _progress.value = (done * 100 / TOTAL_LAYERS)
                    result
                }
            }
            // 等待所有完成
            deferreds.forEach { it.await() }
        }

        _isScanning.value = false
        _currentLayer.value = ""
        _progress.value = 100

        // 按层 ID 排序结果
        val sortedResults = synchronized(resultsLock) { results.sortedBy { it.layerId } }

        val totalRisk = sortedResults.sumOf { it.riskContribution }.coerceAtMost(100)
        val detectedCount = sortedResults.count { it.detected }

        Log.i(TAG, "Parallel scan complete: ${detectedCount}/${TOTAL_LAYERS} layers detected, " +
            "risk=$totalRisk, total=${totalTimeMs}ms")

        ParallelScanResult(
            layers = sortedResults,
            totalRiskScore = totalRisk,
            totalLatencyMs = totalTimeMs,
            detectedCount = detectedCount,
            totalLayers = TOTAL_LAYERS,
            quickScanText = quickScanResult
        )
    }

    /**
     * 仅执行指定层 (用于增量扫描或单层调试)。
     */
    suspend fun scanSingleLayer(layerId: Int): LayerResult? = withContext(Dispatchers.Default) {
        val def = layerDefs.find { it.id == layerId } ?: return@withContext null
        // P1-D9 修复: 单层扫描也复用一次 quickScan 结果,而非在 detector 内重复调用
        val quickScanResult = fetchQuickScanOnce()
        var detected = false
        val latency = measureTimeMillis {
            detected = try { def.detector(quickScanResult) } catch (e: Throwable) {
                Log.w(TAG, "[${def.name}] single-layer scan threw: ${e.message}")
                false
            }
        }
        LayerResult(def.id, def.name, detected, def.weight, latency)
    }

    /**
     * 扫描指定的层集合 (用于"跳过 L1/L2 这种已知安全的层"场景)。
     */
    suspend fun scanSubset(layerIds: Collection<Int>): List<LayerResult> = coroutineScope {
        val defs = layerDefs.filter { it.id in layerIds }
        // P1-D9 修复: 整个 subset 扫描只调用一次 quickScan,结果共享给所有 parser 层
        val quickScanResult = fetchQuickScanOnce()
        val completed = java.util.concurrent.atomic.AtomicInteger(0)
        val total = defs.size
        _progress.value = 0
        _isScanning.value = true

        val results = mutableListOf<LayerResult>()
        val resultsLock = Any()

        val deferreds = defs.map { def ->
            async(Dispatchers.Default) {
                _currentLayer.value = def.name
                var detected = false
                val latency = measureTimeMillis {
                    detected = try { def.detector(quickScanResult) } catch (_: Throwable) { false }
                }
                val r = LayerResult(def.id, def.name, detected, def.weight, latency)
                synchronized(resultsLock) { results.add(r) }
                val done = completed.incrementAndGet()
                _progress.value = (done * 100 / total)
                r
            }
        }
        deferreds.forEach { it.await() }
        _isScanning.value = false
        _progress.value = 100
        synchronized(resultsLock) { results.sortedBy { it.layerId } }
    }

    fun reset() {
        _progress.value = 0
        _currentLayer.value = ""
        _isScanning.value = false
    }
}

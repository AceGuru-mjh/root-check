package com.apex.root.domain.parallel

import android.content.Context
import android.util.Log
import com.apex.root.data.jni.NativeBridge
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * 设备指纹基线 + 增量检测 — v1.1.0 新增。
 *
 * 首次扫描时捕获设备的"指纹"(系统属性、挂载点、内存特征等),
 * 后续扫描与基线对比,只重新检测有变化的部分。
 *
 * 适用场景:
 *  - 用户首次安装后做一次完整扫描 → 建立基线
 *  - 后续扫描:对比基线,只重检变化项 → 大幅加速
 *  - Guard 实时监控:周期性对比基线,发现新增 root 痕迹立即告警
 *
 * 基线存储位置: context.filesDir/apex_baseline.json
 */
class DeviceFingerprintBaseline(private val context: Context) {

    companion object {
        private const val TAG = "Baseline"
        private const val BASELINE_FILE = "apex_baseline.json"
        private const val BASELINE_VERSION = 1
    }

    data class FingerprintSnapshot(
        val timestamp: Long,
        val systemProps: Map<String, String>,
        val memFingerprintMask: Int,
        val rwxPageCount: Int,
        val detectedLayerIds: Set<Int>,
        val riskScore: Int,
        val installedRootApps: List<String>
    ) {
        fun toJson(): JSONObject {
            return JSONObject().apply {
                put("version", BASELINE_VERSION)
                put("timestamp", timestamp)
                put("memFingerprintMask", memFingerprintMask)
                put("rwxPageCount", rwxPageCount)
                put("riskScore", riskScore)
                put("detectedLayerIds", JSONArray(detectedLayerIds))
                put("installedRootApps", JSONArray(installedRootApps))
                val propsObj = JSONObject()
                systemProps.forEach { (k, v) -> propsObj.put(k, v) }
                put("systemProps", propsObj)
            }
        }

        companion object {
            fun fromJson(json: JSONObject): FingerprintSnapshot? {
                return try {
                    val propsObj = json.optJSONObject("systemProps") ?: JSONObject()
                    val propsMap = mutableMapOf<String, String>()
                    propsObj.keys().forEach { k -> propsMap[k] = propsObj.getString(k) }

                    val layerIds = mutableSetOf<Int>()
                    val arr = json.optJSONArray("detectedLayerIds")
                    if (arr != null) for (i in 0 until arr.length()) layerIds.add(arr.getInt(i))

                    val rootApps = mutableListOf<String>()
                    val appsArr = json.optJSONArray("installedRootApps")
                    if (appsArr != null) for (i in 0 until appsArr.length()) rootApps.add(appsArr.getString(i))

                    FingerprintSnapshot(
                        timestamp = json.getLong("timestamp"),
                        systemProps = propsMap,
                        memFingerprintMask = json.getInt("memFingerprintMask"),
                        rwxPageCount = json.getInt("rwxPageCount"),
                        detectedLayerIds = layerIds,
                        riskScore = json.getInt("riskScore"),
                        installedRootApps = rootApps
                    )
                } catch (e: Exception) {
                    Log.e(TAG, "Failed to parse baseline JSON: ${e.message}")
                    null
                }
            }
        }
    }

    data class BaselineDiff(
        val newDetectedLayers: Set<Int>,    // 基线未检测到、现在检测到的层
        val removedDetectedLayers: Set<Int>,// 基线检测到、现在未检测到的层
        val riskScoreDelta: Int,            // 风险分变化
        val memFingerprintChanged: Boolean,
        val rwxPageCountDelta: Int,
        val newRootApps: List<String>,
        val timestamp: Long = System.currentTimeMillis()
    ) {
        val hasSignificantChanges: Boolean get() =
            newDetectedLayers.isNotEmpty() || newRootApps.isNotEmpty() ||
            (riskScoreDelta > 10) || (rwxPageCountDelta > 5)

        val summary: String get() = buildString {
            if (newDetectedLayers.isNotEmpty()) {
                append("新增检测到 ${newDetectedLayers.size} 层: ${newDetectedLayers.joinToString(",") { "L$it" }}\n")
            }
            if (removedDetectedLayers.isNotEmpty()) {
                append("已清除 ${removedDetectedLayers.size} 层: ${removedDetectedLayers.joinToString(",") { "L$it" }}\n")
            }
            if (riskScoreDelta != 0) {
                append("风险分变化: ${if (riskScoreDelta > 0) "+" else ""}$riskScoreDelta\n")
            }
            if (memFingerprintChanged) append("内存指纹已变化\n")
            if (rwxPageCountDelta != 0) {
                append("RWX 页数变化: ${if (rwxPageCountDelta > 0) "+" else ""}$rwxPageCountDelta\n")
            }
            if (newRootApps.isNotEmpty()) {
                append("新安装 root 应用: ${newRootApps.joinToString(", ")}\n")
            }
            if (!hasSignificantChanges) append("无显著变化")
        }
    }

    private val baselineFile: File get() = File(context.filesDir, BASELINE_FILE)

    /**
     * 捕获当前设备指纹快照。
     */
    fun captureSnapshot(scanResult: ParallelScanResult? = null): FingerprintSnapshot {
        // 读取关键系统属性
        val props = mutableMapOf<String, String>()
        listOf(
            "ro.build.fingerprint",
            "ro.build.version.release",
            "ro.build.version.security_patch",
            "ro.boot.verifiedbootstate",
            "ro.boot.flash.locked",
            "ro.boot.vbmeta.device_state",
            "ro.debuggable",
            "ro.secure",
            "ro.build.type"
        ).forEach { key ->
            // v1.0.1 修复: SystemProperties 是 @hide API,Kotlin 2.0 严格模式不可用
            // 改用反射读取 (兼容所有 Android 版本)
            val value = try {
                val cls = Class.forName("android.os.SystemProperties")
                val method = cls.getMethod("get", String::class.java, String::class.java)
                method.invoke(null, key, "") as? String ?: ""
            } catch (_: Throwable) { "" }
            if (value.isNotEmpty()) props[key] = value
        }

        val memMask = NativeBridge.fullMemoryFingerprint()
        val rwxCount = NativeBridge.countRWXPages()
        val riskScore = scanResult?.totalRiskScore ?: NativeBridge.getRiskScore()
        val detectedLayers = scanResult?.detectedLayers?.map { it.layerId }?.toSet()
            ?: emptySet()

        // 通过 quick scan 文本检测已安装的 root 应用
        val rootApps = mutableListOf<String>()
        val quickScan = NativeBridge.runQuickScan()
        if (quickScan.contains("L8 Magisk")) rootApps.add("Magisk")
        if (quickScan.contains("L9 KernelSU")) rootApps.add("KernelSU")
        if (quickScan.contains("L10 APatch")) rootApps.add("APatch")
        if (quickScan.contains("L17")) {
            if (NativeBridge.detectSukiSU()) rootApps.add("SukiSU")
            if (NativeBridge.detectMagiskDelta()) rootApps.add("MagiskDelta")
        }

        return FingerprintSnapshot(
            timestamp = System.currentTimeMillis(),
            systemProps = props,
            memFingerprintMask = memMask,
            rwxPageCount = rwxCount,
            detectedLayerIds = detectedLayers,
            riskScore = riskScore,
            installedRootApps = rootApps
        )
    }

    /**
     * 将快照保存为基线 (覆盖现有基线)。
     */
    fun saveBaseline(snapshot: FingerprintSnapshot): Boolean {
        return try {
            val json = snapshot.toJson()
            baselineFile.writeText(json.toString(2))
            Log.i(TAG, "Baseline saved: ${baselineFile.absolutePath}")
            true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to save baseline: ${e.message}")
            false
        }
    }

    /**
     * 加载已保存的基线。无基线返回 null。
     */
    fun loadBaseline(): FingerprintSnapshot? {
        if (!baselineFile.exists()) return null
        return try {
            val json = JSONObject(baselineFile.readText())
            FingerprintSnapshot.fromJson(json)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load baseline: ${e.message}")
            null
        }
    }

    /**
     * 对比当前快照与基线,返回差异。
     * 若无基线,返回 null (调用方应先建立基线)。
     */
    fun diff(current: FingerprintSnapshot): BaselineDiff? {
        val baseline = loadBaseline() ?: return null
        return BaselineDiff(
            newDetectedLayers = current.detectedLayerIds - baseline.detectedLayerIds,
            removedDetectedLayers = baseline.detectedLayerIds - current.detectedLayerIds,
            riskScoreDelta = current.riskScore - baseline.riskScore,
            memFingerprintChanged = current.memFingerprintMask != baseline.memFingerprintMask,
            rwxPageCountDelta = current.rwxPageCount - baseline.rwxPageCount,
            newRootApps = current.installedRootApps - baseline.installedRootApps.toSet()
        )
    }

    /**
     * 是否已建立基线。
     */
    fun hasBaseline(): Boolean = baselineFile.exists()

    /**
     * 清除基线 (用户主动重置)。
     */
    fun clearBaseline() {
        if (baselineFile.exists()) baselineFile.delete()
    }
}

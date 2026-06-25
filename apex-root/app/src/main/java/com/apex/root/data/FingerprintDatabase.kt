package com.apex.root.data

import android.content.Context
import android.util.Log
import com.apex.root.data.jni.NativeBridge
import java.io.ByteArrayOutputStream
import java.io.InputStream
import java.security.SecureRandom
import javax.crypto.Cipher
import javax.crypto.spec.GCMParameterSpec
import javax.crypto.spec.SecretKeySpec

data class FingerprintEntry(
    val type: String,
    val patterns: List<String>,
    val baselineNs: Long = 0,
    val thresholdSigma: Double = 0.0
)

class FingerprintDatabase(private val context: Context) {

    companion object {
        private const val TAG = "FingerprintDB"
        private const val DB_ASSET_PATH = "secure_fingerprint/fingerprint_db.enc"
        private const val DB_ASSET_KEY_PATH = "secure_fingerprint/fingerprint_db.key"
        private const val GCM_TAG_LENGTH = 128 // bits

        private var instance: FingerprintDatabase? = null
        fun getInstance(context: Context): FingerprintDatabase {
            if (instance == null) {
                instance = FingerprintDatabase(context.applicationContext)
            }
            return instance!!
        }
    }

    private var entries: List<FingerprintEntry> = emptyList()
    private var loaded = false

    fun load(): Boolean {
        if (loaded) return true
        try {
            val encrypted = readAssetBytes(DB_ASSET_PATH)
            if (encrypted == null) {
                Log.w(TAG, "Encrypted DB not found, loading plaintext fallback")
                return loadPlaintextFallback()
            }

            val deviceKey = deriveDatabaseKey()
            val plaintext = aes256GcmDecrypt(encrypted, deviceKey)
            if (plaintext == null) {
                Log.e(TAG, "Failed to decrypt fingerprint DB, key mismatch or corruption")
                return false
            }

            entries = parseDatabase(plaintext.decodeToString())
            loaded = true
            Log.i(TAG, "Fingerprint DB loaded: ${entries.size} entries")
            return true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load fingerprint DB: ${e.message}")
            return loadPlaintextFallback()
        }
    }

    fun getEntries(): List<FingerprintEntry> = entries

    fun getPatternsByType(type: String): List<String> {
        return entries.find { it.type == type }?.patterns ?: emptyList()
    }

    fun getBaselineNs(): Long = entries.find { it.type == "syscall_latency_baseline" }?.baselineNs ?: 1500

    fun getThresholdSigma(): Double = entries.find { it.type == "syscall_latency_baseline" }?.thresholdSigma ?: 3.5

    fun isLoaded(): Boolean = loaded

    /**
     * Encrypt a plaintext YAML database and store as asset.
     * Call this from a build tool to generate the encrypted DB.
     */
    fun encryptDatabase(plaintextYaml: String, outputPath: String): Boolean {
        try {
            val deviceKey = deriveDatabaseKey()
            val encrypted = aes256GcmEncrypt(plaintextYaml.encodeToByteArray(), deviceKey)
            java.io.File(outputPath).writeBytes(encrypted)
            Log.i(TAG, "Encrypted DB written to $outputPath (${encrypted.size} bytes)")
            return true
        } catch (e: Exception) {
            Log.e(TAG, "Encryption failed: ${e.message}")
            return false
        }
    }

    private fun deriveDatabaseKey(): ByteArray {
        val deviceId = NativeBridge.getDeviceIdentifier() ?: "apex-root-default"
        val seed = deviceId.encodeToByteArray()
        val hash = NativeBridge.sha3_512(seed)
        val key = ByteArray(32)
        hash.copyInto(key, 0, 0, 32)
        return key
    }

    private fun aes256GcmEncrypt(plaintext: ByteArray, key: ByteArray): ByteArray {
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        val spec = SecretKeySpec(key, "AES")
        cipher.init(Cipher.ENCRYPT_MODE, spec)
        val iv = cipher.iv
        val ciphertext = cipher.doFinal(plaintext)
        val out = ByteArrayOutputStream()
        out.write(iv)
        out.write(ciphertext)
        return out.toByteArray()
    }

    private fun aes256GcmDecrypt(encrypted: ByteArray, key: ByteArray): ByteArray? {
        return try {
            val cipher = Cipher.getInstance("AES/GCM/NoPadding")
            val spec = SecretKeySpec(key, "AES")
            val gcmSpec = GCMParameterSpec(GCM_TAG_LENGTH, encrypted, 0, 12)
            cipher.init(Cipher.DECRYPT_MODE, spec, gcmSpec)
            cipher.doFinal(encrypted, 12, encrypted.size - 12)
        } catch (e: Exception) {
            Log.e(TAG, "Decryption failed: ${e.message}")
            null
        }
    }

    private fun readAssetBytes(path: String): ByteArray? {
        return try {
            context.assets.open(path).use { input ->
                val baos = ByteArrayOutputStream()
                val buf = ByteArray(4096)
                var n: Int
                while (input.read(buf).also { n = it } != -1) {
                    baos.write(buf, 0, n)
                }
                baos.toByteArray()
            }
        } catch (_: Exception) {
            null
        }
    }

    private fun loadPlaintextFallback(): Boolean {
        try {
            val yaml = context.assets.open("secure_fingerprint/fingerprint_db.yaml")
                .bufferedReader().readText()
            entries = parseDatabase(yaml)
            loaded = true
            Log.i(TAG, "Plaintext fallback loaded: ${entries.size} entries")
            return true
        } catch (e: Exception) {
            Log.e(TAG, "Plaintext fallback also failed: ${e.message}")
            return false
        }
    }

    private fun parseDatabase(yaml: String): List<FingerprintEntry> {
        val result = mutableListOf<FingerprintEntry>()
        val lines = yaml.lines()

        var currentType = ""
        var currentPatterns = mutableListOf<String>()
        var currentBaseline = 0L
        var currentSigma = 0.0
        var inEntries = false
        var inPatterns = false

        for (line in lines) {
            val trimmed = line.trim()
            if (trimmed.startsWith("entries:")) {
                if (currentType.isNotEmpty() && currentPatterns.isNotEmpty()) {
                    result.add(FingerprintEntry(currentType, currentPatterns.toList(), currentBaseline, currentSigma))
                }
                inEntries = true
                currentType = ""
                currentPatterns = mutableListOf()
                continue
            }
            if (!inEntries) continue

            when {
                trimmed.startsWith("- type:") -> {
                    if (currentType.isNotEmpty() && currentPatterns.isNotEmpty()) {
                        result.add(FingerprintEntry(currentType, currentPatterns.toList(), currentBaseline, currentSigma))
                    }
                    currentType = trimmed.substringAfter("type:").trim().removeSurrounding("\"")
                    currentPatterns = mutableListOf()
                    currentBaseline = 0
                    currentSigma = 0.0
                    inPatterns = false
                }
                trimmed == "patterns:" -> inPatterns = true
                trimmed.startsWith("- ") && inPatterns -> {
                    val p = trimmed.substringAfter("- ").trim().removeSurrounding("\"")
                    currentPatterns.add(p)
                }
                trimmed.startsWith("baseline_ns:") -> {
                    currentBaseline = trimmed.substringAfter(":").trim().toLongOrNull() ?: 0
                }
                trimmed.startsWith("threshold_sigma:") -> {
                    currentSigma = trimmed.substringAfter(":").trim().toDoubleOrNull() ?: 0.0
                }
            }
        }
        if (currentType.isNotEmpty() && currentPatterns.isNotEmpty()) {
            result.add(FingerprintEntry(currentType, currentPatterns.toList(), currentBaseline, currentSigma))
        }
        return result
    }
}

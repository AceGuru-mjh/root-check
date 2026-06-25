package com.rootdetector.util

import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest
import java.security.SecureRandom
import java.util.zip.ZipEntry
import java.util.zip.ZipInputStream
import javax.crypto.Cipher
import javax.crypto.SecretKeyFactory
import javax.crypto.spec.IvParameterSpec
import javax.crypto.spec.PBEKeySpec
import javax.crypto.spec.SecretKeySpec

/**
 * APK 加壳保护工具类
 * 
 * 实现 DEX 加密/解密、运行时解密和完整性校验
 */
object ApkProtector {
    
    private const val TAG = "ApkProtector"
    private const val ENCRYPTED_DEX_DIR = "encrypted_dex"
    private const val DECRYPTED_DEX_DIR = "decrypted_dex"
    private const val KEY_FILE = "key.dat"
    private const val IV_FILE = "iv.dat"
    private const val SIGNATURE_FILE = "signature.dat"
    
    // 加密密钥（实际应用中应从安全存储获取）
    private const val KEY_PASSWORD = "RootDetector_Secure_Key_2024"
    private const val SALT = "RootDetector_Salt_Value"
    private const val ITERATION_COUNT = 65536
    private const val KEY_LENGTH = 256
    
    /**
     * 初始化加壳保护
     * 在 Application.onCreate() 中调用
     */
    fun initialize(context: Context) {
        try {
            // 1. 验证 APK 完整性
            if (!verifyApkIntegrity(context)) {
                throw SecurityException("APK integrity verification failed")
            }
            
            // 2. 检查是否已解密
            val decryptedDir = File(context.filesDir, DECRYPTED_DEX_DIR)
            if (!decryptedDir.exists() || decryptedDir.listFiles()?.isEmpty() == true) {
                // 3. 解密 DEX 文件
                decryptDexFiles(context)
            }
            
            // 4. 加载解密后的 DEX
            loadDecryptedDex(context)
            
        } catch (e: Exception) {
            // 安全检查失败，终止应用
            android.util.Log.e(TAG, "Protection initialization failed", e)
            throw RuntimeException("Security check failed", e)
        }
    }
    
    /**
     * 加密 DEX 文件（构建时使用）
     * 
     * @param apkPath 原始 APK 路径
     * @param outputPath 输出加密后的 APK 路径
     */
    fun encryptDexFiles(apkPath: String, outputPath: String) {
        val tempDir = File(System.getProperty("java.io.tmpdir"), "dex_encrypt_${System.currentTimeMillis()}")
        tempDir.mkdirs()
        
        try {
            // 1. 提取 DEX 文件
            val dexFiles = extractDexFromApk(apkPath, tempDir)
            
            // 2. 生成加密密钥
            val key = generateKey()
            val iv = generateIV()
            
            // 3. 加密每个 DEX 文件
            dexFiles.forEach { dexFile ->
                val encryptedFile = File(tempDir, "${dexFile.name}.enc")
                encryptFile(dexFile, encryptedFile, key, iv)
                dexFile.delete()
            }
            
            // 4. 保存密钥和 IV
            saveKeyAndIV(tempDir, key, iv)
            
            // 5. 重新打包 APK
            repackApk(apkPath, outputPath, tempDir)
            
            // 6. 计算并保存签名
            val signature = calculateApkSignature(outputPath)
            saveSignature(tempDir, signature)
            
        } finally {
            tempDir.deleteRecursively()
        }
    }
    
    /**
     * 解密 DEX 文件（运行时使用）
     */
    private fun decryptDexFiles(context: Context) {
        val encryptedDir = File(context.filesDir, ENCRYPTED_DEX_DIR)
        val decryptedDir = File(context.filesDir, DECRYPTED_DEX_DIR)
        
        if (!encryptedDir.exists()) {
            // 首次运行，从 APK 中提取并加密
            prepareEncryptedDex(context, encryptedDir)
        }
        
        decryptedDir.mkdirs()
        
        // 加载密钥和 IV
        val (key, iv) = loadKeyAndIV(context)
        
        // 解密所有加密的 DEX 文件
        encryptedDir.listFiles()?.filter { it.name.endsWith(".enc") }?.forEach { encFile ->
            val dexName = encFile.nameWithoutExtension
            val decryptedFile = File(decryptedDir, dexName)
            decryptFile(encFile, decryptedFile, key, iv)
        }
    }
    
    /**
     * 从 APK 中提取 DEX 文件并加密
     */
    private fun prepareEncryptedDex(context: Context, outputDir: File) {
        outputDir.mkdirs()
        
        val apkPath = context.applicationInfo.sourceDir
        val tempDir = File(context.cacheDir, "dex_extract_${System.currentTimeMillis()}")
        tempDir.mkdirs()
        
        try {
            // 提取 DEX 文件
            val dexFiles = extractDexFromApk(apkPath, tempDir)
            
            // 生成密钥
            val key = generateKey()
            val iv = generateIV()
            
            // 加密并保存
            dexFiles.forEach { dexFile ->
                val encryptedFile = File(outputDir, "${dexFile.name}.enc")
                encryptFile(dexFile, encryptedFile, key, iv)
            }
            
            // 保存密钥和 IV
            saveKeyAndIV(context, key, iv)
            
        } finally {
            tempDir.deleteRecursively()
        }
    }
    
    /**
     * 加载解密后的 DEX 文件
     */
    private fun loadDecryptedDex(context: Context) {
        val decryptedDir = File(context.filesDir, DECRYPTED_DEX_DIR)
        
        decryptedDir.listFiles()?.filter { it.name.endsWith(".dex") }?.forEach { dexFile ->
            try {
                // 使用 DexClassLoader 加载
                val dexClassLoader = dalvik.system.DexClassLoader(
                    dexFile.absolutePath,
                    context.codeCacheDir.absolutePath,
                    null,
                    context.classLoader
                )
                
                // 这里可以添加类加载逻辑
                android.util.Log.d(TAG, "Loaded DEX: ${dexFile.name}")
                
            } catch (e: Exception) {
                android.util.Log.e(TAG, "Failed to load DEX: ${dexFile.name}", e)
            }
        }
    }
    
    /**
     * 验证 APK 完整性
     */
    fun verifyApkIntegrity(context: Context): Boolean {
        return try {
            val apkPath = context.applicationInfo.sourceDir
            val currentSignature = calculateApkSignature(apkPath)
            
            // 获取期望的签名（从签名文件或其他安全存储）
            val expectedSignature = getExpectedSignature(context)
            
            currentSignature == expectedSignature
        } catch (e: Exception) {
            android.util.Log.e(TAG, "Integrity verification failed", e)
            false
        }
    }
    
    /**
     * 验证签名证书
     */
    fun verifySignatureCertificate(context: Context): Boolean {
        return try {
            val packageInfo = context.packageManager.getPackageInfo(
                context.packageName,
                PackageManager.GET_SIGNATURES
            )
            
            val signatures = packageInfo.signatures
            if (signatures == null || signatures.isEmpty()) {
                return false
            }
            
            // 验证签名证书指纹
            val certFingerprint = getCertificateFingerprint(signatures[0])
            val expectedFingerprint = getExpectedCertificateFingerprint(context)
            
            certFingerprint == expectedFingerprint
        } catch (e: Exception) {
            android.util.Log.e(TAG, "Certificate verification failed", e)
            false
        }
    }
    
    /**
     * 检测调试器
     */
    fun detectDebugger(): Boolean {
        return try {
            // 检查 TracerPID
            val statusFile = File("/proc/self/status")
            if (statusFile.exists()) {
                val lines = statusFile.readLines()
                val tracerPidLine = lines.find { it.startsWith("TracerPid:") }
                if (tracerPidLine != null) {
                    val tracerPid = tracerPidLine.substringAfter(":").trim().toIntOrNull() ?: 0
                    if (tracerPid != 0) {
                        return true
                    }
                }
            }
            
            // 检查是否可调试
            if (android.os.Debug.isDebuggerConnected()) {
                return true
            }
            
            false
        } catch (e: Exception) {
            android.util.Log.e(TAG, "Debugger detection failed", e)
            false
        }
    }
    
    /**
     * 检测模拟器
     */
    fun detectEmulator(): Boolean {
        return try {
            val score = calculateEmulatorScore()
            score >= 3 // 阈值：3 分以上认为是模拟器
        } catch (e: Exception) {
            android.util.Log.e(TAG, "Emulator detection failed", e)
            false
        }
    }
    
    /**
     * 计算模拟器检测分数
     */
    private fun calculateEmulatorScore(): Int {
        var score = 0
        
        // 检查 Build 信息
        if (Build.FINGERPRINT.contains("generic") || 
            Build.FINGERPRINT.contains("unknown")) {
            score++
        }
        
        if (Build.MODEL.contains("google_sdk") || 
            Build.MODEL.contains("Emulator") ||
            Build.MODEL.contains("Android SDK")) {
            score++
        }
        
        if (Build.MANUFACTURER.contains("Genymotion") ||
            Build.MANUFACTURER.contains("unknown")) {
            score++
        }
        
        if (Build.BRAND.startsWith("generic") && Build.DEVICE.startsWith("generic")) {
            score++
        }
        
        if ("google_sdk" == Build.PRODUCT) {
            score++
        }
        
        // 检查硬件特征
        val cpuAbi = Build.SUPPORTED_ABIS
        if (cpuAbi.size == 1 && cpuAbi[0] == "x86") {
            score++
        }
        
        return score
    }
    
    // ==================== 辅助方法 ====================
    
    private fun extractDexFromApk(apkPath: String, outputDir: File): List<File> {
        val dexFiles = mutableListOf<File>()
        val zipInputStream = ZipInputStream(FileInputStream(apkPath))
        
        var entry: ZipEntry? = zipInputStream.nextEntry
        while (entry != null) {
            if (entry.name.endsWith(".dex")) {
                val dexFile = File(outputDir, entry.name)
                FileOutputStream(dexFile).use { fos ->
                    zipInputStream.copyTo(fos)
                }
                dexFiles.add(dexFile)
            }
            entry = zipInputStream.nextEntry
        }
        
        zipInputStream.close()
        return dexFiles
    }
    
    private fun generateKey(): SecretKeySpec {
        val factory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256")
        val spec = PBEKeySpec(KEY_PASSWORD.toCharArray(), SALT.toByteArray(), ITERATION_COUNT, KEY_LENGTH)
        val secretKey = factory.generateSecret(spec)
        return SecretKeySpec(secretKey.encoded, "AES")
    }
    
    private fun generateIV(): IvParameterSpec {
        val iv = ByteArray(16)
        SecureRandom().nextBytes(iv)
        return IvParameterSpec(iv)
    }
    
    private fun encryptFile(input: File, output: File, key: SecretKeySpec, iv: IvParameterSpec) {
        val cipher = Cipher.getInstance("AES/CBC/PKCS5Padding")
        cipher.init(Cipher.ENCRYPT_MODE, key, iv)
        
        FileInputStream(input).use { fis ->
            FileOutputStream(output).use { fos ->
                // 写入 IV 到文件头部
                fos.write(iv.iv)
                
                val buffer = ByteArray(8192)
                var bytesRead: Int
                while (fis.read(buffer).also { bytesRead = it } != -1) {
                    val encrypted = cipher.update(buffer, 0, bytesRead)
                    if (encrypted != null) {
                        fos.write(encrypted)
                    }
                }
                
                val finalBlock = cipher.doFinal()
                if (finalBlock != null) {
                    fos.write(finalBlock)
                }
            }
        }
    }
    
    private fun decryptFile(input: File, output: File, key: SecretKeySpec, iv: IvParameterSpec) {
        val cipher = Cipher.getInstance("AES/CBC/PKCS5Padding")
        cipher.init(Cipher.DECRYPT_MODE, key, iv)
        
        FileInputStream(input).use { fis ->
            // 跳过 IV（前 16 字节）
            val ivBuffer = ByteArray(16)
            fis.read(ivBuffer)
            
            FileOutputStream(output).use { fos ->
                val buffer = ByteArray(8192)
                var bytesRead: Int
                while (fis.read(buffer).also { bytesRead = it } != -1) {
                    val decrypted = cipher.update(buffer, 0, bytesRead)
                    if (decrypted != null) {
                        fos.write(decrypted)
                    }
                }
                
                val finalBlock = cipher.doFinal()
                if (finalBlock != null) {
                    fos.write(finalBlock)
                }
            }
        }
    }
    
    private fun saveKeyAndIV(context: Context, key: SecretKeySpec, iv: IvParameterSpec) {
        val keyFile = File(context.filesDir, KEY_FILE)
        val ivFile = File(context.filesDir, IV_FILE)
        
        FileOutputStream(keyFile).use { it.write(key.encoded) }
        FileOutputStream(ivFile).use { it.write(iv.iv) }
    }
    
    private fun saveKeyAndIV(dir: File, key: SecretKeySpec, iv: IvParameterSpec) {
        val keyFile = File(dir, KEY_FILE)
        val ivFile = File(dir, IV_FILE)
        
        FileOutputStream(keyFile).use { it.write(key.encoded) }
        FileOutputStream(ivFile).use { it.write(iv.iv) }
    }
    
    private fun loadKeyAndIV(context: Context): Pair<SecretKeySpec, IvParameterSpec> {
        val keyFile = File(context.filesDir, KEY_FILE)
        val ivFile = File(context.filesDir, IV_FILE)
        
        val keyBytes = FileInputStream(keyFile).use { it.readBytes() }
        val ivBytes = FileInputStream(ivFile).use { it.readBytes() }
        
        val key = SecretKeySpec(keyBytes, "AES")
        val iv = IvParameterSpec(ivBytes)
        
        return Pair(key, iv)
    }
    
    private fun repackApk(originalApk: String, outputApk: String, modifiedDir: File) {
        // 这里实现 APK 重新打包逻辑
        // 实际应用中需要使用 ZipOutputStream 重新打包
        // 简化实现：复制原始 APK 并替换加密的 DEX
        File(originalApk).copyTo(File(outputApk), overwrite = true)
    }
    
    private fun calculateApkSignature(apkPath: String): String {
        val digest = MessageDigest.getInstance("SHA-256")
        FileInputStream(apkPath).use { fis ->
            val buffer = ByteArray(8192)
            var bytesRead: Int
            while (fis.read(buffer).also { bytesRead = it } != -1) {
                digest.update(buffer, 0, bytesRead)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }
    
    private fun saveSignature(dir: File, signature: String) {
        val signatureFile = File(dir, SIGNATURE_FILE)
        signatureFile.writeText(signature)
    }
    
    private fun getExpectedSignature(context: Context): String {
        // 从安全存储获取期望的签名
        // 实际应用中应该从安全存储（如 Keystore）获取
        val signatureFile = File(context.filesDir, SIGNATURE_FILE)
        return if (signatureFile.exists()) {
            signatureFile.readText()
        } else {
            ""
        }
    }
    
    private fun getCertificateFingerprint(signature: android.content.pm.Signature): String {
        val digest = MessageDigest.getInstance("SHA-256")
        val certBytes = signature.toByteArray()
        return digest.digest(certBytes).joinToString("") { "%02x".format(it) }
    }
    
    private fun getExpectedCertificateFingerprint(context: Context): String {
        // 从安全存储获取期望的证书指纹
        // 实际应用中应该硬编码或从安全存储获取
        return ""
    }
}

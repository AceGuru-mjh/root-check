package com.apex.root.ipc.client

import android.net.LocalSocket
import android.net.LocalSocketAddress
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.*
import java.io.*
import java.nio.ByteBuffer
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicLong
import android.util.Log

/**
 * 修复版 SecureSocketClient
 *
 * 修复内容：
 * 1. 移除 runBlocking — 改为 suspend fun，避免主线程 ANR
 * 2. 添加自动重连机制（指数退避，最多 5 次）
 * 3. 正确管理 CoroutineScope 生命周期
 * 4. 连接断开时自动清理资源
 * 5. P0-K2 修复 (v1.1.1): reader loop 与 send() 均强制使用 Dispatchers.IO,
 *    防止调用方 (如 ScanViewModel.viewModelScope, 默认 Dispatchers.Main.immediate)
 *    把阻塞 I/O 调度到主线程导致 ANR。
 *
 * ⚠️ SECURITY WARNING (P2-K2): 类名 "Secure" 名不副实
 *
 * 当前实现是 hex 编码的明文 over Unix domain socket, 无 TLS/MAC/peer auth。
 * "Secure" 仅指 Unix socket 的隐式信任 (同 UID 进程可连接), 不提供加密。
 * 任何能 bind/connect 到该 Unix socket 的同 UID 进程均可读写消息内容、
 * 注入伪造帧或重放历史帧。
 *
 * 若需真正的安全通信, 应:
 * 1. 用 NaCl/secretbox + 预共享密钥加密 ( 推荐, 自包含 )
 * 2. 或用 Android Binder + signature-level permission ( 系统 API, 进程级鉴权 )
 * 3. 或用 TLS + 证书 pinning ( 适合跨设备, 但 Unix socket 场景过重 )
 *
 * 当前调用方 (DetectionProtocol / ScanViewModel) 仅在 app 自身进程内使用,
 * 攻击面为同 UID 注入进程 (需已 compromise 同 sandbox), 风险中等。
 * 计划在 v1.2.0 重命名为 LocalSocketClient 或增加真正的加密层。
 */
class SecureSocketClient(
    private val socketName: String,
    private val scope: CoroutineScope,
    private val dispatcher: CoroutineDispatcher = Dispatchers.IO
) {
    companion object {
        private const val TAG = "SecureSocketClient"
        private const val MAX_RECONNECT_ATTEMPTS = 5
        private const val INITIAL_BACKOFF_MS = 1000L
    }

    private var socket: LocalSocket? = null
    private var reader: BufferedReader? = null
    private var writer: BufferedWriter? = null
    private val sequenceNumber = AtomicLong(0)
    private val messageQueue = ConcurrentLinkedQueue<ByteArray>()

    // P0-K2 修复 (v1.1.1): clientScope 显式叠加 Dispatchers.IO, 不再继承调用方的 scope.dispatcher。
    // 之前 clientScope = CoroutineScope(scope.coroutineContext + SupervisorJob()) 会继承
    // viewModelScope 的 Dispatchers.Main.immediate, 导致 readLine/write 阻塞主线程 → ANR。
    private val clientScope = CoroutineScope(scope.coroutineContext + SupervisorJob() + Dispatchers.IO)
    private var readerJob: Job? = null
    private var isRunning = false

    private val _messages = MutableSharedFlow<ByteArray>(replay = 0, extraBufferCapacity = 64)
    val messages: SharedFlow<ByteArray> = _messages.asSharedFlow()

    private val _connectionState = MutableStateFlow(false)
    val connectionState: StateFlow<Boolean> = _connectionState.asStateFlow()

    /**
     * 连接到 socket（suspend，不在主线程阻塞）
     */
    suspend fun connect(): Boolean = withContext(Dispatchers.IO) {
        // P0-K2 修复 (v1.1.1): 与 send() 一致, 强制 IO 调度器。
        try {
            val localSocket = LocalSocket()
            val address = LocalSocketAddress(socketName, LocalSocketAddress.Namespace.RESERVED)
            localSocket.connect(address)
            socket = localSocket
            reader = BufferedReader(InputStreamReader(localSocket.inputStream))
            writer = BufferedWriter(OutputStreamWriter(localSocket.outputStream))
            isRunning = true
            _connectionState.value = true
            startReader()
            Log.i(TAG, "Connected to $socketName")
            true
        } catch (e: Exception) {
            Log.e(TAG, "Connection failed: ${e.message}")
            _connectionState.value = false
            false
        }
    }

    /**
     * 自动重连（指数退避）
     */
    suspend fun reconnectWithBackoff(): Boolean {
        var backoff = INITIAL_BACKOFF_MS
        repeat(MAX_RECONNECT_ATTEMPTS) { attempt ->
            Log.i(TAG, "Reconnect attempt ${attempt + 1}/$MAX_RECONNECT_ATTEMPTS (backoff=${backoff}ms)")
            delay(backoff)
            cleanup()
            if (connect()) return true
            backoff *= 2 // 指数退避
        }
        Log.e(TAG, "Reconnect failed after $MAX_RECONNECT_ATTEMPTS attempts")
        return false
    }

    private fun startReader() {
        // P0-K2 修复 (v1.1.1): launch 显式指定 Dispatchers.IO, readLine() 是阻塞 I/O 不能跑在 Main。
        readerJob = clientScope.launch(Dispatchers.IO) {
            while (isRunning && isActive) {
                try {
                    val line = reader?.readLine() ?: break
                    val data = decodeHex(line)
                    _messages.tryEmit(data)
                } catch (e: IOException) {
                    Log.e(TAG, "Reader error: ${e.message}")
                    _connectionState.value = false
                    break
                }
            }
            // 读取结束 — 尝试自动重连
            if (isRunning) {
                Log.w(TAG, "Connection lost, attempting reconnect...")
                clientScope.launch(Dispatchers.IO) { reconnectWithBackoff() }
            }
        }
    }

    suspend fun send(data: ByteArray): Boolean = withContext(Dispatchers.IO) {
        // P0-K2 修复 (v1.1.1): 强制 Dispatchers.IO, 不再使用构造函数的 dispatcher 参数,
        // 防止调用方在构造 SecureSocketClient 时传入 Main 调度器导致 write/flush 阻塞主线程。
        try {
            val seq = sequenceNumber.incrementAndGet()
            val header = ByteBuffer.allocate(12).putLong(seq).putInt(data.size).array()
            val payload = header + data
            writer?.write(encodeHex(payload))
            writer?.newLine()
            writer?.flush()
            true
        } catch (e: Exception) {
            Log.e(TAG, "Send error: ${e.message}")
            _connectionState.value = false
            false
        }
    }

    /**
     * 断开连接（suspend，不阻塞主线程）
     */
    suspend fun disconnect() {
        isRunning = false
        readerJob?.cancelAndJoin()
        cleanup()
        clientScope.cancel()
    }

    /**
     * 非挂起的立即关闭（用于 onCleared 等不能调用 suspend 的场景）。
     * 同步关闭 socket / reader / writer，并取消 reader job（不等待 join）。
     */
    // v1.0.2 P0-2 修复: 提供 closeAndJoin() 让调用方等待协程真正结束
    fun closeNow() {
        isRunning = false
        readerJob?.cancel()
        cleanup()
        clientScope.cancel()
    }

    /**
     * v1.0.2: 关闭并等待 readerJob 真正结束,避免资源泄漏。
     * closeNow() 只 cancel 不 join,ViewModel 销毁后协程可能仍在运行。
     */
    suspend fun closeAndJoin() {
        isRunning = false
        try {
            readerJob?.cancelAndJoin()
        } catch (_: Throwable) {}
        cleanup()
        clientScope.cancel()
    }

    private fun cleanup() {
        try { socket?.close() } catch (_: Exception) {}
        try { reader?.close() } catch (_: Exception) {}
        try { writer?.close() } catch (_: Exception) {}
        socket = null
        reader = null
        writer = null
        _connectionState.value = false
    }

    private fun encodeHex(data: ByteArray): String =
        data.joinToString("") { "%02x".format(it) }

    private fun decodeHex(hex: String): ByteArray {
        // 修复：原 it.toInt(16) 在非法输入时抛 NumberFormatException（RuntimeException），
        // 逃逸了 startReader 里的 catch(IOException) → reader 协程崩溃。
        // 改为返回空数组，让上层跳过该帧。
        if (hex.length % 2 != 0) return ByteArray(0)
        return try {
            hex.chunked(2).map { it.toInt(16).toByte() }.toByteArray()
        } catch (_: NumberFormatException) {
            ByteArray(0)
        }
    }
}

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

class SecureSocketClient(
    private val socketName: String,
    private val dispatcher: CoroutineDispatcher = Dispatchers.IO
) {
    companion object {
        private const val TAG = "SecureSocketClient"
    }

    private var socket: LocalSocket? = null
    private var reader: BufferedReader? = null
    private var writer: BufferedWriter? = null
    private val sequenceNumber = AtomicLong(0)
    private val messageQueue = ConcurrentLinkedQueue<ByteArray>()
    private var isRunning = false
    private var job: Job? = null

    private val _messages = MutableSharedFlow<ByteArray>(replay = 0, extraBufferCapacity = 64)
    val messages: SharedFlow<ByteArray> = _messages.asSharedFlow()

    private val _connectionState = MutableStateFlow(false)
    val connectionState: StateFlow<Boolean> = _connectionState.asStateFlow()

    fun connect(): Boolean = runBlocking {
        withContext(dispatcher) {
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
    }

    private fun startReader() {
        job = CoroutineScope(dispatcher + SupervisorJob()).launch {
            while (isRunning) {
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
        }
    }

    suspend fun send(data: ByteArray): Boolean {
        return withContext(dispatcher) {
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
    }

    fun disconnect() {
        isRunning = false
        job?.cancel()
        runBlocking {
            withContext(dispatcher) {
                try {
                    socket?.close()
                } catch (_: Exception) {}
            }
        }
    }

    private fun encodeHex(data: ByteArray): String =
        data.joinToString("") { "%02x".format(it) }

    private fun decodeHex(hex: String): ByteArray =
        hex.chunked(2).map { it.toInt(16).toByte() }.toByteArray()
}

package com.rootdetector.data.datasource

interface NativeDataSource {
    fun startDaemon(): Boolean
    fun runDetection(layerMask: Int): String?
    fun quickDetect(): String?
}

class NativeDataSourceImpl(
    private val nativeStartDaemon: () -> Boolean,
    private val nativeRunDetection: (Int) -> String?,
    private val nativeQuickDetect: () -> String?
) : NativeDataSource {
    override fun startDaemon(): Boolean = nativeStartDaemon()
    override fun runDetection(layerMask: Int): String? = nativeRunDetection(layerMask)
    override fun quickDetect(): String? = nativeQuickDetect()
}
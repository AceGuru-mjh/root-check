package com.rootdetector.util

import com.google.gson.GsonBuilder
import com.rootdetector.model.DetectionReport
import java.io.File

object JsonExporter {
    
    private val gson = GsonBuilder()
        .setPrettyPrinting()
        .create()
    
    fun exportToJson(report: DetectionReport): String {
        return gson.toJson(report)
    }
    
    fun exportToFile(report: DetectionReport, file: File) {
        val json = exportToJson(report)
        file.writeText(json)
    }
    
    fun importFromJson(json: String): DetectionReport {
        return gson.fromJson(json, DetectionReport::class.java)
    }
}

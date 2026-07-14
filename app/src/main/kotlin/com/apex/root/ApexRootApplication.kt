package com.apex.root

import android.app.Application
import com.apex.root.data.database.ApexDatabase
import com.apex.root.data.local.SettingsDataSource
import com.apex.root.data.repository.GuardEventRepository
import com.apex.root.data.repository.ScanRepository
import com.apex.root.data.repository.SettingsRepository

/**
 * Application 类 - 提供全局依赖
 */
class ApexRootApplication : Application() {
    
    // 数据库实例
    val database: ApexDatabase by lazy {
        ApexDatabase.getDatabase(this)
    }
    
    // 数据源
    val settingsDataSource: SettingsDataSource by lazy {
        SettingsDataSource(this)
    }
    
    // 仓库
    val settingsRepository: SettingsRepository by lazy {
        SettingsRepository(settingsDataSource)
    }
    
    val guardEventRepository: GuardEventRepository by lazy {
        GuardEventRepository(database.guardEventDao())
    }
    
    val scanRepository: ScanRepository by lazy {
        ScanRepository(this)
    }
}

package com.apex.root.data.database

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase

/**
 * App 数据库
 */
@Database(
    entities = [GuardEventEntity::class],
    version = 1,
    exportSchema = false
)
abstract class ApexDatabase : RoomDatabase() {
    
    abstract fun guardEventDao(): GuardEventDao
    
    companion object {
        @Volatile
        private var INSTANCE: ApexDatabase? = null
        
        fun getDatabase(context: Context): ApexDatabase {
            return INSTANCE ?: synchronized(this) {
                val instance = Room.databaseBuilder(
                    context.applicationContext,
                    ApexDatabase::class.java,
                    "apex_database"
                )
                .fallbackToDestructiveMigration()
                .build()
                INSTANCE = instance
                instance
            }
        }
    }
}

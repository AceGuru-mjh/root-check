package com.apex.root.data.database

import androidx.room.*
import kotlinx.coroutines.flow.Flow

/**
 * 事件历史数据访问对象
 */
@Dao
interface GuardEventDao {
    
    @Query("SELECT * FROM guard_events ORDER BY timestamp DESC")
    fun getAllEvents(): Flow<List<GuardEventEntity>>
    
    @Query("SELECT * FROM guard_events ORDER BY timestamp DESC LIMIT :limit")
    fun getRecentEvents(limit: Int): Flow<List<GuardEventEntity>>
    
    @Query("SELECT * FROM guard_events WHERE handled = 0 ORDER BY timestamp DESC")
    fun getUnhandledEvents(): Flow<List<GuardEventEntity>>
    
    @Query("SELECT * FROM guard_events WHERE type = :type ORDER BY timestamp DESC")
    fun getEventsByType(type: String): Flow<List<GuardEventEntity>>
    
    @Query("SELECT * FROM guard_events WHERE severity = :severity ORDER BY timestamp DESC")
    fun getEventsBySeverity(severity: String): Flow<List<GuardEventEntity>>
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertEvent(event: GuardEventEntity): Long
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertEvents(events: List<GuardEventEntity>)
    
    @Update
    suspend fun updateEvent(event: GuardEventEntity)
    
    @Delete
    suspend fun deleteEvent(event: GuardEventEntity)
    
    @Query("DELETE FROM guard_events WHERE id = :id")
    suspend fun deleteEventById(id: Long)
    
    @Query("UPDATE guard_events SET handled = 1 WHERE id = :id")
    suspend fun markEventAsHandled(id: Long)
    
    @Query("UPDATE guard_events SET handled = 1 WHERE handled = 0")
    suspend fun markAllEventsAsHandled()
    
    @Query("DELETE FROM guard_events WHERE timestamp < :olderThan")
    suspend fun deleteEventsOlderThan(olderThan: Long)
    
    @Query("SELECT COUNT(*) FROM guard_events")
    fun getEventCount(): Flow<Int>
    
    @Query("SELECT COUNT(*) FROM guard_events WHERE handled = 0")
    fun getUnhandledEventCount(): Flow<Int>
}

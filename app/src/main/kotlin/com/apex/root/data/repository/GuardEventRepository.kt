package com.apex.root.data.repository

import com.apex.root.data.database.GuardEventDao
import com.apex.root.data.database.GuardEventEntity
import com.apex.root.domain.model.GuardEvent
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import java.util.UUID

/**
 * 事件历史仓库
 */
class GuardEventRepository(private val guardEventDao: GuardEventDao) {
    
    /**
     * 获取所有事件流
     */
    val allEventsFlow: Flow<List<GuardEvent>> = guardEventDao.getAllEvents()
        .map { entities -> entities.map { it.toDomainModel() } }
    
    /**
     * 获取最近事件流
     */
    fun getRecentEventsFlow(limit: Int = 20): Flow<List<GuardEvent>> {
        return guardEventDao.getRecentEvents(limit)
            .map { entities -> entities.map { it.toDomainModel() } }
    }
    
    /**
     * 获取未处理事件流
     */
    val unhandledEventsFlow: Flow<List<GuardEvent>> = guardEventDao.getUnhandledEvents()
        .map { entities -> entities.map { it.toDomainModel() } }
    
    /**
     * 获取事件总数流
     */
    val eventCountFlow: Flow<Int> = guardEventDao.getEventCount()
    
    /**
     * 获取未处理事件数流
     */
    val unhandledEventCountFlow: Flow<Int> = guardEventDao.getUnhandledEventCount()
    
    /**
     * 插入事件
     */
    suspend fun insertEvent(event: GuardEvent): Long {
        val entity = GuardEventEntity.fromDomainModel(
            event.copy(eventId = event.eventId.ifEmpty { UUID.randomUUID().toString() })
        )
        return guardEventDao.insertEvent(entity)
    }
    
    /**
     * 批量插入事件
     */
    suspend fun insertEvents(events: List<GuardEvent>) {
        val entities = events.map { 
            GuardEventEntity.fromDomainModel(
                it.copy(eventId = it.eventId.ifEmpty { UUID.randomUUID().toString() })
            )
        }
        guardEventDao.insertEvents(entities)
    }
    
    /**
     * 标记事件为已处理
     */
    suspend fun markEventAsHandled(eventId: String) {
        // 需要先查询出 ID
        // 简化处理：假设调用者知道数据库 ID
    }
    
    /**
     * 标记所有事件为已处理
     */
    suspend fun markAllEventsAsHandled() {
        guardEventDao.markAllEventsAsHandled()
    }
    
    /**
     * 删除旧事件（清理策略）
     */
    suspend fun deleteEventsOlderThan(days: Int) {
        val cutoffTime = System.currentTimeMillis() - (days * 24 * 60 * 60 * 1000L)
        guardEventDao.deleteEventsOlderThan(cutoffTime)
    }
    
    /**
     * 删除单个事件
     */
    suspend fun deleteEvent(eventId: String) {
        // 需要实现根据 eventId 删除的逻辑
    }
}

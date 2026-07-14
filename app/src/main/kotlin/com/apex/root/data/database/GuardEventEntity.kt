package com.apex.root.data.database

import androidx.room.Entity
import androidx.room.PrimaryKey
import androidx.room.Index
import com.apex.root.domain.model.GuardEventType
import com.apex.root.domain.model.SeverityLevel

/**
 * 事件历史数据库实体
 */
@Entity(
    tableName = "guard_events",
    indices = [Index(value = ["timestamp"])]
)
data class GuardEventEntity(
    @PrimaryKey(autoGenerate = true)
    val id: Long = 0,
    val eventId: String,
    val type: String, // GuardEventType.name
    val severity: String, // SeverityLevel.name
    val description: String,
    val packageName: String? = null,
    val timestamp: Long = System.currentTimeMillis(),
    val handled: Boolean = false
) {
    fun toDomainModel(): com.apex.root.domain.model.GuardEvent {
        return com.apex.root.domain.model.GuardEvent(
            eventId = eventId,
            type = GuardEventType.valueOf(type),
            severity = SeverityLevel.valueOf(severity),
            description = description,
            packageName = packageName,
            timestamp = timestamp,
            handled = handled
        )
    }
    
    companion object {
        fun fromDomainModel(event: com.apex.root.domain.model.GuardEvent): GuardEventEntity {
            return GuardEventEntity(
                eventId = event.eventId,
                type = event.type.name,
                severity = event.severity.name,
                description = event.description,
                packageName = event.packageName,
                timestamp = event.timestamp,
                handled = event.handled
            )
        }
    }
}

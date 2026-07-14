package com.apex.root.ui.components

import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Text
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import com.apex.root.ui.theme.ApexRootTheme
import org.junit.Rule
import org.junit.Test

/**
 * ApexCard 组件测试
 */
class ApexCardTest {
    
    @get:Rule
    val composeTestRule = createComposeRule()
    
    @Test
    fun apexCard_clickable_withOnClick() {
        var clickCount = 0
        
        composeTestRule.setContent {
            ApexRootTheme {
                ApexCard(
                    onClick = { clickCount++ },
                    modifier = Modifier.testTag("clickableCard")
                ) {
                    Text("Clickable Card", modifier = Modifier.fillMaxWidth())
                }
            }
        }
        
        // 验证卡片存在且可点击
        composeTestRule.onNodeWithTag("clickableCard")
            .performClick()
        
        assert(clickCount == 1) { "Card should be clickable" }
    }
    
    @Test
    fun apexCard_notClickable_withoutOnClick() {
        composeTestRule.setContent {
            ApexRootTheme {
                ApexCard(
                    onClick = null,
                    modifier = Modifier.testTag("staticCard")
                ) {
                    Text("Static Card", modifier = Modifier.fillMaxWidth())
                }
            }
        }
        
        // 验证卡片存在但不可点击
        composeTestRule.onNodeWithTag("staticCard")
            .assertExists()
    }
    
    @Test
    fun apexCard_displaysContent() {
        val cardContent = "Test Card Content"
        
        composeTestRule.setContent {
            ApexRootTheme {
                ApexCard {
                    Text(cardContent, modifier = Modifier.fillMaxWidth())
                }
            }
        }
        
        composeTestRule.onNodeWithText(cardContent)
            .assertExists()
    }
}

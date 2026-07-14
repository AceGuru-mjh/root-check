# APEX Root - UI 重构规范 (UI Refactor Specification)

> Material Design 3 风格 | 高质量优先 | 渐进式开发
> 版本：1.0.0 | 状态：进行中

---

## 📋 目录

1. [设计原则](#设计原则)
2. [技术规范](#技术规范)
3. [组件库规范](#组件库规范)
4. [页面实现规范](#页面实现规范)
5. [性能优化](#性能优化)
6. [测试策略](#测试策略)
7. [无障碍设计](#无障碍设计)
8. [国际化](#国际化)
9. [实施清单](#实施清单)

---

## 设计原则

### Material Design 3 核心准则

1. **动态色彩 (Dynamic Color)**
   - 支持 Android 12+ Monet 取色
   - 提供深色/浅色/对比度主题
   - 品牌色使用 `Primary`, `Secondary`, `Tertiary` 三色系统

2. **形状系统 (Shape System)**
   - 小部件：4dp 圆角
   - 中部件：8dp 圆角
   - 大部件：12dp-16dp 圆角
   - 卡片统一使用 12dp

3. **字体排印 (Typography)**
   - 使用 `MaterialTheme.typography`
   - 标题层级：Display → Headline → Title → Body → Label
   - 中文使用 Noto Sans SC

4. **高程与阴影 (Elevation)**
   - Level 0: 无阴影 (背景)
   - Level 1: 1dp (卡片)
   - Level 2: 3dp (悬浮按钮)
   - Level 3: 6dp (对话框)
   - Level 4: 8dp (顶部栏)
   - Level 5: 12dp (模态弹窗)

5. **动效设计 (Motion)**
   - 默认时长：250ms
   - 快速反馈：150ms
   - 复杂转场：400ms
   - 使用 `animate*AsState()` 和 `AnimatedVisibility`

---

## 技术规范

### 项目配置

#### build.gradle.kts (App Module)
```kotlin
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

android {
    namespace = "com.apex.root"
    compileSdk = 34
    
    defaultConfig {
        applicationId = "com.apex.root"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "2.0.0-m3"
        
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }
    }
    
    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
    
    buildFeatures {
        compose = true
        aidl = false
    }
    
    composeOptions {
        kotlinCompilerExtensionVersion = "1.5.8"
    }
}

dependencies {
    // Compose BOM
    implementation(platform("androidx.compose:compose-bom:2024.01.00"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    
    // Activity + ViewModel
    implementation("androidx.activity:activity-compose:1.8.2")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.7.0")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.7.0")
    
    // Navigation
    implementation("androidx.navigation:navigation-compose:2.7.6")
    
    // Coroutines
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3")
    
    // DataStore
    implementation("androidx.datastore:datastore-preferences:1.0.0")
    
    // Room (可选)
    implementation("androidx.room:room-runtime:2.6.1")
    annotationProcessor("androidx.room:room-compiler:2.6.1")
    
    // Testing
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.compose.ui:ui-test-junit4")
    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
}
```

### 代码规范

#### ViewModel 模式
```kotlin
@HiltViewModel
class DashboardViewModel @Inject constructor(
    private val detectionEngine: DetectionEngine,
    private val deviceRepository: DeviceRepository
) : ViewModel() {
    
    private val _uiState = MutableStateFlow(DashboardUiState())
    val uiState: StateFlow<DashboardUiState> = _uiState.asStateFlow()
    
    fun onAction(action: DashboardAction) {
        viewModelScope.launch {
            when (action) {
                is DashboardAction.StartScan -> startScan()
                is DashboardAction.RefreshDevice -> loadDeviceInfo()
            }
        }
    }
    
    private fun startScan() {
        viewModelScope.launch {
            _uiState.update { it.copy(isScanning = true) }
            try {
                val result = detectionEngine.runFullScan()
                _uiState.update { 
                    it.copy(
                        isScanning = false,
                        lastScanResult = result,
                        scanError = null
                    ) 
                }
            } catch (e: Exception) {
                _uiState.update { 
                    it.copy(
                        isScanning = false,
                        scanError = e.message
                    ) 
                }
            }
        }
    }
}

// UI State
data class DashboardUiState(
    val deviceInfo: DeviceInfo? = null,
    val lastScanResult: RootDetectionResult? = null,
    val isScanning: Boolean = false,
    val scanProgress: Float = 0f,
    val scanError: String? = null,
    val recentEvents: List<GuardEvent> = emptyList()
)

// Actions
sealed interface DashboardAction {
    data object StartScan : DashboardAction
    data object RefreshDevice : DashboardAction
    data class DismissError(val errorId: String) : DashboardAction
}
```

#### Composable 函数规范
```kotlin
@Composable
fun DashboardScreen(
    viewModel: DashboardViewModel = hiltViewModel(),
    navigator: DashboardNavigator,
    modifier: Modifier = Modifier
) {
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()
    
    DashboardContent(
        state = uiState,
        onAction = viewModel::onAction,
        onNavigate = navigator::navigate,
        modifier = modifier
    )
}

@Composable
private fun DashboardContent(
    state: DashboardUiState,
    onAction: (DashboardAction) -> Unit,
    onNavigate: (Destination) -> Unit,
    modifier: Modifier = Modifier
) {
    Scaffold(
        topBar = { ApexTopAppBar(title = "APEX Root") }
    ) { paddingValues ->
        LazyColumn(
            modifier = modifier
                .fillMaxSize()
                .padding(paddingValues),
            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // 内容项
        }
    }
}
```

---

## 组件库规范

### 基础组件

#### 1. ApexTopAppBar
```kotlin
@Composable
fun ApexTopAppBar(
    title: String,
    subtitle: String? = null,
    actions: @Composable RowScope.() -> Unit = {},
    navigationIcon: @Composable () -> Unit = {},
    scrollBehavior: TopAppBarScrollBehavior? = null,
    modifier: Modifier = Modifier
) {
    TopAppBar(
        title = {
            Column {
                Text(
                    text = title,
                    style = MaterialTheme.typography.headlineSmall
                )
                if (subtitle != null) {
                    Text(
                        text = subtitle,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        },
        actions = actions,
        navigationIcon = navigationIcon,
        colors = TopAppBarDefaults.topAppBarColors(
            containerColor = MaterialTheme.colorScheme.surface,
            scrolledContainerColor = MaterialTheme.colorScheme.surfaceVariant
        ),
        scrollBehavior = scrollBehavior,
        modifier = modifier
    )
}
```

#### 2. ApexCard
```kotlin
@Composable
fun ApexCard(
    modifier: Modifier = Modifier,
    onClick: (() -> Unit)? = null,
    enabled: Boolean = true,
    elevation: CardElevation = CardDefaults.cardElevation(defaultElevation = 1.dp),
    colors: CardColors = CardDefaults.cardColors(),
    shape: Shape = CardDefaults.shape,
    content: @Composable ColumnScope.() -> Unit
) {
    Card(
        modifier = modifier,
        onClick = onClick ?: {},
        enabled = enabled && onClick != null,
        elevation = elevation,
        colors = colors,
        shape = shape,
        content = content
    )
}
```

#### 3. RiskLevelBadge
```kotlin
@Composable
fun RiskLevelBadge(
    riskLevel: RiskLevel,
    modifier: Modifier = Modifier
) {
    val (label, color) = when (riskLevel) {
        RiskLevel.NONE -> "安全" to MaterialTheme.colorScheme.primary
        RiskLevel.LOW -> "低风险" to MaterialTheme.colorScheme.secondary
        RiskLevel.MEDIUM -> "中等风险" to MaterialTheme.colorScheme.tertiary
        RiskLevel.HIGH -> "高风险" to MaterialTheme.colorScheme.error
        RiskLevel.CRITICAL -> "严重风险" to MaterialTheme.colorScheme.onErrorContainer
    }
    
    AssistChip(
        onClick = {},
        enabled = false,
        label = { Text(label, style = MaterialTheme.typography.labelSmall) },
        leadingIcon = {
            Icon(
                imageVector = Icons.Default.Shield,
                contentDescription = null,
                tint = color,
                modifier = Modifier.size(16.dp)
            )
        },
        colors = AssistChipDefaults.assistChipColors(
            containerColor = color.copy(alpha = 0.1f)
        ),
        border = AssistChipDefaults.assistChipBorder(
            borderColor = color,
            borderWidth = 1.dp
        ),
        modifier = modifier
    )
}
```

#### 4. ConfigSwitchItem
```kotlin
@Composable
fun ConfigSwitchItem(
    icon: ImageVector,
    title: String,
    subtitle: String? = null,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true
) {
    Surface(
        modifier = modifier
            .fillMaxWidth()
            .clickable(enabled = enabled) { onCheckedChange(!checked) },
        color = Color.Transparent
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp, vertical = 12.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(
                modifier = Modifier.weight(1f),
                horizontalArrangement = Arrangement.spacedBy(12.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    imageVector = icon,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.size(24.dp)
                )
                Column {
                    Text(
                        text = title,
                        style = MaterialTheme.typography.bodyLarge,
                        color = if (enabled) 
                            MaterialTheme.colorScheme.onSurface 
                        else 
                            MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                    )
                    if (subtitle != null) {
                        Text(
                            text = subtitle,
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
            }
            Switch(
                checked = checked,
                onCheckedChange = onCheckedChange,
                enabled = enabled,
                modifier = Modifier.semantics { 
                    stateDescription = if (checked) "已开启" else "已关闭" 
                }
            )
        }
    }
}
```

#### 5. ScanButton
```kotlin
@Composable
fun ScanButton(
    onClick: () -> Unit,
    isScanning: Boolean,
    progress: Float = 0f,
    modifier: Modifier = Modifier,
    enabled: Boolean = true
) {
    val containerColor = if (isScanning) 
        MaterialTheme.colorScheme.secondaryContainer
    else 
        MaterialTheme.colorScheme.primaryContainer
    
    FilledTonalButton(
        onClick = onClick,
        enabled = enabled && !isScanning,
        modifier = modifier
            .fillMaxWidth()
            .height(56.dp),
        colors = ButtonDefaults.filledTonalButtonColors(
            containerColor = containerColor
        )
    ) {
        if (isScanning) {
            CircularProgressIndicator(
                modifier = Modifier.size(20.dp),
                strokeWidth = 2.dp,
                color = MaterialTheme.colorScheme.onSecondaryContainer
            )
            Spacer(Modifier.width(8.dp))
            Text("扫描中 ${(progress * 100).toInt()}%")
        } else {
            Icon(
                imageVector = Icons.Default.Search,
                contentDescription = null,
                modifier = Modifier.size(20.dp)
            )
            Spacer(Modifier.width(8.dp))
            Text("开始扫描")
        }
    }
}
```

---

## 页面实现规范

### DashboardScreen

#### 布局结构
```
Scaffold
├── TopAppBar (带滚动行为)
└── LazyColumn
    ├── DeviceSummaryCard (设备摘要)
    ├── QuickScanSection (快速扫描)
    ├── LastScanResultCard (上次结果)
    ├── RiskOverviewCard (风险概览)
    └── RecentEventsSection (最近事件)
```

#### 关键交互
- 下拉刷新设备信息
- 点击扫描按钮启动扫描
- 点击检测结果查看详情
- 长按事件项显示操作菜单

### SettingsScreen

#### 布局结构
```
Scaffold
├── TopAppBar
│   └── SearchBar (展开/收起)
└── LazyColumn
    ├── GeneralSettingsGroup
    │   ├── 自动扫描
    │   ├── 通知设置
    │   └ ── 主题选择
    ├── DetectionLayersGroup
    │   ├── L1 开关
    │   ├── L4 开关
    │   └── ... (所有检测层)
    ├── AdvancedSettingsGroup
    │   ├── 灵敏度调节
    │   ├── 保护模式
    │   └── 开发者选项
    └── AboutSection
        ├── 版本号
        └── 开源许可
```

#### 关键交互
- 搜索框过滤设置项
- 开关即时生效
- 长按恢复默认值
- 滑动删除自定义配置

### ScanBottomSheet

#### 布局结构
```
ModalBottomSheet
├── SheetHeader (可拖动手柄)
├── ScanProgressHeader
│   ├── 当前层级图标
│   ├── 进度百分比
│   └── 预计剩余时间
├── LinearProgressIndicator (整体进度)
├── LayerProgressBar (当前层进度)
├── CurrentLayerDescription
└── CancelButton
```

#### 关键交互
- 拖动手柄关闭
- 点击取消按钮终止扫描
- 完成后自动 dismiss 或显示结果

---

## 性能优化

### 列表优化
```kotlin
// ✅ 正确：使用 LazyColumn
LazyColumn {
    items(items = dataList, key = { it.id }) { item ->
        ListItemContent(item = item)
    }
}

// ❌ 错误：避免 verticalScroll + Column
Column(modifier = Modifier.verticalScroll(rememberScrollState())) {
    dataList.forEach { item ->
        ListItemContent(item = item) // 全部绘制，性能差
    }
}
```

### 状态优化
```kotlin
// ✅ 正确：细粒度状态更新
data class UiState(
    val isLoading: Boolean = false,
    val items: List<Item> = emptyList(),
    val error: String? = null
)

// 只更新变化的字段
state.copy(items = newItems)

// ❌ 错误：粗粒度状态
var isLoading by mutableStateOf(false)
var items by mutableStateOf(emptyList<Item>())
var error by mutableStateOf<String?>(null)
```

### 图片/图标优化
```kotlin
// ✅ 正确：使用 vector icons
Icon(
    imageVector = Icons.Default.Shield,
    contentDescription = "安全"
)

// ✅ 正确：PainterResource 缓存
val painter = rememberVectorPainter(image = vectorDrawable)

// ❌ 错误：每次重组都加载
Icon(
    painter = painterResource(id = R.drawable.icon), // 每次都重新加载
    contentDescription = null
)
```

---

## 测试策略

### UI 测试
```kotlin
@RunWith(AndroidJUnit4::class)
class DashboardScreenTest {
    
    @get:Rule
    val composeTestRule = createComposeRule()
    
    @Test
    fun dashboard_displaysDeviceSummary() {
        composeTestRule.setContent {
            DashboardContent(
                state = DashboardUiState(
                    deviceInfo = DeviceInfo(
                        brand = "Google",
                        model = "Pixel 8"
                    )
                ),
                onAction = {},
                onNavigate = {}
            )
        }
        
        composeTestRule
            .onNodeWithText("Google Pixel 8")
            .assertIsDisplayed()
    }
    
    @Test
    fun scanButton_clickable_whenNotScanning() {
        composeTestRule.setContent {
            DashboardContent(
                state = DashboardUiState(isScanning = false),
                onAction = {},
                onNavigate = {}
            )
        }
        
        composeTestRule
            .onNodeWithText("开始扫描")
            .performClick()
    }
}
```

### ViewModel 测试
```kotlin
@Test
fun `startScan updates state correctly`() = runTest {
    val mockEngine = MockDetectionEngine()
    val viewModel = DashboardViewModel(mockEngine, mockRepo)
    
    viewModel.onAction(DashboardAction.StartScan)
    
    // 验证扫描中状态
    assertTrue(viewModel.uiState.value.isScanning)
    
    // 等待完成
    advanceUntilIdle()
    
    // 验证结果状态
    assertFalse(viewModel.uiState.value.isScanning)
    assertNotNull(viewModel.uiState.value.lastScanResult)
}
```

---

## 无障碍设计

### ContentDescription
```kotlin
// ✅ 正确：所有交互元素都有描述
IconButton(onClick = { /*...*/ }) {
    Icon(
        imageVector = Icons.Default.Search,
        contentDescription = "搜索设置"
    )
}

// ✅ 正确：装饰性图标设为 null
Icon(
    imageVector = Icons.Default.Check,
    contentDescription = null, // 装饰性
    tint = MaterialTheme.colorScheme.primary
)
```

### 语义节点
```kotlin
// ✅ 正确：添加语义信息
Box(
    modifier = Modifier
        .semantics {
            role = Role.Button
            stateDescription = "已启用"
            onClick { 
                onToggle() 
                true 
            }
        }
        .clickable { onToggle() }
) {
    // 内容
}
```

### 触摸目标
```kotlin
// ✅ 正确：最小 48dp 触摸区域
IconButton(
    onClick = { /*...*/ },
    modifier = Modifier.size(48.dp) // 确保最小尺寸
) {
    Icon(
        imageVector = Icons.Default.Add,
        modifier = Modifier.size(24.dp) // 图标可以更小
    )
}
```

---

## 国际化

### 字符串资源
```xml
<!-- res/values/strings.xml -->
<resources>
    <string name="app_name">APEX Root</string>
    <string name="scan_button">开始扫描</string>
    <string name="risk_level_none">安全</string>
    <string name="risk_level_low">低风险</string>
    <!-- ... -->
</resources>

<!-- res/values-en/strings.xml -->
<resources>
    <string name="app_name">APEX Root</string>
    <string name="scan_button">Start Scan</string>
    <string name="risk_level_none">Secure</string>
    <string name="risk_level_low">Low Risk</string>
    <!-- ... -->
</resources>
```

### Compose 使用
```kotlin
@Composable
fun ScanButton() {
    Text(
        text = stringResource(R.string.scan_button),
        style = MaterialTheme.typography.labelLarge
    )
}
```

---

## 实施清单

### Phase 0 - 基础设施 (当前阶段)
- [x] 创建项目目录结构
- [x] 编写 KNOWLEDGE_GRAPH.md
- [x] 编写 UI_REFACTOR_SPEC.md (本文档)
- [ ] 配置 build.gradle.kts
- [ ] 创建 Theme.kt (Color, Typography, Shape)
- [ ] 创建 MainActivity + NavHost

### Phase 1 - 核心组件
- [ ] ApexTopAppBar
- [ ] ApexCard
- [ ] RiskLevelBadge
- [ ] ConfigSwitchItem
- [ ] ScanButton
- [ ] LayerProgressIndicator
- [ ] EmptyState

### Phase 2 - Dashboard 页面
- [ ] DashboardViewModel
- [ ] DashboardUiState + Actions
- [ ] DashboardScreen 布局
- [ ] DeviceSummaryCard
- [ ] QuickScanSection
- [ ] LastScanResultCard
- [ ] RiskOverviewCard
- [ ] RecentEventsSection

### Phase 3 - Settings 页面
- [ ] SettingsViewModel
- [ ] SettingsUiState + Actions
- [ ] SettingsScreen 布局
- [ ] SearchBar (展开/收起)
- [ ] SettingsGroupHeader
- [ ] GeneralSettingsGroup
- [ ] DetectionLayersGroup
- [ ] AdvancedSettingsGroup
- [ ] AboutSection

### Phase 4 - Scan 功能
- [ ] ScanViewModel
- [ ] ScanSession 状态管理
- [ ] ScanBottomSheet
- [ ] ScanProgressHeader
- [ ] LayerProgressBar
- [ ] Cancel/Pause 逻辑
- [ ] 完成回调

### Phase 5 - 工具页面
- [ ] ToolsScreen
- [ ] ToolCard
- [ ] 工具箱功能列表

### Phase 6 - 关于页面
- [ ] AboutScreen
- [ ] AppInfoCard
- [ ] LicenseList
- [ ] OpenSourceLibraries

### Phase 7 - 集成与测试
- [ ] 导航流程整合
- [ ] UI 测试覆盖
- [ ] ViewModel 测试覆盖
- [ ] 性能测试
- [ ] 无障碍测试

---

## 质量检查清单

### 代码审查要点
- [ ] 所有 Composable 函数都有预览
- [ ] 状态提升正确实现
- [ ] 没有硬编码字符串 (全部使用 stringResource)
- [ ] 所有交互元素有 contentDescription
- [ ] 触摸目标≥48dp
- [ ] 颜色使用 MaterialTheme.colorScheme
- [ ] 字体使用 MaterialTheme.typography
- [ ] 列表使用 LazyColumn/LazyRow
- [ ] 协程作用域正确管理
- [ ] 异常处理完善

### 视觉检查要点
- [ ] 间距符合 8dp 网格系统
- [ ] 卡片圆角一致 (12dp)
- [ ] 阴影层级正确
- [ ] 深色模式适配完整
- [ ] 动画流畅 (60fps)
- [ ] 不同屏幕尺寸适配

---

*本规范随开发进度持续更新，所有 UI 实现必须遵循此规范*

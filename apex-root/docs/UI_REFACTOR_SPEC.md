# APEX Root UI 重构规范 (SPEC)

**版本**: 1.0.0  
**创建日期**: 2024-01-XX  
**状态**: 待实施  
**设计语言**: Material Design 3 (M3)

---

## 一、重构目标

### 1.1 核心目标
- ✅ **解决所有已知 UI 问题**: 按钮无响应、滑动卡顿、嵌套滚动冲突
- ✅ **采用 Material Design 3**: 使用官方 M3 组件和设计规范
- ✅ **高质量优先**: 代码可维护性 > 开发速度
- ✅ **性能优化**: LazyColumn 替代 verticalScroll，避免过度重绘
- ✅ **简化配置**: 将 200+ 配置项精简为 20-30 个核心配置

### 1.2 技术栈
- **UI 框架**: Jetpack Compose (最新稳定版)
- **设计系统**: Material 3 (`androidx.material3:material3`)
- **架构**: MVVM + 单向数据流
- **依赖注入**: Hilt (如已使用) 或 Koin
- **异步**: Kotlin Coroutines + Flow

### 1.3 设计原则
1. **状态提升**: 状态保存在 ViewModel，UI 只是状态的函数
2. **不可变数据**: 所有 UI 状态数据类使用 `val`
3. **单一职责**: 每个 Composable 只做一件事
4. **可组合性**: 小组件组合成大组件，避免巨型 Composable
5. **懒加载**: 列表/长内容必须使用 `LazyColumn`/`LazyRow`

---

## 二、信息架构

### 2.1 应用结构
```
APEX Root
├── 🏠 首页 (Dashboard)
│   ├── 一键扫描
│   ├── 风险状态展示
│   ├── 快速操作卡片
│   └── 最近警报摘要
├── 🔍 扫描 (Scan)
│   ├── 扫描进度
│   ├── 分层检测结果
│   └── 详细报告
├── 🛡️ 防护 (Guard)
│   ├── 防护开关
│   ├── 实时监控
│   └── 警报历史
├── ⚙️ 设置 (Settings)
│   ├── 检测设置
│   ├── 防护设置
│   ├── 外观设置
│   └── 高级设置 (开发者模式)
└── ℹ️ 关于 (About)
    ├── 应用信息
    ├── 许可证
    └── 检查更新
```

### 2.2 导航图 (Navigation Graph)
```kotlin
sealed class Screen(val route: String) {
    object Splash : Screen("splash")
    object Dashboard : Screen("dashboard")
    object Scan : Screen("scan")
    object Report : Screen("report/{taskId}") {
        fun createRoute(taskId: String) = "report/$taskId"
    }
    object Alert : Screen("alert")
    object Guard : Screen("guard")
    object Settings : Screen("settings")
    object Permissions : Screen("permissions")
    object Whitelist : Screen("whitelist")
    object History : Screen("history")
    object Update : Screen("update")
    object About : Screen("about")
    // ... 其他页面
}

// 顶层导航 (Bottom Navigation)
val topLevelDestinations = listOf(
    Screen.Dashboard,
    Screen.Scan,
    Screen.Guard,
    Screen.Settings
)
```

---

## 三、主题设计 (Theme)

### 3.1 颜色方案
```kotlin
// 使用 Material 3 动态配色 (Dynamic Color) + 自定义品牌色
private val DarkColorScheme = darkColorScheme(
    primary = Purple80,
    secondary = PurpleGrey80,
    tertiary = Pink80,
    background = Color(0xFF1C1B1F),
    surface = Color(0xFF1C1B1F),
    onPrimary = Color.White,
    onSecondary = Color.White,
    onTertiary = Color.White,
    onBackground = Color.White,
    onSurface = Color.White
)

private val LightColorScheme = lightColorScheme(
    primary = Purple40,
    secondary = PurpleGrey40,
    tertiary = Pink40,
    background = Color(0xFFFFFBFE),
    surface = Color(0xFFFFFBFE),
    onPrimary = Color.White,
    onSecondary = Color.White,
    onTertiary = Color.White,
    onBackground = Color(0xFF1C1B1F),
    onSurface = Color(0xFF1C1B1F)
)

// 语义化颜色扩展
object ApexColors {
    val Safe = Color(0xFF4CAF50)      // 绿色
    val Warning = Color(0xFFFFC107)   // 黄色
    val Danger = Color(0xFFFF5722)    // 橙色
    val Critical = Color(0xFFF44336)  // 红色
    
    val Layer1 = Color(0xFF2196F3)    // 蓝色
    val Layer2 = Color(0xFF9C27B0)    // 紫色
    val Layer3 = Color(0xFFFF9800)    // 橙色
}
```

### 3.2 字体排印
```kotlin
// 使用 Material 3 默认字体排印，仅自定义标题
val ApexTypography = Typography(
    displayLarge = TextStyle(
        fontFamily = FontFamily.Default,
        fontWeight = FontWeight.Bold,
        fontSize = 57.sp,
        lineHeight = 64.sp,
        letterSpacing = (-0.25).sp
    ),
    headlineLarge = TextStyle(
        fontWeight = FontWeight.SemiBold,
        fontSize = 32.sp,
        lineHeight = 40.sp
    ),
    titleLarge = TextStyle(
        fontWeight = FontWeight.Medium,
        fontSize = 22.sp,
        lineHeight = 28.sp
    ),
    bodyLarge = TextStyle(
        fontSize = 16.sp,
        lineHeight = 24.sp,
        letterSpacing = 0.5.sp
    )
)
```

### 3.3 形状和圆角
```kotlin
val ApexShapes = Shapes(
    extraSmall = RoundedCornerShape(4.dp),
    small = RoundedCornerShape(8.dp),
    medium = RoundedCornerShape(12.dp),
    large = RoundedCornerShape(16.dp),
    extraLarge = RoundedCornerShape(24.dp)
)
```

---

## 四、核心组件库

### 4.1 通用组件

#### 4.1.1 ApexButton (主按钮)
```kotlin
@Composable
fun ApexButton(
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    loading: Boolean = false,
    text: String,
    icon: Painter? = null,
    variant: ButtonVariant = ButtonVariant.Primary
) {
    // 实现：支持 Primary/Secondary/Tertiary/Text 变体
    // 支持 loading 状态（显示 CircularProgressIndicator）
    // 支持图标 + 文本组合
}

enum class ButtonVariant {
    Primary, Secondary, Tertiary, Text, Destructive
}
```

#### 4.1.2 ApexCard (卡片容器)
```kotlin
@Composable
fun ApexCard(
    modifier: Modifier = Modifier,
    onClick: (() -> Unit)? = null,
    elevation: CardElevation = CardDefaults.cardElevation(defaultElevation = 1.dp),
    colors: CardColors = CardDefaults.cardColors(),
    content: @Composable ColumnScope.() -> Unit
) {
    // 实现：Material 3 Card 包装器
    // 支持可选点击
    // 统一圆角和内边距
}
```

#### 4.1.3 ApexListItem (列表项)
```kotlin
@Composable
fun ApexListItem(
    heading: String,
    supportingText: String? = null,
    leadingIcon: @Composable (() -> Unit)? = null,
    trailingContent: @Composable (() -> Unit)? = null,
    onClick: (() -> Unit)? = null,
    modifier: Modifier = Modifier,
    enabled: Boolean = true
) {
    // 实现：Material 3 ListItem 包装器
    // 统一的间距和样式
    // 支持 leading/trailing 内容
}
```

#### 4.1.4 ApexSwitch (开关项)
```kotlin
@Composable
fun ApexSwitchItem(
    label: String,
    description: String? = null,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    leadingIcon: @Composable (() -> Unit)? = null
) {
    // 实现：带标签和描述的 Switch
    // 整个区域可点击
    // 避免点击冲突
}
```

#### 4.1.5 ApexDialog (对话框)
```kotlin
@Composable
fun ApexDialog(
    onDismissRequest: () -> Unit,
    title: String,
    text: String,
    confirmButton: @Composable () -> Unit,
    dismissButton: @Composable (() -> Unit)? = null,
    icon: @Composable (() -> Unit)? = null
) {
    // 实现：Material 3 AlertDialog 包装器
    // 统一的样式
}
```

#### 4.1.6 ApexProgressBar (进度条)
```kotlin
@Composable
fun ApexProgressBar(
    progress: Float, // 0.0 - 1.0
    modifier: Modifier = Modifier,
    color: Color = MaterialTheme.colorScheme.primary,
    showLabel: Boolean = false,
    label: String = "${(progress * 100).toInt()}%"
) {
    // 实现：LinearProgressIndicator + 可选百分比标签
}
```

#### 4.1.7 ApexChip (选择芯片)
```kotlin
@Composable
fun ApexChip(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    icon: Painter? = null,
    enabled: Boolean = true
) {
    // 实现：FilterChip / AssistChip / SuggestionChip
    // 支持单选/多选场景
}
```

#### 4.1.8 ApexEmptyState (空状态)
```kotlin
@Composable
fun ApexEmptyState(
    icon: Painter,
    title: String,
    subtitle: String? = null,
    actionButton: @Composable (() -> Unit)? = null,
    modifier: Modifier = Modifier
) {
    // 实现：统一的空状态布局
    // 图标 + 标题 + 副标题 + 可选操作按钮
}
```

#### 4.1.9 ApexErrorState (错误状态)
```kotlin
@Composable
fun ApexErrorState(
    message: String,
    onRetry: (() -> Unit)? = null,
    modifier: Modifier = Modifier
) {
    // 实现：错误提示 UI
    // 支持重试按钮
}
```

#### 4.1.10 ApexLoadingOverlay (加载遮罩)
```kotlin
@Composable
fun ApexLoadingOverlay(
    isLoading: Boolean,
    message: String? = null,
    modifier: Modifier = Modifier
) {
    // 实现：全屏/局部加载遮罩
    // 半透明背景 + CircularProgressIndicator + 可选文本
}
```

### 4.2 仪表板专用组件

#### 4.2.1 RiskLevelGauge (风险等级仪表)
```kotlin
@Composable
fun RiskLevelGauge(
    riskLevel: RiskLevel,
    riskScore: Int,
    modifier: Modifier = Modifier
) {
    // 实现：环形进度条 + 风险等级文字
    // 颜色根据风险等级变化 (绿/黄/橙/红)
}
```

#### 4.2.2 ScanActionButton (扫描按钮)
```kotlin
@Composable
fun ScanActionButton(
    isScanning: Boolean,
    progress: Float,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    // 实现：大型圆形扫描按钮
    // 空闲状态：显示"扫描"文字 + 图标
    // 扫描中：显示环形进度 + 百分比
    // 完成：显示对勾动画
}
```

#### 4.2.3 QuickActionCard (快速操作卡片)
```kotlin
@Composable
fun QuickActionCard(
    icon: Painter,
    title: String,
    subtitle: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    badge: String? = null
) {
    // 实现：带图标、标题、副标题的卡片
    // 可选角标（如"新"、"推荐"）
}
```

#### 4.2.4 RecentAlertsList (最近警报列表)
```kotlin
@Composable
fun RecentAlertsList(
    alerts: List<SecurityAlert>,
    onViewAll: () -> Unit,
    onAlertClick: (SecurityAlert) -> Unit,
    modifier: Modifier = Modifier
) {
    // 实现：LazyColumn 展示最近 5 条警报
    // 支持展开查看更多
}
```

#### 4.2.5 DeviceInfoCard (设备信息卡片)
```kotlin
@Composable
fun DeviceInfoCard(
    deviceModel: String,
    androidVersion: String,
    securityPatchLevel: String,
    bootloaderStatus: BootloaderStatus,
    modifier: Modifier = Modifier
) {
    // 实现：展示设备基本信息
    // Bootloader 状态用颜色区分 (解锁/锁定)
}
```

### 4.3 扫描结果组件

#### 4.3.1 LayerResultItem (检测层结果项)
```kotlin
@Composable
fun LayerResultItem(
    layerId: Int,
    layerName: String,
    detected: Boolean,
    latencyMs: Long,
    weight: Int,
    expanded: Boolean,
    onExpandToggle: () -> Unit,
    modifier: Modifier = Modifier
) {
    // 实现：可展开的检测层结果
    // 显示层名称、状态、耗时、权重
    // 展开显示详细信息
}
```

#### 4.3.2 ScanProgressScreen (扫描进度页)
```kotlin
@Composable
fun ScanProgressScreen(
    currentLayer: String,
    progress: Int, // 0-100
    completedLayers: Int,
    totalLayers: Int,
    modifier: Modifier = Modifier
) {
    // 实现：全屏扫描进度
    // 显示当前检测层名称
    // 进度条 + 已完成层数
}
```

#### 4.3.3 ReportSummary (报告摘要)
```kotlin
@Composable
fun ReportSummary(
    result: ParallelScanResult,
    modifier: Modifier = Modifier
) {
    // 实现：扫描报告摘要
    // 总风险分数、检测到的层数、总耗时
    // 风险等级徽章
}
```

### 4.4 设置页面组件

#### 4.4.1 SettingsCategory (设置分类)
```kotlin
@Composable
fun SettingsCategory(
    title: String,
    items: List<SettingsItem>,
    modifier: Modifier = Modifier
) {
    // 实现：设置分类标题 + 列表项
    // 使用 LazyColumn 优化性能
}

data class SettingsItem(
    val icon: Painter,
    val title: String,
    val subtitle: String? = null,
    val type: SettingsItemType
)

sealed class SettingsItemType {
    data class Toggle(val checked: Boolean, val onCheckedChange: (Boolean) -> Unit) : SettingsItemType()
    data class Select(val options: List<String>, val selectedIndex: Int, val onSelect: (Int) -> Unit) : SettingsItemType()
    data class Navigate(val onClick: () -> Unit) : SettingsItemType()
    data class Slider(val value: Float, val valueRange: ClosedFloatingPointRange<Float>, val onValueChange: (Float) -> Unit) : SettingsItemType()
}
```

#### 4.4.2 SettingsSlider (设置滑块)
```kotlin
@Composable
fun SettingsSlider(
    label: String,
    value: Float,
    valueRange: ClosedFloatingPointRange<Float>,
    onValueChange: (Float) -> Unit,
    valueFormatter: (Float) -> String = { it.toInt().toString() },
    modifier: Modifier = Modifier
) {
    // 实现：带标签和值显示的 Slider
}
```

---

## 五、页面实现规范

### 5.1 DashboardScreen (主仪表板)

#### 功能需求
1. 展示设备安全状态（风险等级、分数）
2. 一键扫描入口
3. 快速操作卡片（防护、隐藏、清理等）
4. 最近警报摘要（最多 5 条）
5. 设备基本信息

#### 布局结构
```
DashboardScreen
├── TopAppBar (标题 + 设置入口)
├── Column (verticalScroll 替换为 LazyColumn)
│   ├── RiskLevelGauge (风险仪表)
│   ├── ScanActionButton (扫描按钮)
│   ├── Spacer
│   ├── Text ("快速操作")
│   ├── Row (快速操作卡片网格)
│   │   ├── QuickActionCard (防护)
│   │   ├── QuickActionCard (隐藏)
│   │   ├── QuickActionCard (清理)
│   │   └── QuickActionCard (更多)
│   ├── Spacer
│   ├── Row (标题 + 查看全部)
│   │   ├── Text ("最近警报")
│   │   └── TextButton ("查看全部")
│   └── RecentAlertsList (警报列表)
└── BottomNavigationBar
```

#### 关键实现点
- ✅ 使用 `LazyColumn` 替代 `Column + verticalScroll`
- ✅ 扫描按钮状态由 ViewModel 驱动
- ✅ 警报列表点击跳转到 AlertScreen
- ✅ 支持下拉刷新触发扫描

### 5.2 SettingsScreen (设置页)

#### 功能需求
1. 分组展示配置项（检测、防护、外观、高级）
2. 支持开关、下拉选择、滑块、跳转
3. 搜索功能（P2）
4. 重置为默认值（P2）

#### 配置分组 (精简版)
```kotlin
sealed class SettingsGroup(val title: String) {
    object Detection : SettingsGroup("检测设置")
    object Guard : SettingsGroup("防护设置")
    object Appearance : SettingsGroup("外观设置")
    object Advanced : SettingsGroup("高级设置")
}

// 核心配置项 (20-30 个)
data class CoreSettings(
    // 检测设置
    val detectionLevel: DetectionLevel,
    val autoScanInterval: AutoScanInterval,
    val enableLayer1: Boolean,
    val enableLayer2: Boolean,
    // ... 各检测层开关 (合并为"启用所有检测层"总开关 + 单独高级设置)
    
    // 防护设置
    val guardEnabled: Boolean,
    val guardLevel: GuardLevel,
    val hideStrategy: HideStrategy,
    
    // 外观设置
    val themeMode: ThemeMode,
    val accentColor: AccentColor,
    val useDynamicColor: Boolean,
    
    // 高级设置
    val updateChannel: UpdateChannel,
    val logLevel: LogLevel,
    val enableExperimentalFeatures: Boolean
)
```

#### 布局结构
```
SettingsScreen
├── TopAppBar (标题 + 搜索图标)
├── LazyColumn
│   ├── SettingsCategory ("检测设置")
│   │   ├── ApexListItem (检测深度 → 跳转选择页)
│   │   ├── ApexListItem (自动扫描间隔 → 跳转选择页)
│   │   └── ApexSwitchItem (启用所有检测层)
│   ├── SettingsCategory ("防护设置")
│   │   ├── ApexSwitchItem (启用防护)
│   │   ├── ApexListItem (防护等级 → 跳转选择页)
│   │   └── ApexListItem (隐藏策略 → 跳转选择页)
│   ├── SettingsCategory ("外观设置")
│   │   ├── ApexListItem (主题模式 → 跳转选择页)
│   │   ├── ApexListItem (强调色 → 跳转选择页)
│   │   └── ApexSwitchItem (使用动态配色)
│   └── SettingsCategory ("高级设置")
│       ├── ApexListItem (更新渠道 → 跳转选择页)
│       ├── ApexListItem (日志级别 → 跳转选择页)
│       ├── ApexSwitchItem (启用实验性功能)
│       └── ApexListItem (重置为默认值 → 确认对话框)
└── BottomNavigationBar
```

#### 关键实现点
- ✅ 必须使用 `LazyColumn` 避免性能问题
- ✅ 设置变更立即生效并保存到 SettingsRepository
- ✅ 使用 `rememberUpdatedState` 避免重组问题
- ✅ 高级设置默认折叠，需要连续点击 7 次版本号才显示

### 5.3 SplashScreen (启动页)

#### 功能需求
1. 展示应用 Logo 和名称
2. 检查必要权限
3. 初始化 Native 库
4. 自动跳转到 Dashboard

#### 布局结构
```
SplashScreen
├── Box (居中)
│   ├── Image (Logo)
│   ├── Text ("APEX Root")
│   └── CircularProgressIndicator (底部)
└── LaunchedEffect
    ├── 请求权限
    ├── 加载 Native 库
    └── 导航到 Dashboard
```

### 5.4 ReportScreen (报告页)

#### 功能需求
1. 展示扫描结果摘要
2. 分层检测结果列表
3. 导出报告按钮
4. 分享按钮

#### 布局结构
```
ReportScreen
├── TopAppBar (标题 + 导出 + 分享)
├── LazyColumn
│   ├── ReportSummary (摘要卡片)
│   ├── Divider
│   ├── Text ("检测详情")
│   └── 展开/收起所有
│   └── LayerResultItem (重复，每层一个)
└── BottomBar
    ├── Button ("重新扫描")
    └── Button ("返回主页")
```

---

## 六、性能优化规范

### 6.1 列表优化
```kotlin
// ✅ 正确：使用 LazyColumn
@Composable
fun OptimizedList(items: List<String>) {
    LazyColumn {
        items(items) { item ->
            ListItem(text = item)
        }
    }
}

// ❌ 错误：使用 verticalScroll
@Composable
fun SlowList(items: List<String>) {
    Column(modifier = Modifier.verticalScroll(rememberScrollState())) {
        items.forEach { item ->
            ListItem(text = item)
        }
    }
}
```

### 6.2 重组优化
```kotlin
// ✅ 正确：使用 key 参数
LazyColumn {
    items(items, key = { it.id }) { item ->
        ExpensiveComposable(item)
    }
}

// ✅ 正确：拆分 Composable
@Composable
fun Parent(items: List<Item>) {
    LazyColumn {
        items(items) { item ->
            Child(item) // 子 Composable 独立重组
        }
    }
}

@Composable
fun Child(item: Item) {
    // 只依赖 item，父状态变化不触发此重组
}

// ❌ 错误：Lambda 捕获不稳定状态
@Composable
fun BadExample(state: StateFlow<Int>) {
    val onClick = { /* 使用 state.value */ } // 每次重组都创建新 Lambda
    Button(onClick = onClick) { }
}

// ✅ 正确：使用 rememberUpdatedState
@Composable
fun GoodExample(state: StateFlow<Int>) {
    val currentState by state.collectAsState()
    val onClick by rememberUpdatedState(currentState) {
        // 使用 currentState
    }
    Button(onClick = onClick) { }
}
```

### 6.3 图片加载优化
```kotlin
// ✅ 使用 Coil 加载图片
@Composable
fun ImageItem(url: String) {
    AsyncImage(
        model = ImageRequest.Builder(LocalContext.current)
            .data(url)
            .crossfade(true)
            .build(),
        contentDescription = null,
        modifier = Modifier.size(100.dp)
    )
}
```

### 6.4 状态收集优化
```kotlin
// ✅ 正确：在 Composable 中收集 StateFlow
@Composable
fun MyScreen(viewModel: MyViewModel = hiltViewModel()) {
    val uiState by viewModel.uiState.collectAsState()
    
    when (val state = uiState) {
        is UiState.Loading -> LoadingSpinner()
        is UiState.Success -> Content(state.data)
        is UiState.Error -> ErrorState(state.message)
    }
}

// ✅ 正确：侧效应在 LaunchedEffect 中
@Composable
fun MyScreenWithSideEffect(viewModel: MyViewModel) {
    val navController = LocalNavController.current
    
    LaunchedEffect(Unit) {
        viewModel.events.collect { event ->
            when (event) {
                is Event.NavigateToDetail -> navController.navigate("detail/${event.id}")
                is Event.ShowSnackbar -> snackbarHost.showSnackbar(event.message)
            }
        }
    }
    
    // UI 内容
}
```

---

## 七、测试规范

### 7.1 单元测试
```kotlin
class DashboardViewModelTest {
    @Test
    fun `scan button click should start scan`() = runTest {
        val viewModel = DashboardViewModel(mockRepository)
        viewModel.onScanClick()
        
        assertEquals(UiState.Scanning, viewModel.uiState.value)
    }
    
    @Test
    fun `scan completion should update result`() = runTest {
        val viewModel = DashboardViewModel(mockRepository)
        val mockResult = ParallelScanResult(...)
        
        viewModel.onScanComplete(mockResult)
        
        val state = viewModel.uiState.value as UiState.Result
        assertEquals(mockResult, state.result)
    }
}
```

### 7.2 UI 测试
```kotlin
@RunWith(AndroidJUnit4::class)
class DashboardScreenTest {
    
    @get:Rule
    val composeTestRule = createComposeRule()
    
    @Test
    fun dashboard_showsRiskGauge() {
        composeTestRule.setContent {
            DashboardScreen(viewModel = fakeViewModel)
        }
        
        composeTestRule
            .onNodeWithText("风险等级")
            .assertIsDisplayed()
    }
    
    @Test
    fun scanButton_clickable() {
        composeTestRule.setContent {
            DashboardScreen(viewModel = fakeViewModel)
        }
        
        composeTestRule
            .onNodeWithContentDescription("扫描按钮")
            .performClick()
        
        verify(fakeViewModel).onScanClick()
    }
    
    @Test
    fun alertsList_scrolls() {
        composeTestRule.setContent {
            DashboardScreen(viewModel = fakeViewModelWithManyAlerts)
        }
        
        composeTestRule
            .onNodeWithTag("alertsList")
            .performTouchInput {
                swipeUp()
            }
        
        // 验证滚动后新项可见
        composeTestRule
            .onNodeWithText("警报 20")
            .assertIsDisplayed()
    }
}
```

---

## 八、无障碍 (Accessibility)

### 8.1 内容描述
```kotlin
// ✅ 正确：为图标按钮添加 contentDescription
IconButton(
    onClick = { },
    modifier = Modifier.semantics {
        contentDescription = "设置"
    }
) {
    Icon(Icons.Default.Settings, contentDescription = null)
}

// ✅ 正确：为图片添加描述
Image(
    painter = painterResource(R.drawable.logo),
    contentDescription = "APEX Root Logo"
)

// ❌ 错误：缺少内容描述
IconButton(onClick = { }) {
    Icon(Icons.Default.Settings, contentDescription = null)
}
```

### 8.2 焦点管理
```kotlin
// ✅ 正确：为自定义组件添加语义
@Composable
fun CustomCard(title: String, onClick: () -> Unit) {
    Surface(
        modifier = Modifier
            .clickable(onClick = onClick)
            .semantics {
                role = Role.Button
                contentDescription = title
            }
    ) {
        Text(title)
    }
}
```

### 8.3 字体缩放支持
```kotlin
// ✅ 正确：使用 sp 单位，支持系统字体缩放
Text(
    text = "标题",
    fontSize = 16.sp // 会随系统设置缩放
)

// ❌ 错误：使用固定像素
Text(
    text = "标题",
    fontSize = 16.px // 不会缩放
)
```

---

## 九、国际化 (i18n)

### 9.1 字符串资源
```xml
<!-- res/values/strings.xml -->
<resources>
    <string name="app_name">APEX Root</string>
    <string name="scan_button">扫描</string>
    <string name="risk_level_safe">安全</string>
    <string name="risk_level_warning">警告</string>
    <string name="risk_level_danger">危险</string>
    <string name="risk_level_critical">严重</string>
    
    <!-- 带参数的字符串 -->
    <string name="detected_layers">检测到 %d 个异常层</string>
    <string name="scan_duration">耗时 %d ms</string>
</resources>
```

### 9.2 使用字符串资源
```kotlin
// ✅ 正确：使用 stringResource
@Composable
fun RiskBadge(riskScore: Int) {
    Text(
        text = stringResource(
            R.string.detected_layers,
            riskScore
        )
    )
}
```

---

## 十、实施清单

### Phase 0: 准备 (本周)
- [x] 删除旧 UI 组件
- [x] 创建知识图谱
- [x] 创建本 SPEC 文档
- [ ] 更新 build.gradle.kts 添加 M3 依赖
- [ ] 创建基础 Theme.kt

### Phase 1: 核心组件 (1-2 周)
- [ ] 实现所有 4.1 通用组件
- [ ] 编写组件单元测试
- [ ] 编写组件 UI 测试
- [ ] 创建组件文档 (README)

### Phase 2: 核心页面 (2-3 周)
- [ ] SplashScreen
- [ ] DashboardScreen
- [ ] SettingsScreen
- [ ] AppNavigation

### Phase 3: 功能页面 (2-3 周)
- [ ] ScanProgressScreen
- [ ] ReportScreen
- [ ] AlertScreen
- [ ] GuardMonitorScreen

### Phase 4: 辅助页面 (1-2 周)
- [ ] PermissionsScreen
- [ ] WhitelistScreen
- [ ] HistoryScreen
- [ ] UpdateScreen
- [ ] AboutScreen

### Phase 5: 优化和清理 (1 周)
- [ ] 性能分析 (使用 Android Studio Profiler)
- [ ] 修复性能瓶颈
- [ ] 无障碍测试
- [ ] 国际化检查
- [ ] 删除遗留代码
- [ ] 最终测试

---

## 十一、依赖版本

```kotlin
// build.gradle.kts (Module: app)
dependencies {
    // Core
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.7.0")
    implementation("androidx.activity:activity-compose:1.8.2")
    
    // Compose BOM
    implementation(platform("androidx.compose:compose-bom:2024.01.00"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
    
    // Material 3
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    
    // Navigation
    implementation("androidx.navigation:navigation-compose:2.7.6")
    
    // Lifecycle
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.7.0")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.7.0")
    
    // Coroutines
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3")
    
    // Dependency Injection (可选)
    implementation("com.google.dagger:hilt-android:2.48.1")
    kapt("com.google.dagger:hilt-compiler:2.48.1")
    implementation("androidx.hilt:hilt-navigation-compose:1.1.0")
    
    // Image Loading
    implementation("io.coil-kt:coil-compose:2.5.0")
    
    // Testing
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.7.3")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    androidTestImplementation(platform("androidx.compose:compose-bom:2024.01.00"))
    androidTestImplementation("androidx.compose.ui:ui-test-junit4")
    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
}
```

---

## 十二、变更日志

| 版本 | 日期 | 变更内容 | 作者 |
|-----|------|---------|------|
| 1.0.0 | 2024-01-XX | 初始版本 | AI Assistant |

---

**审批**:
- [ ] 技术负责人审批
- [ ] 设计师审批
- [ ] 产品经理审批

**备注**: 本文档为活文档，实施过程中可根据实际情况调整，但需在变更日志中记录。

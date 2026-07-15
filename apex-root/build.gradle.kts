plugins {
    // v1.1.3 工具链升级: AGP 8.2.0 → 8.5.2, Kotlin 1.9.20 → 2.0.21
    id("com.android.application") version "8.5.2" apply false
    id("org.jetbrains.kotlin.android") version "2.0.21" apply false
    // v1.1.3: Kotlin 2.0 内置 Compose 编译器插件,替代 composeOptions.kotlinCompilerExtensionVersion
    id("org.jetbrains.kotlin.plugin.compose") version "2.0.21" apply false
    id("org.jlleitschuh.gradle.ktlint") version "12.1.1" apply false
    id("io.gitlab.arturbosch.detekt") version "1.23.7" apply false
}

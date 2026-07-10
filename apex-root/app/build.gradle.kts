plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("com.google.protobuf") version "0.9.4"
    id("org.jlleitschuh.gradle.ktlint")
    id("io.gitlab.arturbosch.detekt")
}

ktlint {
    version.set("0.50.0")
    android.set(true)
}

detekt {
    config.setFrom(file(".detekt/config.yaml"))
    buildUponDefaultConfig = true
}

android {
    namespace = "com.apex.root"
    compileSdk = 34
    ndkVersion = "28.2.13676358"

    // 签名配置 — AppDev / OpenSource
    // keystore 已在 .gitignore 中（不入库），此处做存在性判断：
    // - 存在 apex-release.jks：debug/release 均使用该签名（便于与已安装版本共存）
    // - 不存在：debug 使用默认 debug 签名，release 产出 unsigned APK
    //   这样开箱即用，无需手动生成 keystore 即可构建 debug。
    //
    // 安全修复: 签名密钥密码不再硬编码于源码中。
    //   优先从 gradle.properties (项目根,不入库) 读取;其次从环境变量读取。
    //   读取顺序: APEX_STORE_PASS / APEX_KEY_PASS
    //   (本地开发可在 ~/.gradle/gradle.properties 中设置)
    val ksFile = rootProject.file("apex-release.jks")
    val hasKeystore = ksFile.exists()
    val apexStorePass = project.findProperty("APEX_STORE_PASS") as String?
        ?: System.getenv("APEX_STORE_PASS")
    val apexKeyPass = project.findProperty("APEX_KEY_PASS") as String?
        ?: System.getenv("APEX_KEY_PASS")
    val apexKeyAlias = project.findProperty("APEX_KEY_ALIAS") as String?
        ?: System.getenv("APEX_KEY_ALIAS")
        ?: "root"
    // 只有 keystore 文件存在 且 密码可用时,才创建 release signingConfig。
    // 这样既不破坏开源开发体验 (没有 keystore 时仍可 debug build),
    // 也避免把密码泄露到源码历史里。
    val hasSigningCreds = hasKeystore && !apexStorePass.isNullOrEmpty() && !apexKeyPass.isNullOrEmpty()
    if (hasKeystore && !hasSigningCreds) {
        logger.warn("APEX: apex-release.jks exists but APEX_STORE_PASS/APEX_KEY_PASS not set — " +
            "set them in ~/.gradle/gradle.properties or env to sign release builds. " +
            "Falling back to debug/unsigned.")
    }
    signingConfigs {
        if (hasSigningCreds) {
            create("release") {
                storeFile = ksFile
                storePassword = apexStorePass
                keyAlias = apexKeyAlias
                keyPassword = apexKeyPass
                // 启用 v2/v3 签名方案,提高篡改难度
                enableV1Signing = true
                enableV2Signing = true
                enableV3Signing = true
            }
        }
    }

    defaultConfig {
        applicationId = "com.apex.root"
        minSdk = 29
        targetSdk = 34
        versionCode = 121
        versionName = "1.0.7"
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // 全架构由 splits.abi 控制，此处不再设置 ndk.abiFilters
        // （二者不能同时存在，否则 Gradle 报 Conflicting configuration）

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20 -fvisibility=hidden -fstack-protector-strong"
                arguments += "-DANDROID_STL=c++_static"
                arguments += "-DAPEX_USE_LIBOQS=OFF"
            }
        }
    }

    // ─── ABI 分包：产出独立 APK（按架构）──────────────────
    // arm64-v8a.apk / armeabi-v7a.apk / x86_64.apk
    // 用户按设备架构选择对应 APK 安装
    splits {
        abi {
            isEnable = true
            reset()
            include("arm64-v8a", "armeabi-v7a", "x86_64")
            isUniversalApk = false
        }
    }

    buildTypes {
        debug {
            if (hasSigningCreds) {
                signingConfig = signingConfigs.getByName("release")
            }
            // 无 keystore 时使用默认 debug 签名，确保开箱可构建
        }
        release {
            // 启用混淆与资源压缩: 移除未使用代码,减小APK体积,提高逆向难度
            // (此前因调试需要关闭,现已恢复 — proguard-rules.pro 已加全 JNI keep 规则)
            isMinifyEnabled = true
            isShrinkResources = true
            if (hasSigningCreds) {
                signingConfig = signingConfigs.getByName("release")
            }
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    composeOptions {
        kotlinCompilerExtensionVersion = "1.5.4"
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }


}

protobuf {
    protoc {
        artifact = "com.google.protobuf:protoc:3.25.1"
    }
    generateProtoTasks {
        all().forEach { task ->
            task.builtins {
                create("java") {
                    option("lite")
                }
                create("kotlin") {
                    option("lite")
                }
            }
        }
    }
}

dependencies {
    implementation(platform("androidx.compose:compose-bom:2023.10.01"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    implementation("androidx.compose.ui:ui-tooling-preview")
    debugImplementation("androidx.compose.ui:ui-tooling")

    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.7.0")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.7.0")
    implementation("androidx.lifecycle:lifecycle-process:2.7.0")
    implementation("androidx.activity:activity-compose:1.8.2")
    implementation("androidx.navigation:navigation-compose:2.7.6")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3")

    // WorkManager — 周期性更新检查
    implementation("androidx.work:work-runtime-ktx:2.9.0")

    implementation("com.google.protobuf:protobuf-kotlin-lite:3.25.1")

    // Unit testing
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.7.3")
    testImplementation("io.mockk:mockk:1.13.8")

    // Instrumentation testing
    androidTestImplementation(platform("androidx.compose:compose-bom:2023.10.01"))
    androidTestImplementation("androidx.compose.ui:ui-test-junit4")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
}

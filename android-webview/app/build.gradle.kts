import java.util.zip.ZipFile

plugins {
    id("com.android.application")
}

val stableTestKeystorePath = providers.environmentVariable("IB_TEST_KEYSTORE").orNull
val stableTestKeystorePassword = providers.environmentVariable("IB_TEST_KEYSTORE_PASSWORD").orNull
    ?: "wegert-debug"
val stableTestKeyPassword = providers.environmentVariable("IB_TEST_KEY_PASSWORD").orNull
    ?: stableTestKeystorePassword
val stableTestKeyAlias = providers.environmentVariable("IB_TEST_KEY_ALIAS").orNull
    ?: "wegert-debug"
val ibSourceHead = providers.environmentVariable("IB_SOURCE_HEAD").orNull ?: "unknown"

android {
    namespace = "org.isomorphisms.ib.webview"
    compileSdk = 36

    buildFeatures {
        buildConfig = true
    }

    defaultConfig {
        applicationId = "org.isomorphisms.ib.webview"
        minSdk = 26
        targetSdk = 36
        versionCode = 7
        versionName = "0.7.0"
        buildConfigField("String", "IB_SOURCE_HEAD", "\"$ibSourceHead\"")
    }

    signingConfigs {
        stableTestKeystorePath?.let { keystorePath ->
            create("stableTest") {
                storeFile = rootProject.file(keystorePath)
                storePassword = stableTestKeystorePassword
                keyAlias = stableTestKeyAlias
                keyPassword = stableTestKeyPassword
                storeType = "pkcs12"
            }
        }
    }

    buildTypes {
        getByName("debug") {
            // No environment-provided stable signer means an unsigned debug APK,
            // never an AGP-generated machine-local debug identity.
            signingConfig = signingConfigs.findByName("stableTest")
        }
        getByName("release") {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

dependencies {
    implementation("com.google.android.gms:play-services-auth:21.5.0")
    testImplementation("junit:junit:4.13.2")
}

tasks.register("verifyWebViewBoundary") {
    dependsOn("testDebugUnitTest", "assembleDebug")
    doLast {
        val implementationFiles = fileTree("src/main") {
            include("**/*.java", "**/*.xml")
        }
        val implementation = implementationFiles.files.joinToString("\n") { it.readText() }

        check(implementation.contains("onRenderProcessGone")) {
            "The acceptance harness must observe renderer death explicitly."
        }
        check(implementation.contains("RENDERER_PRIORITY_IMPORTANT")) {
            "Protected transactions must request important renderer priority."
        }
        check(implementation.contains("android:name=\".LongViewActivity\"")) {
            "The real launcher must enter the protected long-view path."
        }
        check(implementation.contains("android:launchMode=\"singleTask\"")) {
            "Live-renderer return must reuse the existing long-view activity."
        }
        check(implementation.contains("DurableTaskStore")) {
            "Long-view tasks must use browser-owned durable records."
        }
        val longView = file("src/main/java/org/isomorphisms/ib/webview/LongViewActivity.java").readText()
        check(!longView.contains("enterPictureInPictureMode(")) {
            "Picture-in-Picture must not be required for long-view correctness."
        }
        check(!longView.contains("IncrementalLoadService")) {
            "A foreground-service survival experiment must not define long-view correctness."
        }
        check(implementation.contains("setSaveEnabled(false)")) {
            "The WebView hierarchy must not become the hidden form persistence mechanism."
        }
        check(!implementation.contains(".saveState(")) {
            "Do not use WebView.saveState() as acceptance evidence."
        }
        check(!implementation.contains("getSharedPreferences")) {
            "The first fixture must not persist form state through SharedPreferences."
        }
        check(implementation.contains("android:allowBackup=\"false\"")) {
            "The acceptance app must not back up fixture state."
        }

        check(implementation.contains("requestOfflineAccess")) {
            "Drive authorization must request the one-time server code."
        }
        check(!implementation.contains("android.intent.category.BROWSABLE")) {
            "Drive authorization must not be a browser link."
        }

        val apks = fileTree("build/outputs/apk/debug") { include("*.apk") }.files
        check(apks.size == 1) { "Expected exactly one debug APK, found ${apks.size}." }
        ZipFile(apks.single()).use { apk ->
            check(apk.entries().asSequence().none { it.name.startsWith("lib/") }) {
                "This authorization APK must remain native-ABI neutral."
            }
        }
    }
}

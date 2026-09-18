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

android {
    namespace = "org.isomorphisms.ib.webview"
    compileSdk = 36

    defaultConfig {
        applicationId = "org.isomorphisms.ib.webview"
        minSdk = 26
        targetSdk = 36
        versionCode = 1
        versionName = "0.1.0"
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

        val apks = fileTree("build/outputs/apk/debug") { include("*.apk") }.files
        check(apks.size == 1) { "Expected exactly one debug APK, found ${apks.size}." }
    }
}

plugins {
    id("com.android.application")
}

android {
    namespace = "org.isomorphisms.ib.webview"
    compileSdk = 36

    defaultConfig {
        applicationId = "org.isomorphisms.ib.webview"
        minSdk = 26
        targetSdk = 36
        versionCode = 4
        versionName = "0.4.0"
    }

    buildTypes {
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
        check(implementation.contains("android:supportsPictureInPicture=\"true\"")) {
            "Unattended loading must keep the WebView visibly attached in Picture-in-Picture."
        }
        check(implementation.contains("setAutoEnterEnabled(true)")) {
            "Leaving the activity must enter Picture-in-Picture on Android 12+."
        }
        check(!implementation.contains("moveTaskToBack(true)")) {
            "Do not deliberately hide the incremental WebView after physical renderer eviction."
        }
        check(implementation.contains("startForeground(")) {
            "Incremental background loading must keep its host process active."
        }
        check(implementation.contains("android:launchMode=\"singleTask\"")) {
            "Launcher re-entry must reuse the existing incremental page activity."
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

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
    namespace = "org.isomorphisms.ib.prepaint"
    compileSdk = 36

    defaultConfig {
        applicationId = "org.isomorphisms.ib.prepaint"
        minSdk = 26
        targetSdk = 36
        versionCode = 2
        versionName = "0.2.0"
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

tasks.register("verifyPrepaintBoundary") {
    dependsOn("assembleDebug")
    doLast {
        val implementationFiles = fileTree("src/main") {
            include("**/*.java", "**/*.xml")
        }
        val implementation = implementationFiles.files.joinToString("\n") { it.readText() }
        check(!implementation.contains("android.webkit")) {
            "The prepaint harness must not depend on android.webkit."
        }
        check(!implementation.contains("WebView")) {
            "The prepaint harness must not contain a WebView."
        }
        check(!file("src/main/AndroidManifest.xml").readText()
            .contains("android.permission.INTERNET")) {
            "The prepaint harness must not request Internet access."
        }

        val apks = fileTree("build/outputs/apk/debug") { include("*.apk") }.files
        check(apks.size == 1) { "Expected exactly one debug APK, found ${apks.size}." }
        check(apks.single().length() <= 2L * 1024L * 1024L) {
            "Debug APK exceeds the 2 MiB harness budget."
        }
    }
}

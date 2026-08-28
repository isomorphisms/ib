plugins {
    id("com.android.application")
}

android {
    namespace = "org.isomorphisms.ib.prepaint"
    compileSdk = 36

    defaultConfig {
        applicationId = "org.isomorphisms.ib.prepaint"
        minSdk = 26
        targetSdk = 36
        versionCode = 3
        versionName = "0.3.0"

        ndk {
            abiFilters += "armeabi-v7a"
        }
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

    sourceSets {
        getByName("main") {
            jniLibs.srcDir("build/generated/jniLibs")
        }
    }
}

dependencies {
    testImplementation("junit:junit:4.13.2")
}

val buildNativeArmv7 by tasks.registering(Exec::class) {
    workingDir(rootProject.projectDir)
    commandLine("bash", "build-native-armv7.sh")
}

tasks.named("preBuild") {
    dependsOn(buildNativeArmv7)
}

tasks.register("verifyPrepaintBoundary") {
    dependsOn("assembleDebug")
    doLast {
        val implementationFiles = fileTree("src/main") {
            include("**/*.java", "**/*.xml", "**/*.d", "**/*.c")
        }
        val implementation = implementationFiles.files.joinToString("\n") { it.readText() }
        check(!implementation.contains("android.webkit")) {
            "The prepaint harness must not depend on android.webkit."
        }
        check(!implementation.contains("WebView")) {
            "The prepaint harness must not contain a WebView."
        }
        check(file("src/main/AndroidManifest.xml").readText()
            .contains("android.permission.INTERNET")) {
            "The native viewer must request Internet access for the D transport."
        }

        val packagedJava = fileTree("src/main/java") { include("**/*.java") }.files
        check(packagedJava.size == 1 && packagedJava.single().name == "IbActivity.java") {
            "Only the IbActivity callback router may be packaged as Java."
        }
        val javaBridge = packagedJava.single().readText()
        check(javaBridge.lineSequence().count() <= 40) {
            "The Java callback router grew beyond its 40-line boundary."
        }
        check(!javaBridge.contains("java.io") &&
              !javaBridge.contains("java.net") &&
              !javaBridge.contains("android.widget")) {
            "I/O, networking, and View construction belong in D."
        }
        check(implementation.contains("ANativeActivity_onCreate") &&
              implementation.contains("-betterC")) {
            "The D native entry point or runtime-free build boundary is missing."
        }

        val apks = fileTree("build/outputs/apk/debug") { include("*.apk") }.files
        check(apks.size == 1) { "Expected exactly one debug APK, found ${apks.size}." }
        val apk = apks.single()
        check(apk.length() <= 2L * 1024L * 1024L) {
            "Debug APK exceeds the 2 MiB harness budget."
        }

        java.util.zip.ZipFile(apk).use { archive ->
            val entries = mutableListOf<java.util.zip.ZipEntry>()
            val enumeration = archive.entries()
            while (enumeration.hasMoreElements()) entries += enumeration.nextElement()
            val nativeLibraries = entries.map { it.name }
                .filter { it.startsWith("lib/") && it.endsWith(".so") }
            check(nativeLibraries == listOf("lib/armeabi-v7a/libib.so")) {
                "Expected one ARMv7 native library, found $nativeLibraries."
            }
            val dex = entries.singleOrNull { it.name == "classes.dex" }
                ?: error("The callback router DEX is missing.")
            check(dex.size <= 64L * 1024L) {
                "The callback router DEX exceeds 64 KiB."
            }
        }
    }
}

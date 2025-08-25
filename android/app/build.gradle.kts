val NDK_VERSION by extra(project.properties["NDK_VERSION"] as? String ?: "26.1.10909125")
val MIN_SDK by extra((project.properties["MIN_SDK"] as? String ?: "21").toInt())
val TARGET_SDK by extra((project.properties["TARGET_SDK"] as? String ?: "33").toInt())
val STL_TYPE by extra(project.properties["STL_TYPE"] as? String ?: "c++_shared")
val SFML_STATIC by extra(project.properties["SFML_STATIC"] as? String ?: "OFF")
val CMAKE_VERSION by extra(project.properties["CMAKE_VERSION"] as? String ?: "3.30.5")

plugins {
    id("com.android.application")
}

android {
    namespace = "org.cc3dsfs.android"
    ndkVersion = NDK_VERSION
    compileSdk = TARGET_SDK
    defaultConfig {
        applicationId = "org.cc3dsfs.android"
        minSdk = MIN_SDK
        targetSdk = TARGET_SDK
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                arguments.add("-DCMAKE_SYSTEM_NAME=Android")
                arguments.add("-DANDROID_STL=${STL_TYPE}")
                arguments.add("-DSFML_STATIC_LIBRARIES=${SFML_STATIC}")
            }
        }
    }
    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android.txt"), "proguard-rules.pro")
        }
    }
    bundle {
		language {
		    enableSplit = false
		}
		density {
		    enableSplit = false
		}
		abi {
		    enableSplit = true
		}
	}
    externalNativeBuild {
        cmake {
            path("../../CMakeLists.txt")
            version = "${CMAKE_VERSION}"
        }
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
}

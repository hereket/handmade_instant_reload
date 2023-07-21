
SDK="/opt/Android/Sdk"
BUILD_TOOLS="${SDK}/build-tools/29.0.2"
PLATFORM="${SDK}/platforms/android-33"
NDK="${SDK}/ndk/r25b"
BUILD_DIR="__build"


ARM_TOOLCHAIN="${NDK}/toolchains/llvm/prebuilt/"
ARM_TOOLCHAIN+="linux-x86_64/bin/armv7a-linux-androideabi23-clang++"


MY_LOCAL_LDFLAGS='-ljnigraphics -llog -landroid'
DEFINES=" -DHANDMADE_SLOW" 
"${ARM_TOOLCHAIN}" $DEFINES -g -O0 -fPIC -shared -static-libstdc++ -o $BUILD_DIR/apk/lib/armeabi-v7a/libandroid_handmade.so jni/android_handmade.cpp $MY_LOCAL_LDFLAGS
"${ARM_TOOLCHAIN}" $DEFINES -g -O0 -fPIC -shared -static-libstdc++ -o $BUILD_DIR/apk/lib/armeabi-v7a/libhandmade.so jni/handmade.cpp $MY_LOCAL_LDFLAGS


# # NOTE: On older phones 'nativeLibraryDir' was the correct way to get the path. Now it is different.
# ARM_LIB_DIR=$(adb shell dumpsys package com.hereket.handmade_hero | grep nativeLibraryDir | cut -d "=" -f2 | tr -d '\r')
ARM_LIB_DIR=$(adb shell dumpsys package com.hereket.handmade_hero | grep legacyNativeLibraryDir | cut -d "=" -f2 | tr -d '\r')

SO_LOCATION=$ARM_LIB_DIR/arm

adb push __build/apk/lib/armeabi-v7a/libhandmade.so $SO_LOCATION


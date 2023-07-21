set -e

SDK="/opt/Android/Sdk"
# BUILD_TOOLS="${SDK}/build-tools/29.0.2"
BUILD_TOOLS="${SDK}/build-tools/33.0.2"
PLATFORM="${SDK}/platforms/android-33"

JAVA_HOME='/opt/android-studio/jbr'

BUILD_DIR=$(realpath "__build")
PROJECT_DIR=$(realpath ".")

NDK="${SDK}/ndk/r25b"

ARM_TOOLCHAIN="${NDK}/toolchains/llvm/prebuilt/"
ARM_TOOLCHAIN+="linux-x86_64/bin/armv7a-linux-androideabi23-clang++"

SYSROOT=${NDK}/sources/android/support/include


# rm -rf $BUILD_DIR

mkdir -p $BUILD_DIR/gen $BUILD_DIR/obj $BUILD_DIR/apk
mkdir -p $BUILD_DIR/apk/lib/armeabi-v7a $BUILD_DIR/apk/lib/x86

mkdir -p $BUILD_DIR/apk/lib/arm64-v8a 
mkdir -p $BUILD_DIR/apk/lib/arm


#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
#----          JNI                                                         ------
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------


MY_LOCAL_LDFLAGS='-ljnigraphics -llog -landroid'
DEFINES=" -DHANDMADE_SLOW" 
CFLAGS=" -g -O0 -fPIC -shared -static-libstdc++"

"${ARM_TOOLCHAIN}" $DEFINES $CFLAGS \
    -o $BUILD_DIR/apk/lib/armeabi-v7a/libandroid_handmade.so \
    jni/android_handmade.cpp \
    $MY_LOCAL_LDFLAGS


"${ARM_TOOLCHAIN}" $DEFINES $CFLAGS \
    -o $BUILD_DIR/apk/lib/armeabi-v7a/libhandmade.so \
    jni/handmade.cpp \
    $MY_LOCAL_LDFLAGS



#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
#----        Build APK                                                     ------
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------

javac \
    -classpath "${PLATFORM}/android.jar" \
    -d "$BUILD_DIR/obj" \
    java/com/hereket/handmade_hero/MainActivity.java


CLASS_FILES=$(find $BUILD_DIR/obj/ -iname "*.class")
# for x in $CLASS_FILES; do echo $x; done;
"${BUILD_TOOLS}/d8" $CLASS_FILES \
    --output $BUILD_DIR/apk/my_classes.jar \
    --no-desugaring \



pushd $BUILD_DIR/apk
# TODO: Merge d8 passes?
"${BUILD_TOOLS}/d8" my_classes.jar \
    ${PLATFORM}/android.jar \

popd



"${BUILD_TOOLS}/aapt" package -f -M AndroidManifest.xml -S res \
    -A assets \
    -I "${PLATFORM}/android.jar" \
    -F $BUILD_DIR/handmade_hero.unsigned.apk $BUILD_DIR/apk/



"${BUILD_TOOLS}/zipalign" -f -p 4 \
    $BUILD_DIR/handmade_hero.unsigned.apk $BUILD_DIR/handmade_hero.aligned.apk



"${BUILD_TOOLS}/apksigner" sign --ks keystore.jks \
    --ks-key-alias androidkey --ks-pass pass:android \
    --key-pass pass:android --out $BUILD_DIR/handmade_hero.apk \
    $BUILD_DIR/handmade_hero.aligned.apk




# ################################################################################
# ## RUN ON DEVICE
# ################################################################################
"${SDK}/platform-tools/adb" install -r $BUILD_DIR/handmade_hero.apk
"${SDK}/platform-tools/adb" shell am start -n com.hereket.handmade_hero/.MainActivity


#include <jni.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <time.h>
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "handmade.h"

/* __android_log_print(ANDROID_LOG_DEBUG, "----- TAG", "stride: %d, Width: %d  Height: %d", Stride, Width, Height); */

static int Initilalized;

typedef struct {
    char *AppDir;
    char *LibDir;
    AAssetManager *AssetManager;

} platform_state;



global_variable platform_state GameState;
global_variable game_input Input;
global_variable game_memory GlobalGameMemory;

game_update_and_render *GameUpdateAndRender;



long 
timespec_nano_diff(struct timespec start, struct timespec end) 
{
    long result = 0;
    struct timespec diff = {};

    long BILLION = 1000000000;

    if(end.tv_nsec < start.tv_nsec) {
        diff.tv_sec = end.tv_sec - start.tv_sec - 1;
        diff.tv_nsec = end.tv_nsec + start.tv_nsec;
    } else {
        diff.tv_sec = end.tv_sec - start.tv_sec;
        diff.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    result = diff.tv_sec * BILLION + diff.tv_nsec;

    return result;
}

bool32
IsCollided(rect Rect, v2 Point) {
    bool32 Result = 0;

    if(Rect.P.X < Point.X 
        && Rect.P.X + Rect.Size.X > Point.X
        && Rect.P.Y < Point.Y
        && Rect.P.Y + Rect.Size.Y > Point.Y)
    {
        Result = 1;
    }

    return Result;
}

bool32
IsCollided(button Button, v2 Point) {
    v2 P = {Button.PositionX, Button.PositionY};
    v2 Size = {Button.Width, Button.Height};
    rect Rect = {P, Size};
    bool32 Result = IsCollided(Rect, Point);
    return Result;
}

void
ClearWithZeros(uint8 *Memory, uint64 Size) {
    uint8* Pointer = Memory;

    for(int64 Index = 0; Index < Size; Index++) {
        *Pointer++ = 0;
    }
}


DEBUG_READ_ENTIRE_FILE(ReadEntireFile) {
    AAsset* Asset = AAssetManager_open(GameState.AssetManager, (char *)Path, AASSET_MODE_STREAMING);

    file_read_result Result = {};

    if(Asset) {
        Result.FileSize = AAsset_getLength(Asset);
        Result.Contents = (uint8 *)calloc(Result.FileSize, 1);

        int32 ReadStatus = AAsset_read(Asset, Result.Contents, Result.FileSize);
        if(ReadStatus < 0) {
            // TODO: Process error
        }
    }

    return Result;
}

void
ConcatStrings(uint8 *Dest, int32 DestSize, uint8 *Source, int32 SourceSize) {
    uint8 *DestPointer = Dest;
    uint8 *SourcePointer = Source;

    int32 DestOffset = 0;
    for(; DestOffset < DestSize; DestOffset++) {
        if(DestPointer[DestOffset] == 0) {
            break;
        }
        DestPointer++;
    }

    int32 MaxLength = DestSize - DestOffset;
    if(SourceSize < MaxLength) {
        MaxLength = SourceSize;
    }

    for(int32 Index = 0; Index < MaxLength; Index++) {
        *DestPointer = Source[Index];
        DestPointer++;
    }

    DestPointer = 0;
}

extern "C" {
JNIEXPORT void JNICALL 
Java_com_hereket_handmade_1hero_MainActivity_drawStuff(JNIEnv *env, jobject obj,
        jobject bitmap, jint width, jint height, jobject input)
{

    AndroidBitmapInfo info;
    buffer Buffer = {};

    jclass inputClass = env->GetObjectClass(input);
    void *GameLibrary = 0;

    char TAG[] = "---------- JNI";

    if(!Initilalized) {
        jfieldID packageDirField = env->GetFieldID(inputClass, "packageDirectory", "Ljava/lang/String;");
        jstring packageDirectoryJava = (jstring)env->GetObjectField(input, packageDirField);

        const char *packageDirectory = env->GetStringUTFChars(packageDirectoryJava, 0);
        const char *libDirName = "/lib";

        int packageDirectoryLength = strlen(packageDirectory);
        int libDirNameLength = strlen(libDirName);

        char *libDirPath = (char *)calloc(libDirNameLength + packageDirectoryLength + 1, 1);
        strcpy(libDirPath, packageDirectory);
        strcat(libDirPath, libDirName);

        GameState.AppDir = (char *)packageDirectory;
        GameState.LibDir = libDirPath;

        // GlobalGameMemory.PermanentStorageSize = MEGABYTE(64);
        // GlobalGameMemory.TransientStorageSize = GIGABYTE(1);

        GlobalGameMemory.PermanentStorageSize = MEGABYTE(64);
        GlobalGameMemory.TransientStorageSize = MEGABYTE(256);


        GlobalGameMemory.PermanentStorage = (uint8 *)calloc(GlobalGameMemory.PermanentStorageSize + GlobalGameMemory.TransientStorageSize, 1);
        ClearWithZeros(GlobalGameMemory.PermanentStorage, GlobalGameMemory.PermanentStorageSize + GlobalGameMemory.TransientStorageSize);

        GlobalGameMemory.TransientStorage = GlobalGameMemory.PermanentStorage + GlobalGameMemory.PermanentStorageSize;


        GlobalGameMemory.DEBUGReadEntireFile = ReadEntireFile;


        //--------------------------------------------------------------------------------
        // ASSETS
        
        jfieldID assetManagerField = env->GetFieldID(inputClass, "assetManager", "Landroid/content/res/AssetManager;");
        jobject assetManagerObject = (jobject)env->GetObjectField(input, assetManagerField);

        GameState.AssetManager = AAssetManager_fromJava(env, assetManagerObject);

        AAssetDir* AssetDir = AAssetManager_openDir(GameState.AssetManager, "test");

        uint8 *Filename = 0;



        uint8 AssetDirPath[] = "test/";
        int32 AssetDirSize = strlen((char *)AssetDirPath);

        while((Filename = (uint8 *)AAssetDir_getNextFileName(AssetDir)) != NULL) {
            uint8 Path[BUFSIZ] = {};

            ConcatStrings(Path, BUFSIZ, AssetDirPath, AssetDirSize);
            ConcatStrings(Path, BUFSIZ, Filename, strlen((char *)Filename));

            ReadEntireFile(Path);
        }

        //--------------------------------------------------------------------------------



        DIR *Directory;
        struct dirent *DirectoryEntry;
        Directory = opendir(GameState.LibDir);
        if(Directory) {
            while((DirectoryEntry = readdir(Directory)) != NULL) {
                // __android_log_print(ANDROID_LOG_DEBUG, TAG, "%s\n", DirectoryEntry->d_name);
            }

            closedir(Directory);
        }

        Initilalized = 1;
    }

    if(GameLibrary) {
        dlclose(GameLibrary);
    }
    GameLibrary = dlopen("libhandmade.so", RTLD_LAZY);
    if(GameLibrary) {
        GameUpdateAndRender = (game_update_and_render *)dlsym(GameLibrary, "GameUpdateAndRender");
    }

    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    jfieldID touchXField = env->GetFieldID(inputClass, "touchX", "F");
    jfloat touchX = env->GetFloatField(input, touchXField);

    jfieldID touchYField = env->GetFieldID(inputClass, "touchY", "F");
    jfloat touchY = env->GetFloatField(input, touchYField);

    jfieldID canvasWidthField = env->GetFieldID(inputClass, "canvasWidth", "I");
    jint canvasWidth = env->GetIntField(input, canvasWidthField);

    jfieldID canvasHeightField = env->GetFieldID(inputClass, "canvasHeight", "I");
    jint canvasHeight = env->GetIntField(input, canvasHeightField);

    jfieldID dtForFrameField = env->GetFieldID(inputClass, "dtForFrame", "D");
    jdouble dtForFrame = env->GetDoubleField(input, dtForFrameField);


    if(touchX < 0) { Input.Controllers[0].IsTouching = 0; }
    else           { Input.Controllers[0].IsTouching = 1; }

    if(Input.Controllers[0].IsTouching) {
        Input.Controllers[0].TouchX = touchX;
        Input.Controllers[0].TouchY = touchY;
    }

    Input.dtForFrame = dtForFrame;



    if(AndroidBitmap_getInfo(env, bitmap, &info) >= 0) {
        if(AndroidBitmap_lockPixels(env, bitmap, (void **)&Buffer.Memory) >= 0) {

            Buffer.Stride = info.stride;
            Buffer.Width = info.width;
            Buffer.Height = info.height;

            real32 Size = 70.0f;
            button Right = {(real32)Buffer.Width - Size, Buffer.Height - Size*2, Size, Size};
            button Top = {Right.PositionX - Size, Right.PositionY - Size, Size, Size};
            button Bottom = {Top.PositionX, Right.PositionY + Size, Size, Size};
            button Left = {Top.PositionX - Size, Right.PositionY, Size, Size};

            Input.Controllers[0].ButtonLeft = Left;
            Input.Controllers[0].ButtonRight = Right;
            Input.Controllers[0].ButtonTop = Top;
            Input.Controllers[0].ButtonBottom = Bottom;


            v2 TouchP = {touchX, touchY};

            Input.Controllers[0].ButtonLeft.IsPressed = IsCollided(Input.Controllers[0].ButtonLeft, TouchP);
            Input.Controllers[0].ButtonRight.IsPressed = IsCollided(Input.Controllers[0].ButtonRight, TouchP);
            Input.Controllers[0].ButtonTop.IsPressed = IsCollided(Input.Controllers[0].ButtonTop, TouchP);
            Input.Controllers[0].ButtonBottom.IsPressed = IsCollided(Input.Controllers[0].ButtonBottom, TouchP);

            GameUpdateAndRender(&Buffer, &Input, &GlobalGameMemory);
            AndroidBitmap_unlockPixels(env, bitmap);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    long timediff = timespec_nano_diff(start, end);
}
}

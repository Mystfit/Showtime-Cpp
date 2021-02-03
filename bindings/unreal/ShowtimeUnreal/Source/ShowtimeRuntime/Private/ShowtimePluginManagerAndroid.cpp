#include "ShowtimePluginManagerAndroid.h"
#include "Misc/LocalTimestampDirectoryVisitor.h"

#if PLATFORM_ANDROID 
jmethodID UShowtimePluginManagerAndroid::Showtime_GetShowtimePluginPath = NULL;

void UShowtimePluginManagerAndroid::InitPluginFunctions() {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        UE_LOG(Showtime, Display, TEXT("Inside InitPluginFunctions()"));

        UShowtimePluginManagerAndroid::Showtime_GetShowtimePluginPath = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID, "Showtime_GetShowtimePluginPath", "()Ljava/lang/String;", false);
        check(UShowtimePluginManagerAndroid::Showtime_GetShowtimePluginPath != NULL);
    }
}

FString UShowtimePluginManagerAndroid::GetPluginPath() {
	extern FString GExternalFilePath;
	FString plugin_path;// = GExternalFilePath;

    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        if (!UShowtimePluginManagerAndroid::Showtime_GetShowtimePluginPath) UShowtimePluginManagerAndroid::InitPluginFunctions();

        jstring result = (jstring)FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, UShowtimePluginManagerAndroid::Showtime_GetShowtimePluginPath);
        plugin_path = FJavaHelper::FStringFromParam(Env, result);

        UE_LOG(Showtime, Display, TEXT("Plugin path: %s"), *plugin_path);
    }

    return plugin_path;
}

#else
void UShowtimePluginManagerAndroid::InitPluginFunctions() {};
FString UShowtimePluginManagerAndroid::GetPluginPath() { return FString(); };
#endif

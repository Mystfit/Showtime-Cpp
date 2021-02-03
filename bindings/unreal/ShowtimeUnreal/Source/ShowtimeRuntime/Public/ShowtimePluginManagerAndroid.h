#pragma once

#include "CoreMinimal.h"
#if PLATFORM_ANDROID 
#include "Android/AndroidPlatform.h" 
#include "Android/AndroidApplication.h" 
#include "Android/AndroidJNI.h" 
#include "Android/AndroidJava.h" 
#endif
#include "ShowtimePluginManagerAndroid.generated.h"

UCLASS()
class UShowtimePluginManagerAndroid : public UObject {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Exec, Category = "ShowtimePlugins")
    static void InitPluginFunctions();

    /* Call java functions */
    UFUNCTION(BlueprintCallable, Exec, Category = "ShowtimePlugins")
    static FString GetPluginPath();

private:
#if PLATFORM_ANDROID 
    static jmethodID Showtime_GetShowtimePluginPath;
#endif
};

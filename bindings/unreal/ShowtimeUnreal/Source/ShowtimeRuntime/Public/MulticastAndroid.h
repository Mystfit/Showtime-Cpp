#pragma once

#include "CoreMinimal.h"
#include "IPAddress.h"

#if PLATFORM_ANDROID 
#include "Android/AndroidPlatform.h" 
#include "Android/AndroidApplication.h" 
#include "Android/AndroidJNI.h" 
#include "Android/AndroidJava.h" 
#endif
#include "MulticastAndroid.generated.h"

UCLASS()
class UMulticastAndroid : public UObject {
    GENERATED_BODY()
public:
    virtual void BeginDestroy() override;

    /* Call java functions */
    UFUNCTION(BlueprintCallable, Exec, Category = "MulticastAndroid")
    static void InitMulticastFunctions();

    static void GetLocalHostAddrFixed(TSharedRef<FInternetAddr> localip);
    static void GetBroadcastAddrFixed(FInternetAddr& broadcastaddr);

    UFUNCTION(BlueprintCallable, Exec, Category = "MulticastAndroid")
    static void AcquireMulticastLock();

    UFUNCTION(BlueprintCallable, Exec, Category = "MulticastAndroid")
    static void ReleaseMulticastLock();

private:
#if PLATFORM_ANDROID 
    static jmethodID Sockets_GetIP;
    static jmethodID Sockets_GetBroadcastIP;
    static jmethodID Sockets_AcquireMulticastLock;
    static jmethodID Sockets_ReleaseMulticastLock;
#endif
};

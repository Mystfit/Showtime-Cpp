#pragma once

#include "CoreMinimal.h"
#if PLATFORM_ANDROID 
#include "Android/AndroidPlatform.h" 
#include "Android/AndroidApplication.h" 
#include "Android/AndroidJNI.h" 
#include "Android/AndroidJava.h" 

class MulticastAndroid {
public:
    /* Call java functions */
    void InitMulticastFunctions();
    void GetLocalHostAddrFixed(TSharedRef<FInternetAddr> localip);
    void GetBroadcastAddrFixed(FInternetAddr& broadcastaddr);
    void AcquireMulticastLock();
    void ReleaseMulticastLock();

private:
    jmethodID Sockets_GetIP = NULL;
    jmethodID Sockets_GetBroadcastIP = NULL;
    jmethodID Sockets_AcquireMulticastLock = NULL;
    jmethodID Sockets_ReleaseMulticastLock = NULL;
};
#endif

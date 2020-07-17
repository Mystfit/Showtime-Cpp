#pragma once

#include "CoreMinimal.h"
#if PLATFORM_ANDROID 
#include "Android/AndroidPlatform.h" 
#include "Android/AndroidApplication.h" 
#include "Android/AndroidJNI.h" 
#include "Android/AndroidJava.h" 

namespace MulticastAndroid {
    static jmethodID Sockets_GetIP = NULL;
    static jmethodID Sockets_GetBroadcastIP = NULL;
    static jmethodID Sockets_AcquireMulticastLock = NULL;
    static jmethodID Sockets_ReleaseMulticastLock = NULL;


    /* Call java functions */
    void InitMulticastFunctions();
    void GetLocalHostAddrFixed(TSharedRef<FInternetAddr> localip);
    void GetBroadcastAddrFixed(FInternetAddr& broadcastaddr);
    void AcquireMulticastLock();
    void ReleaseMulticastLock();
}
#endif

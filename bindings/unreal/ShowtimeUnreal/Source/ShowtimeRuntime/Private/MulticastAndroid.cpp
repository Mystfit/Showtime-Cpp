#include "MulticastAndroid.h"

#if PLATFORM_ANDROID 
void MulticastAndroid::InitMulticastFunctions() {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        UE_LOG(Showtime, Display, TEXT("Inside InitMulticastFunctions()"));

        Sockets_GetIP = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_GetIP", "()I", false);
        check(Sockets_GetIP != NULL);

        Sockets_GetBroadcastIP = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_GetBroadcastIP", "()I", false);
        check(Sockets_GetBroadcastIP != NULL);

        Sockets_AcquireMulticastLock = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_AcquireMulticastLock", "()V", false);
        check(Sockets_AcquireMulticastLock != NULL);

        Sockets_AcquireMulticastLock = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_ReleaseMulticastLock", "()V", false);
        check(Sockets_AcquireMulticastLock != NULL);
    }
}

void MulticastAndroid::GetLocalHostAddrFixed(TSharedRef<FInternetAddr> localIp) {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        const int ip = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis,
            Sockets_GetIP);
        localIp->SetRawIp({
                uint8(ip & 0xff),
                uint8(ip >> 8 & 0xff),
                uint8(ip >> 16 & 0xff),
                uint8(ip >> 24 & 0xff) });
    }
}

void MulticastAndroid::GetBroadcastAddrFixed(FInternetAddr& broadcastAddr) {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        const int ip = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis,
            Sockets_GetBroadcastIP);
        broadcastAddr.SetRawIp({
                uint8(ip & 0xff),
                uint8(ip >> 8 & 0xff),
                uint8(ip >> 16 & 0xff),
                uint8(ip >> 24 & 0xff) });
    }
}

void MulticastAndroid::AcquireMulticastLock() {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis,
            FJavaWrapper::FindMethod(Env,
                FJavaWrapper::GameActivityClassID,
                "Sockets_AcquireMulticastLock", "()V", false));
        UE_LOG(Showtime, Display, TEXT("Attempting to aquire multicast lock"));
    }
}

void MulticastAndroid::ReleaseMulticastLock() {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis,
            FJavaWrapper::FindMethod(Env,
                FJavaWrapper::GameActivityClassID,
                "Sockets_ReleaseMulticastLock", "()V", false));
        UE_LOG(Showtime, Display, TEXT("Releasing multicast lock"));
    }
}
#endif
#include "MulticastAndroid.h"

void UMulticastAndroid::BeginDestroy() {

    ReleaseMulticastLock();
    UObject::BeginDestroy();
}


#if PLATFORM_ANDROID 
jmethodID UMulticastAndroid::Sockets_GetIP = NULL;
jmethodID UMulticastAndroid::Sockets_GetBroadcastIP = NULL;
jmethodID UMulticastAndroid::Sockets_AcquireMulticastLock = NULL;
jmethodID UMulticastAndroid::Sockets_ReleaseMulticastLock = NULL;

void UMulticastAndroid::InitMulticastFunctions() {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        UE_LOG(Showtime, Display, TEXT("Inside InitMulticastFunctions()"));

        UMulticastAndroid::Sockets_GetIP = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_GetIP", "()I", false);
        check(UMulticastAndroid::Sockets_GetIP != NULL);

        UMulticastAndroid::Sockets_GetBroadcastIP = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_GetBroadcastIP", "()I", false);
        check(UMulticastAndroid::Sockets_GetBroadcastIP != NULL);

        UMulticastAndroid::Sockets_AcquireMulticastLock = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_AcquireMulticastLock", "()V", false);
        check(UMulticastAndroid::Sockets_AcquireMulticastLock != NULL);

        UMulticastAndroid::Sockets_ReleaseMulticastLock = FJavaWrapper::FindMethod(Env,
            FJavaWrapper::GameActivityClassID,
            "Sockets_ReleaseMulticastLock", "()V", false);
        check(UMulticastAndroid::Sockets_AcquireMulticastLock != NULL);
    }
}

void UMulticastAndroid::GetLocalHostAddrFixed(TSharedRef<FInternetAddr> localIp) {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        if (!UMulticastAndroid::Sockets_GetIP) UMulticastAndroid::InitMulticastFunctions();

        const int ip = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis,
            UMulticastAndroid::Sockets_GetIP);
        localIp->SetRawIp({
                uint8(ip & 0xff),
                uint8(ip >> 8 & 0xff),
                uint8(ip >> 16 & 0xff),
                uint8(ip >> 24 & 0xff) });
    }
}

void UMulticastAndroid::GetBroadcastAddrFixed(FInternetAddr& broadcastAddr) {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        if (!UMulticastAndroid::Sockets_GetBroadcastIP) UMulticastAndroid::InitMulticastFunctions();

        const int ip = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis,
            UMulticastAndroid::Sockets_GetBroadcastIP);
        broadcastAddr.SetRawIp({
                uint8(ip & 0xff),
                uint8(ip >> 8 & 0xff),
                uint8(ip >> 16 & 0xff),
                uint8(ip >> 24 & 0xff) });
    }
}

void UMulticastAndroid::AcquireMulticastLock() {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        if (!UMulticastAndroid::Sockets_AcquireMulticastLock) UMulticastAndroid::InitMulticastFunctions();

        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis,
            FJavaWrapper::FindMethod(Env,
                FJavaWrapper::GameActivityClassID,
                "Sockets_AcquireMulticastLock", "()V", false));
        UE_LOG(Showtime, Display, TEXT("Attempting to aquire multicast lock"));
    }
}

void UMulticastAndroid::ReleaseMulticastLock() {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
        if (!UMulticastAndroid::Sockets_ReleaseMulticastLock) UMulticastAndroid::InitMulticastFunctions();

        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis,
            FJavaWrapper::FindMethod(Env,
                FJavaWrapper::GameActivityClassID,
                "Sockets_ReleaseMulticastLock", "()V", false));
        UE_LOG(Showtime, Display, TEXT("Releasing multicast lock"));
    }
}
#else
void UMulticastAndroid::InitMulticastFunctions() {};
void UMulticastAndroid::GetLocalHostAddrFixed(TSharedRef<FInternetAddr> localIp) {}
void UMulticastAndroid::GetBroadcastAddrFixed(FInternetAddr& broadcastAddr) {}
void UMulticastAndroid::AcquireMulticastLock() {}
void UMulticastAndroid::ReleaseMulticastLock() {}
#endif

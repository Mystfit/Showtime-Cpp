#include "MulticastAndroid.h"

namespace MulticastAndroid 
{
#if PLATFORM_ANDROID 
    void InitMulticastFunctions() {
        if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
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

    void GetLocalHostAddrFixed(TSharedRef<FInternetAddr> localIp) {
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

    void GetBroadcastAddrFixed(FInternetAddr& broadcastAddr) {
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

    void AcquireMulticastLock() {
        if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis,
                Sockets_AcquireMulticastLock);
            UE_LOG(Showtime, Display, TEXT("Attempting to aquire multicast lock"));
        }
    }

    void ReleaseMulticastLock() {
        if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) {
            FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis,
                Sockets_ReleaseMulticastLock);
            UE_LOG(Showtime, Display, TEXT("Releasing multicast lock"));
        }
    }
#endif
}

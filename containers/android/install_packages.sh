#!/bin/bash

echo "Installing packages"

# Packages
# --------

# Package tools
apt-get -y install -qq git zip apt-transport-https ca-certificates gnupg software-properties-common wget < /dev/null > /dev/null

# i386 arch to install some (32 bit) prerequisites for android builds 
dpkg --add-architecture i386


# CMake
# -----
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
apt-get update -qq
apt-get -y dist-upgrade -qq
apt-get -y install -qq cmake  < /dev/null > /dev/null

# Compiler toolchain
apt-get -y install -qq build-essential libc6-dev-i386 clang pkg-config < /dev/null > /dev/null


# Android SDK
# -----------
echo "Installing Android SDK"
apt-get -y install -qq libc6:i386 libncurses5:i386 libstdc++6:i386 libbz2-1.0:i386 openjdk-8-jdk > /dev/null
wget -q -O sdk_tools.zip https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip > /dev/null
unzip -q sdk_tools.zip -d $ANDROID_HOME < /dev/null > /dev/null

yes | $ANDROID_HOME/tools/bin/sdkmanager --sdk_root=$ANDROID_HOME --licenses >/dev/null
$ANDROID_HOME/tools/bin/sdkmanager --sdk_root=$ANDROID_HOME "platform-tools" "platforms;android-$ANDROID_PLATFORM" >/dev/null


# Android NDK
# -----------
echo "Installing Android NDK"
wget -q -O android_ndk.zip https://dl.google.com/android/repository/android-ndk-$ANDROID_NDK_SHORT_VERSION-linux-x86_64.zip > /dev/null
unzip -q android_ndk.zip -d $ANDROID_NDK_HOME < /dev/null > /dev/null

# SWIG
# ----
apt-get -y install -qq autoconf automake libtool libpcre3 libpcre3-dev bison flex < /dev/null > /dev/null

echo "Installing monoaot Swig"
git clone https://github.com/mystfit/swig.git
cd ./swig
git checkout monoaot
./autogen.sh >/dev/null
./configure --prefix=$INSTALL_PREFIX >/dev/null
make >/dev/null
make install >/dev/null
cd ..

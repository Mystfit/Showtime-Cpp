#!/bin/bash

echo "Building dependencies"

#LibSodium
# echo "Building libsodium"
# git clone https://github.com/jedisct1/libsodium < /dev/null > /dev/null
# git -C ./czmq libsodium stable
# pushd ./libsodium
# ./configure
# make && make install
# popd

# ZEROMQ
# ------
echo "Building libZMQ"
git clone https://github.com/mystfit/libzmq.git < /dev/null > /dev/null
mkdir -p ./libzmq/build
echo "libZMQ configure command line args:"
echo "cmake -H\"./libzmq\" -B\"./libzmq/build\" $ANDROID_BUILD_FLAGS $COMMON_BUILD_FLAGS -DBUILD_SHARED=OFF -DBUILD_STATIC=ON -DENABLE_DRAFTS=TRUE -DZMQ_BUILD_TESTS=OFF"
cmake -H"./libzmq" -B"./libzmq/build" $ANDROID_BUILD_FLAGS $COMMON_BUILD_FLAGS -DWITH_DOCS=OFF  -DWITH_LIBSODIUM=OFF -DENABLE_CURVE=OFF -DBUILD_SHARED=OFF -DBUILD_STATIC=ON -DENABLE_DRAFTS=TRUE -DZMQ_BUILD_TESTS=OFF
echo "libZMQ build command line args:"
echo "cmake --build ./libzmq/build -j $VM_CPU_COUNT --target install"
cmake --build ./libzmq/build -j $VM_CPU_COUNT --target install

# CZMQ
# ----
echo "Building CZMQ"
echo "Current path is $PWD"
git clone https://github.com/mystfit/czmq.git < /dev/null > /dev/null
git -C ./czmq checkout android
mkdir -p ./czmq/build
echo "CZMQ configure command line args:"
echo "cmake -H\"./czmq\" -B\"./czmq/build\" -DCMAKE_VERBOSE_MAKEFILE=ON $ANDROID_BUILD_FLAGS $COMMON_BUILD_FLAGS -DENABLE_DRAFTS=TRUE -DBUILD_TESTING=OFF -DCZMQ_BUILD_SHARED=OFF -DCZMQ_BUILD_STATIC=ON"
cmake -H"./czmq" -B"./czmq/build" -DCMAKE_VERBOSE_MAKEFILE=ON $ANDROID_BUILD_FLAGS $COMMON_BUILD_FLAGS -DENABLE_DRAFTS=TRUE -DBUILD_TESTING=OFF -DCZMQ_BUILD_SHARED=OFF -DCZMQ_BUILD_STATIC=ON
echo "CZMQ build command line args:"
echo "cmake --build ./czmq/build -j $VM_CPU_COUNT --target install"
cmake --build ./czmq/build -j $VM_CPU_COUNT --target install

# Flatbuffers - built twice (FlatC for host and cross-compiled libs for android)
# ----
echo "Building Flatbuffers"
git clone https://github.com/google/flatbuffers.git < /dev/null > /dev/null
git -C ./flatbuffers fetch 
git -C ./flatbuffers fetch --tags 
git -C ./flatbuffers checkout v1.12.0
mkdir -p ./flatbuffers/android_build
mkdir -p ./flatbuffers/host_build
cmake -H"./flatbuffers" -B"./flatbuffers/android_build" $ANDROID_BUILD_FLAGS $COMMON_BUILD_FLAGS -DFLATBUFFERS_BUILD_TESTS=OFF -DFLATBUFFERS_BUILD_FLATLIB=ON > /dev/null #-DFLATBUFFERS_BUILD_FLATC=OFF -DFLATBUFFERS_BUILD_FLATHASH=OFF > /dev/null
cmake -H"./flatbuffers" -B"./flatbuffers/host_build" $HOST_BUILD_FLAGS $COMMON_BUILD_FLAGS -DFLATBUFFERS_BUILD_TESTS=OFF > /dev/null #-DFLATBUFFERS_BUILD_FLATLIB=OFF 
cmake --build ./flatbuffers/android_build -j $VM_CPU_COUNT --target install > /dev/null
cmake --build ./flatbuffers/host_build -j $VM_CPU_COUNT --target install > /dev/null

# FMT
# ----
echo "Building fmt"
git clone https://github.com/fmtlib/fmt.git < /dev/null > /dev/null
mkdir -p ./fmt/build
cmake -H"./fmt" -B"./fmt/build" $ANDROID_BUILD_FLAGS $COMMON_BUILD_FLAGS -DFMT_TEST=FALSE > /dev/null
cmake --build ./fmt/build -j $VM_CPU_COUNT --target install > /dev/null

# BOOST
# -----
echo "Building Boost"
git clone https://github.com/mystfit/Boost-for-Android.git < /dev/null > /dev/null
pushd ./Boost-for-Android
BOOST_LIBS="log,thread,system,context,fiber,date_time,chrono,atomic,regex,test"
echo "Building boost for android $ANDROID_BOOST_VER"
./build-android.sh --boost=$ANDROID_BOOST_VER --layout=versioned --with-libraries=$BOOST_LIBS --arch=$ANDROID_ABI --prefix=$ANDROID_NDK_SYSROOT $ANDROID_NDK_ROOT
popd

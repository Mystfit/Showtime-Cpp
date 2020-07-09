#!/bin/bash

CMAKE_ANDROID_BOOST_FLAGS="-DBoost_DEBUG=FALSE 							\
						   -DBoost_COMPILER=clang 						\
						   -DBoost_VERSION=$ANDROID_BOOST_VER_BASE 		\
						   -DBoost_ROOT=$ANDROID_NDK_SYSROOT/$ANDROID_ABI"

LIBRARY_BUILD_FLAGS="-DBUILD_DRAFTS=FALSE 					\
	 				 -DBUILD_STATIC=TRUE 					\
	 				 -DBUILD_SHARED=FALSE 					\
	 				 -DBUILD_TESTING=FALSE 					\
	 				 -DBUILD_SHARED_FROM_STATIC_LIBS=TRUE	\
	 				 -DBINDINGS_DOTNET=FALSE 				\
	 				 -DBINDINGS_DOTNET_CSPROJ=TRUE 			\
	 				 -DBINDINGS_DOTNET_BUILD_PACKAGE=FALSE 	\
	 				 -DBUILD_SERVER_LAUNCHER=FALSE"

cmake -H"$SHOWTIME_SOURCE" -B"$SHOWTIME_BUILD" $ANDROID_BUILD_FLAGS $COMMON_BUILD_FLAGS $CMAKE_ANDROID_BOOST_FLAGS $LIBRARY_BUILD_FLAGS
cmake --build $SHOWTIME_BUILD -j $VM_CPU_COUNT --target all

echo "Copying binaries to $SHOWTIME_BIN_OUTPUT"
cp "$SHOWTIME_BUILD/bin/Android/*" "$SHOWTIME_BIN_OUTPUT" 2>/dev/null || :

echo "Copying libraries to $SHOWTIME_LIB_OUTPUT"
cp "$SHOWTIME_BUILD/lib/Android/*" "$SHOWTIME_LIB_OUTPUT" 2>/dev/null || :
cp "/home/vagrant/czmq/build/libczmq.so" "$SHOWTIME_LIB_OUTPUT" 2>/dev/null || :
cp "/home/vagrant/libzmq/build/lib/libzmq.so" "$SHOWTIME_LIB_OUTPUT" 2>/dev/null || :

#!/bin/bash
cmake --build $SHOWTIME_BUILD -j $VM_CPU_COUNT --target all

echo "Copying binaries to $SHOWTIME_BIN_OUTPUT"
cp -r "$SHOWTIME_BUILD/bin/Android/"* "$SHOWTIME_BIN_OUTPUT"

echo "Copying libraries to $SHOWTIME_LIB_OUTPUT"
cp "$SHOWTIME_BUILD/lib/Android/"* "$SHOWTIME_LIB_OUTPUT"
# cp "/home/vagrant/czmq/build/libczmq.so" "$SHOWTIME_LIB_OUTPUT"
# cp "/home/vagrant/libzmq/build/lib/libzmq.so" "$SHOWTIME_LIB_OUTPUT"

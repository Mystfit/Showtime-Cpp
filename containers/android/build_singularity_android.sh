singularity run -B $SHOWTIME_SOURCE:/showtime-source,$SHOWTIME_BIN_OUTPUT:/showtime-bin-output,$SHOWTIME_LIB_OUTPUT:/showtime-lib-output,$SHOWTIME_BUILD:/showtime-build $VM_DIR/build-android.sif

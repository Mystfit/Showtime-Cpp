if (NOT SUBST_SOURCE)
    message(FATAL_ERROR "Must set -D SUBST_SOURCE=source_file")
endif()
if (NOT SUBST_DEST)
    message(FATAL_ERROR "Must set -D SUBST_DEST=dest_file")
endif()

# Find boost shared runtimes (windows only)
file(GLOB BOOST_RUNTIMES ${BOOST_DLL_PATTERN})
list(APPEND ZST_RUNTIMES ${BOOST_RUNTIMES})

MESSAGE(STATUS "Formatting setup.py - ${SUBST_SOURCE}  ${SUBST_DEST}")
MESSAGE(STATUS "Using build-time vars:")
MESSAGE(STATUS "ZST_LIB_DIRS: ${ZST_LIB_DIRS}")
MESSAGE(STATUS "ZST_LIBS: ${ZST_LIBS}")
MESSAGE(STATUS "ZST_FLAGS: ${ZST_FLAGS}")
MESSAGE(STATUS "ZST_INCLUDE_DIRS: ${ZST_INCLUDE_DIRS}")
MESSAGE(STATUS "ZST_RUNTIMES: ${ZST_RUNTIMES}")

configure_file("${SUBST_SOURCE}" "${SUBST_DEST}" @ONLY)
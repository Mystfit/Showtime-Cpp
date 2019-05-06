if (NOT SUBST_SOURCE)
    message(FATAL_ERROR "Must set -D SUBST_SOURCE=source_file")
endif()
if (NOT SUBST_DEST)
    message(FATAL_ERROR "Must set -D SUBST_DEST=dest_file")
endif()

message(STATUS "Formatting setup.py - ${SUBST_SOURCE}  ${SUBST_DEST}")
configure_file("${SUBST_SOURCE}" "${SUBST_DEST}" @ONLY)

find_program(JRUBY_EXECUTABLE jruby)
if(DEFINED ${JRUBY_EXECUTABLE})
    execute_process(
        COMMAND "${JRUBY_EXECUTABLE}" -e "puts JRUBY_VERSION"
        OUTPUT_VARIABLE JRUBY_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

find_package_handle_standard_args(JRuby
    REQUIRED_VARS JRUBY_EXECUTABLE
    VERSION_VAR JRUBY_VERSION
    FAIL_MESSAGE "Failed to find correct JRuby"
)

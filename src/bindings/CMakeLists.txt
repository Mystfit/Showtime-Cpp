option(BINDINGS_RUBY "Build python bindings" OFF)
option(BINDINGS_PYTHON "Build ruby bindings" OFF)

if(BINDINGS_RUBY)
	add_subdirectory(ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_PYTHON)
	add_subdirectory(python)
endif(BINDINGS_PYTHON)
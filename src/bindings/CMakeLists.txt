# SWIG bindings
option(BINDINGS_RUBY "Build python bindings" OFF)
option(BINDINGS_PYTHON "Build ruby bindings" OFF)
option(BINDINGS_DOTNET "Build C# bindings" OFF)

if(BINDINGS_RUBY)
	add_subdirectory(ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_PYTHON)
    FIND_PACKAGE(PythonLibs 2.7.12 REQUIRED)
	INCLUDE(FindPythonInterp)
	get_filename_component(PYTHON_DIR ${PYTHON_EXECUTABLE} DIRECTORY)	
	add_subdirectory(python)
endif(BINDINGS_PYTHON)

if(BINDINGS_RUBY)
	add_subdirectory(ruby)
endif(BINDINGS_RUBY)

if(BINDINGS_DOTNET)
	add_subdirectory(dotnet)
endif(BINDINGS_DOTNET)
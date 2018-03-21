# SWIG bindings

option(BINDINGS_RUBY "Build python bindings" OFF)
option(BINDINGS_PYTHON "Build ruby bindings" OFF)
option(BINDINGS_DOTNET "Build C# bindings" OFF)

if(BINDINGS_RUBY OR BINDINGS_PYTHON OR BINDINGS_DOTNET)
	find_package(SWIG)

	if(SWIG_FOUND)
		include(UseSWIG)
		
		if(BINDINGS_PYTHON)
			add_subdirectory(python)
		endif(BINDINGS_PYTHON)

		if(BINDINGS_RUBY)
			add_subdirectory(ruby)
		endif(BINDINGS_RUBY)

		if(BINDINGS_DOTNET)
			add_subdirectory(dotnet)
		endif(BINDINGS_DOTNET)
	endif(SWIG_FOUND)
endif()

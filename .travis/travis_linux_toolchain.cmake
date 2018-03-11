INCLUDE(CMakeForceCompiler)

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
#SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
set(CMAKE_C_COMPILER /home/linuxbrew/.linuxbrew/bin/gcc CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER /home/linuxbrew/.linuxbrew/bin/g++ CACHE STRING "" FORCE)
set(CMAKE_LINKER /home/linuxbrew/.linuxbrew/bin/ld CACHE STRING "" FORCE)
set(LINKER_RULE "<CMAKE_LINKER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE ${LINKER_RULE})
set(CMAKE_CXX_LINK_SHARED ${LINKER_RULE})
set(CMAKE_CXX_LINK_MODULE ${LINKER_RULE})

# Set these vars as active to skip the compiler check
# SET(CMAKE_C_COMPILER_WORKS 1 CACHE INTERNAL "") 
# SET(CMAKE_CXX_COMPILER_WORKS 1 CACHE INTERNAL "")

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH /home/linuxbrew/.linuxbrew)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

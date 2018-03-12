# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
#SET(CMAKE_SYSTEM_VERSION 1)

# set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# specify the cross compiler
set(CMAKE_C_COMPILER /home/linuxbrew/.linuxbrew/bin/gcc CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER /home/linuxbrew/.linuxbrew/bin/g++ CACHE STRING "" FORCE)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH /home/linuxbrew/.linuxbrew)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

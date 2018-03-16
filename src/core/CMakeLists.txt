message(STATUS "Building ${CORE_TARGET} target")

# Write version info into header file
set(VERSION_FILE "${Showtime_COMPILED_INCLUDE_DIRS}/ZstVersion.h")
FILE(WRITE ${VERSION_FILE} "#define SHOWTIME_VERSION \"${SHOWTIME_VERSION}\"")

# Public include files
set(API_FILES
    ${VERSION_FILE}
    ${Showtime_INCLUDE_DIRS}/ZstCore.h
    ${Showtime_INCLUDE_DIRS}/ZstLogging.h
    ${Showtime_INCLUDE_DIRS}/ZstCable.h
    ${Showtime_INCLUDE_DIRS}/ZstConstants.h
    ${Showtime_INCLUDE_DIRS}/ZstExports.h
    ${Showtime_INCLUDE_DIRS}/ZstURI.h
    ${Showtime_INCLUDE_DIRS}/ZstSerialisable.h
    ${Showtime_INCLUDE_DIRS}/ZstSynchronisable.h
    ${Showtime_INCLUDE_DIRS}/ZstEvents.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstEntityBase.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstComponent.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstContainer.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPlug.h
    ${Showtime_INCLUDE_DIRS}/entities/ZstPerformer.h
    ${Showtime_COMPILED_INCLUDE_DIRS}/ZstVersion.h
)
list(APPEND Showtime_API_INTERFACE ${API_FILES})

# Private headers, only accessible within the Showtime library
set(ZST_CORE_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventDispatcher.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstINetworkInteractor.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.h"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.h"
)

# Core cpp files
set(ZST_CORE_SRC
    "${CMAKE_CURRENT_LIST_DIR}/ZstActor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEvents.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstEventDispatcher.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstLogging.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstCable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstURI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstSynchronisable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstValue.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ZstMessagePool.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstEntityBase.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstContainer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPlug.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/entities/ZstPerformer.cpp"
)

target_sources(${CORE_TARGET} PUBLIC ${ZST_PUBLIC_HEADERS} )
target_sources(${CORE_TARGET} PRIVATE ${ZST_CORE_SRC} ${ZST_CORE_HEADERS} ${Showtime_API_INTERFACE})
target_include_directories(${CORE_TARGET} 
    PUBLIC 
        $<BUILD_INTERFACE:${Showtime_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${Showtime_COMPILED_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${Showtime_SOURCE_DEPENDENCIES}
)

IF(WIN32)
    target_compile_definitions(${CORE_TARGET} PRIVATE -DZST_EXPORT_CORE_API)
    target_compile_definitions(${CORE_TARGET} INTERFACE -DZST_IMPORT_CORE_API)
    target_compile_definitions(${CORE_TARGET} PUBLIC 
                                -D_WIN32_WINNT=0x0601)
    if(HUNTER_ENABLED AND NOT USE_LOCAL_BOOST)
        target_compile_definitions(${CORE_TARGET} PUBLIC 
                                   -DBOOST_AUTO_LINK_TAGGED)
    endif()
else()
    # Use shared boost on non-windows platforms
    if(USE_LOCAL_BOOST)
        target_compile_definitions(${CORE_TARGET} PRIVATE -DBOOST_ALL_DYN_LINK)
    endif()

    # Turn off C11 ABI compatibility in glibc
    target_compile_definitions(${CORE_TARGET} PUBLIC -D_GLIBCXX_USE_CXX11_ABI=1)
endif(WIN32)

if(NOT HUNTER_ENABLED)
    if(USE_STATIC_CZMQ)
        target_compile_definitions(${CORE_TARGET} PRIVATE 
                                    -DZMQ_STATIC
                                    -DCZMQ_STATIC
        )
    endif()
    target_include_directories(${CORE_TARGET} PUBLIC 
                                # ${ZeroMQ_INCLUDE_DIRS}
                                ${CZMQ_INCLUDE_DIRS}
    )
endif()

if(USE_LOCAL_BOOST)
    target_include_directories(${CORE_TARGET} PUBLIC 
                               ${Boost_INCLUDE_DIRECTORIES}
    )
endif()

# Set threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_USE_WIN32_THREADS_INIT)
    message(STATUS "Using windows threads")
elseif(CMAKE_USE_PTHREADS_INIT)
    message(STATUS "Using posix threads")
endif()
message(STATUS "Setting link thread library to ${CMAKE_THREAD_LIBS_INIT}")

target_link_libraries(${CORE_TARGET} PUBLIC 
                        Threads::Threads
                        ${CZMQ_TARGET}
                        ${msgpackc_TARGET}
                        ${fmt_TARGET}
                        ${Boost_THREAD_TARGET}
                        ${Boost_SYSTEM_TARGET}
                        ${Boost_FILESYSTEM_TARGET}
                        ${Boost_DATE_TIME_TARGET}
                        ${Boost_CHRONO_TARGET}
                        ${Boost_ATOMIC_TARGET}
                        ${Boost_LOG_SETUP_TARGET}
                        ${Boost_LOG_TARGET}
)

list(APPEND Showtime_RUNTIME_DEPENDENCIES $<TARGET_FILE:${CZMQ_TARGET}>)

# Copy required binaries
# add_custom_command(TARGET ${CORE_TARGET} POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E
#                    copy $<TARGET_FILE:${CZMQ_TARGET}> $<TARGET_FILE_DIR:${CORE_TARGET}>
#                    COMMENT "Copying $<TARGET_FILE:${CZMQ_TARGET} to $<TARGET_FILE_DIR:${CORE_TARGET}>")
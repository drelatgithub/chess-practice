cmake_minimum_required(VERSION 3.13)

#######################################
# Configurations
#######################################

# vcpkg triplet config
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()


project(chess)

#######################################
# Source files
#######################################

set(src_dir          "src")

# List sources
file(GLOB_RECURSE src_list LIST_DIRECTORIES true CONFIGURE_DEPENDS "${src_dir}/*.cpp")

#######################################
# Compiling configs
#######################################

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Compiler flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "/MP /EHsc")
else()
    set(CMAKE_CXX_FLAGS "-Wall -ffast-math -fno-finite-math-only -mtune=native -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -funroll-loops -flto")
endif()

# Add the executable
add_executable(chess ${src_list})

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src_list})

# Include paths
target_include_directories(chess PUBLIC
    ${src_dir}
)

# Preprocessor macros
target_compile_definitions(chess PRIVATE
    # Add any preprocessor macros here

)

# Additional link directories
if(MSVC)
else()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(chess PRIVATE Threads::Threads)
endif()

if(CHESS_ADDITIONAL_LINK_DIRS)
    target_link_directories(chess PRIVATE ${CHESS_ADDITIONAL_LINK_DIRS})
endif()

# RPath specification
if(CHESS_RPATH)
    set_target_properties(chess PROPERTIES
        BUILD_RPATH ${CHESS_RPATH}
    )
endif()

# Build tool specific
if(MSVC)
    set_directory_properties(PROPERTY VS_STARTUP_PROJECT "chess")
endif()


#######################################
# External dependencies
#######################################

# catch2
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(chess PRIVATE Catch2::Catch2)

# asio
find_package(asio CONFIG REQUIRED)
target_link_libraries(chess PRIVATE asio asio::asio)

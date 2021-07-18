cmake_minimum_required(VERSION 3.13)

#######################################
# Configurations
#######################################

# vcpkg triplet config
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()


project(chess)

set(proto_src_dir    "${CMAKE_CURRENT_LIST_DIR}/proto")
set(proto_dst_dir    "${CMAKE_CURRENT_LIST_DIR}/src/proto")
set(src_dir          "src")


#######################################
# Protocol buffer
#######################################

find_program(protobuf_protoc_program protoc)
find_program(protobuf_protoc_grpc_program grpc_cpp_plugin)

# Proto file
file(GLOB_RECURSE proto_src_list LIST_DIRECTORIES true CONFIGURE_DEPENDS "${proto_src_dir}/*.proto")

set(chess_proto_srcs)
set(chess_proto_hdrs)
set(chess_grpc_srcs)
set(chess_grpc_hdrs)
foreach (each_proto_src ${proto_src_list}) # Iterate over list of absolute paths
    # get filename without extension
    get_filename_component(proto_name "${each_proto_src}" NAME_WLE)
    # add output to path
    list(APPEND chess_proto_srcs "${proto_dst_dir}/${proto_name}.pb.cc")
    list(APPEND chess_proto_hdrs "${proto_dst_dir}/${proto_name}.pb.h")
    list(APPEND chess_grpc_srcs "${proto_dst_dir}/${proto_name}.grpc.pb.cc")
    list(APPEND chess_grpc_hdrs "${proto_dst_dir}/${proto_name}.grpc.pb.h")
endforeach()

message(STATUS "proto src dir ${proto_src_dir}")

# Generated sources
add_custom_command(
    OUTPUT "${chess_proto_srcs}" "${chess_proto_hdrs}" "${chess_grpc_srcs}" "${chess_grpc_hdrs}"
    COMMAND ${protobuf_protoc_program}
    ARGS
        --grpc_out "${proto_dst_dir}"
        --cpp_out "${proto_dst_dir}"
        -I "${proto_src_dir}"
        --plugin=protoc-gen-grpc="${protobuf_protoc_grpc_program}"
        "${proto_src_list}"
    DEPENDS "${proto_src_list}")

#######################################
# Source files
#######################################

# List sources
file(GLOB_RECURSE src_list LIST_DIRECTORIES true CONFIGURE_DEPENDS "${src_dir}/*.cpp")
list(APPEND src_list "${chess_proto_srcs}" "${chess_proto_hdrs}" "${chess_grpc_srcs}" "${chess_grpc_hdrs}")

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

# grpc
find_package(gRPC CONFIG REQUIRED)
target_link_libraries(chess PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts gRPC::grpc++_reflection)

# find_package(modules CONFIG REQUIRED)
target_link_libraries(chess PRIVATE re2::re2 c-ares::cares)

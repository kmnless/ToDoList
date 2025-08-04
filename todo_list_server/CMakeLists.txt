if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

cmake_minimum_required(VERSION 3.15)
project(ToDoListServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

if(NOT Protobuf_PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc not found. Install protobuf-compiler.")
endif()

find_program(GRPC_CPP_PLUGIN_EXECUTABLE
    NAMES grpc_cpp_plugin
    PATHS ${gRPC_ROOT}/bin ${gRPC_ROOT}/../bin
    DOC "gRPC C++ plugin for protocol buffer compiler"
)

if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "grpc_cpp_plugin not found. Check your gRPC installation.")
endif()

set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proto/todo.proto")
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")

file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
    OUTPUT ${GENERATED_DIR}/todo.pb.cc ${GENERATED_DIR}/todo.pb.h
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${GENERATED_DIR} --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating todo.pb.cc and todo.pb.h"
    VERBATIM
)

add_custom_command(
    OUTPUT ${GENERATED_DIR}/todo.grpc.pb.cc ${GENERATED_DIR}/todo.grpc.pb.h
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out=${GENERATED_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE} --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILE}
    DEPENDS ${GENERATED_DIR}/todo.pb.h ${PROTO_FILE}
    COMMENT "Generating todo.grpc.pb.cc and todo.grpc.pb.h"
    VERBATIM
)

add_executable(ToDoListServer
    src/main.cpp
    src/TodoServiceImpl.cpp
    ${GENERATED_DIR}/todo.pb.cc
    ${GENERATED_DIR}/todo.grpc.pb.cc
)

target_include_directories(ToDoListServer PRIVATE
    ${GENERATED_DIR}
    src
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

target_link_libraries(ToDoListServer PRIVATE
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc
    absl::strings
    absl::memory
)

if(WIN32)
    target_link_libraries(ToDoListServer PRIVATE
        ws2_32
        iphlpapi
        advapi32
        crypt32
        gdi32
    )
endif()
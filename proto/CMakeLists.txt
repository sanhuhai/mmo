# Proto module CMakeLists.txt

cmake_minimum_required(VERSION 3.8)

# Get all proto files
file(GLOB PROTO_FILES *.proto)

# Get generated C++ files
file(GLOB PROTO_SRCS *.pb.cc)
file(GLOB PROTO_HDRS *.pb.h)

# Add proto library
add_library(proto_module STATIC
    ${PROTO_SRCS}
)

# Include directories
target_include_directories(proto_module
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Protobuf_INCLUDE_DIRS}
)

# Link against protobuf
target_link_libraries(proto_module
    PRIVATE
        libprotobuf
)

add_dependencies(proto_module libprotobuf)

# Add Lua binding file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/proto_lua.cpp")
    target_sources(proto_module PRIVATE proto_lua.cpp)
endif()

# Set C++ standard
target_compile_features(proto_module PRIVATE cxx_std_17)

# Set compile options
if(MSVC)
    target_compile_options(proto_module PRIVATE /W4 /WX /wd4702)
else()
    target_compile_options(proto_module PRIVATE -Wall -Wextra -Werror)
endif()

# Generate proto files if they don't exist
if(PROTO_FILES AND NOT PROTO_SRCS)
    message(STATUS "Generating proto files...")
    execute_process(
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out=. --proto_path=. ${PROTO_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE PROTOC_RESULT
    )
    if(PROTOC_RESULT EQUAL 0)
        message(STATUS "Proto files generated successfully")
        # Refresh the file lists
        file(GLOB PROTO_SRCS *.pb.cc)
        file(GLOB PROTO_HDRS *.pb.h)
        target_sources(proto_module PRIVATE ${PROTO_SRCS})
    else()
        message(WARNING "Failed to generate proto files")
    endif()
endif()

message(STATUS "Proto module configured with sources: ${PROTO_SRCS}")

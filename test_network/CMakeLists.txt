# Test Network Client CMakeLists.txt
# Cross-platform network test client

cmake_minimum_required(VERSION 3.8)

# Test network client executable
set(TEST_NETWORK_SOURCES
    test_network_client.cpp
)

add_executable(test_network_client ${TEST_NETWORK_SOURCES})

# Platform-specific settings
if(WIN32)
    # Windows socket libraries
    target_link_libraries(test_network_client PRIVATE 
        ws2_32      # Winsock2
        wsock32     # Winsock
    )
    
    # Windows-specific compile definitions
    target_compile_definitions(test_network_client PRIVATE 
        _WIN32_WINNT=0x0601
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
elseif(UNIX)
    # Unix/Linux libraries
    target_link_libraries(test_network_client PRIVATE 
        pthread     # POSIX threads
    )
    
    target_compile_definitions(test_network_client PRIVATE 
        _GNU_SOURCE
    )
endif()

# Link against proto module
target_link_libraries(test_network_client PRIVATE 
    proto_module
)

# Add protobuf library dependency
if(WIN32)
    target_link_libraries(test_network_client PRIVATE 
        $<TARGET_FILE:libprotobuf>
    )
    add_dependencies(test_network_client libprotobuf)
else()
    target_link_libraries(test_network_client PRIVATE 
        libprotobuf
    )
endif()

add_dependencies(test_network_client proto_module)

# Include directories
target_include_directories(test_network_client PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${PROTO_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

# Set C++ standard
target_compile_features(test_network_client PRIVATE cxx_std_17)

# Set compile options
if(MSVC)
    target_compile_options(test_network_client PRIVATE /W4 /WX-)
else()
    target_compile_options(test_network_client PRIVATE 
        -Wall 
        -Wextra 
        -Wno-unused-parameter 
    )
endif()

# Output directory
set_target_properties(test_network_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "Test network client configured")

cmake_minimum_required(VERSION 3.5)
project(mmo_server VERSION 1.0.0 LANGUAGES CXX C)

# ==========================================
# Platform Detection
# ==========================================
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS TRUE)
    message(STATUS "Platform: Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Platform: Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
    message(STATUS "Platform: macOS")
else()
    message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
endif()

# ==========================================
# Compiler Settings
# ==========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# MSVC (Visual Studio) Settings
if(MSVC)
    message(STATUS "Compiler: MSVC ${MSVC_VERSION}")
    
    # Warning level and multi-processor compilation
    add_compile_options(/W4 /WX- /MP)
    
    # Disable specific warnings
    add_compile_options(/wd4251 /wd4275 /wd4267 /wd4244)
    
    # Security and Windows version
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    
    # Runtime library settings
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Enable address sanitizer in Debug mode (VS2019+)
    if(MSVC_VERSION GREATER_EQUAL 1920)
        option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
        if(ENABLE_ASAN)
            add_compile_options(/fsanitize=address)
        endif()
    endif()
    
# GCC/Clang Settings
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "Compiler: Clang ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
    
    # Warning flags
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-Wno-unused-parameter -Wno-unused-variable)
    
    # Position independent code for shared libraries
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    
    # Debug symbols
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    
    # Enable additional checks in Debug
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
endif()

# ==========================================
# Build Options
# ==========================================
option(ENABLE_ASIO_STANDALONE "Use standalone Asio" ON)
option(ENABLE_SPDLOG "Use spdlog for logging" ON)
option(ENABLE_PROTOBUF "Use Protocol Buffers" OFF)
option(ENABLE_MYSQL "Use MySQL" ON)
option(ENABLE_REDIS "Use Redis" OFF)
option(BUILD_LUA_FROM_SOURCE "Build Lua from source" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Build tests" OFF)

set(LUA_VERSION "5.4" CACHE STRING "Lua version to use")

# ==========================================
# Directories
# ==========================================
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)
set(PROTO_DIR ${CMAKE_SOURCE_DIR}/proto)
set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts)

include_directories(${INCLUDE_DIR})
include_directories(${THIRD_PARTY_DIR})

# ==========================================
# Dependencies
# ==========================================
find_package(Threads REQUIRED)

# ==========================================
# Third Party Libraries (Header-only)
# ==========================================

# Asio (header-only)
set(ASIO_INCLUDE_DIR ${THIRD_PARTY_DIR}/asio/include)
include_directories(${ASIO_INCLUDE_DIR})
add_definitions(-DASIO_STANDALONE)
message(STATUS "Asio include: ${ASIO_INCLUDE_DIR}")

# LuaBridge3 (header-only)
set(LUABRIDGE_INCLUDE_DIR ${THIRD_PARTY_DIR}/LuaBridge3/Source)
include_directories(${LUABRIDGE_INCLUDE_DIR})
message(STATUS "LuaBridge3 include: ${LUABRIDGE_INCLUDE_DIR}")

# Lua - Build from source
if(BUILD_LUA_FROM_SOURCE)
    message(STATUS "Building Lua ${LUA_VERSION} from source...")
    add_subdirectory(${THIRD_PARTY_DIR}/lua)
    set(LUA_INCLUDE_DIR ${THIRD_PARTY_DIR}/lua/src)
    set(LUA_LIBRARIES lua_static)
    include_directories(${LUA_INCLUDE_DIR})
    message(STATUS "Lua include: ${LUA_INCLUDE_DIR}")
    message(STATUS "Lua library: ${LUA_LIBRARIES}")
else()
    find_package(Lua ${LUA_VERSION} REQUIRED)
    include_directories(${LUA_INCLUDE_DIR})
endif()

# spdlog
if(ENABLE_SPDLOG)
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        message(STATUS "Found spdlog: ${spdlog_VERSION}")
    else()
        message(STATUS "spdlog not found, logging will use simple implementation")
    endif()
endif()

# Protocol Buffers
if(ENABLE_PROTOBUF)
    find_package(Protobuf QUIET)
    if(Protobuf_FOUND)
        message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
        include_directories(${Protobuf_INCLUDE_DIRS})
        
        file(GLOB PROTO_FILES ${PROTO_DIR}/*.proto)
        if(PROTO_FILES)
            protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
            include_directories(${CMAKE_CURRENT_BINARY_DIR})
            message(STATUS "Proto files: ${PROTO_FILES}")
        endif()
    else()
        message(STATUS "Protobuf not found, proto support disabled")
        set(ENABLE_PROTOBUF OFF)
    endif()
endif()

# MySQL
if(ENABLE_MYSQL)
    find_package(MySQL QUIET)
    if(NOT MySQL_FOUND)
        # Check for bundled MySQL in third_party
        if(EXISTS "${THIRD_PARTY_DIR}/mysql/include/mysql.h")
            set(MySQL_INCLUDE_DIR "${THIRD_PARTY_DIR}/mysql/include")
            set(MySQL_LIBRARY "${THIRD_PARTY_DIR}/mysql/lib/libmysql.lib")
            message(STATUS "Found bundled MySQL in third_party")
        endif()
        
        # Windows paths
        if(PLATFORM_WINDOWS AND NOT MySQL_INCLUDE_DIR)
            find_path(MySQL_INCLUDE_DIR mysql/mysql.h
                PATHS
                    "C:/Program Files/MySQL/MySQL Server 8.0/include"
                    "C:/Program Files (x86)/MySQL/MySQL Server 8.0/include"
                    "C:/Program Files/MySQL/MySQL Server 8.4/include"
                    "C:/Program Files (x86)/MySQL/MySQL Server 8.4/include"
                    "C:/Program Files/MySQL/MySQL Server 5.7/include"
                    "C:/Program Files (x86)/MySQL/MySQL Server 5.7/include"
                    "C:/mysql/include"
                DOC "MySQL include directory"
            )
            
            find_library(MySQL_LIBRARY
                NAMES libmysql mysqlclient
                PATHS
                    "C:/Program Files/MySQL/MySQL Server 8.0/lib"
                    "C:/Program Files (x86)/MySQL/MySQL Server 8.0/lib"
                    "C:/Program Files/MySQL/MySQL Server 8.4/lib"
                    "C:/Program Files (x86)/MySQL/MySQL Server 8.4/lib"
                    "C:/Program Files/MySQL/MySQL Server 5.7/lib"
                    "C:/Program Files (x86)/MySQL/MySQL Server 5.7/lib"
                    "C:/mysql/lib"
                DOC "MySQL library"
            )
        # Linux paths
        else()
            find_path(MySQL_INCLUDE_DIR mysql/mysql.h
                PATHS
                    "/usr/include/mysql"
                    "/usr/local/include/mysql"
                    "/usr/include/mariadb"
                    "/usr/local/include/mariadb"
                DOC "MySQL include directory"
            )
            
            find_library(MySQL_LIBRARY
                NAMES mysqlclient mysqlclient_r mariadb
                PATHS
                    "/usr/lib/x86_64-linux-gnu"
                    "/usr/lib64/mysql"
                    "/usr/lib/mysql"
                    "/usr/local/lib"
                DOC "MySQL library"
            )
        endif()
        
        if(MySQL_INCLUDE_DIR AND MySQL_LIBRARY)
            set(MySQL_FOUND TRUE)
            set(MySQL_INCLUDE_DIRS ${MySQL_INCLUDE_DIR})
            set(MySQL_LIBRARIES ${MySQL_LIBRARY})
            include_directories(${MySQL_INCLUDE_DIRS})
            # For bundled MySQL, also include parent directory for mysql/mysql.h style includes
            if(EXISTS "${THIRD_PARTY_DIR}/mysql/include/mysql.h")
                include_directories(${THIRD_PARTY_DIR}/mysql/include)
            endif()
            message(STATUS "Found MySQL: ${MySQL_LIBRARIES}")
        else()
            message(STATUS "MySQL not found, database support disabled")
            set(ENABLE_MYSQL OFF)
        endif()
    else()
        include_directories(${MySQL_INCLUDE_DIRS})
    endif()
endif()

# Redis
if(ENABLE_REDIS)
    if(PLATFORM_WINDOWS)
        # Windows: Build hiredis from redis/deps/hiredis
        message(STATUS "Building hiredis from redis/deps/hiredis for Windows...")
        add_subdirectory(${THIRD_PARTY_DIR}/redis/deps/hiredis)
        set(HIREDIS_INCLUDE_DIR ${THIRD_PARTY_DIR}/redis/deps/hiredis)
        set(HIREDIS_LIBRARIES hiredis)
        include_directories(${HIREDIS_INCLUDE_DIR})
        message(STATUS "hiredis include: ${HIREDIS_INCLUDE_DIR}")
        message(STATUS "hiredis library: ${HIREDIS_LIBRARIES}")
    elseif(PLATFORM_LINUX)
        # Linux: Use system hiredis library
        message(STATUS "Using system hiredis library for Linux...")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(HIREDIS REQUIRED hiredis)
        include_directories(${HIREDIS_INCLUDE_DIRS})
        message(STATUS "hiredis include: ${HIREDIS_INCLUDE_DIRS}")
        message(STATUS "hiredis library: ${HIREDIS_LIBRARIES}")
    endif()
endif()

# ==========================================
# Source Code
# ==========================================
add_subdirectory(src)

# ==========================================
# Copy Resources
# ==========================================
file(COPY ${CMAKE_SOURCE_DIR}/lua DESTINATION ${CMAKE_BINARY_DIR})
if(ENABLE_PROTOBUF)
    file(COPY ${CMAKE_SOURCE_DIR}/proto DESTINATION ${CMAKE_BINARY_DIR})
endif()

# ==========================================
# Installation
# ==========================================
include(GNUInstallDirs)

install(TARGETS mmo_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/lua DESTINATION ${CMAKE_INSTALL_DATADIR}/mmo_server)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/proto DESTINATION ${CMAKE_INSTALL_DATADIR}/mmo_server)

# ==========================================
# Summary
# ==========================================
message(STATUS "")
message(STATUS "==========================================")
message(STATUS "MMO Server Configuration Summary")
message(STATUS "==========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  Lua ${LUA_VERSION}: ${BUILD_LUA_FROM_SOURCE} (from source)")
message(STATUS "  Asio: ON (header-only)")
message(STATUS "  LuaBridge: ON (header-only)")
message(STATUS "  spdlog: ${ENABLE_SPDLOG}")
message(STATUS "  Protobuf: ${ENABLE_PROTOBUF}")
message(STATUS "  MySQL: ${ENABLE_MYSQL}")
message(STATUS "  Redis: ${ENABLE_REDIS}")
message(STATUS "")
message(STATUS "Paths:")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Lua include: ${LUA_INCLUDE_DIR}")
message(STATUS "  Lua library: ${LUA_LIBRARIES}")
if(ENABLE_PROTOBUF)
    message(STATUS "  Protobuf: ${Protobuf_VERSION}")
endif()
if(ENABLE_MYSQL)
    message(STATUS "  MySQL: ${MySQL_LIBRARIES}")
endif()
if(ENABLE_REDIS)
    message(STATUS "  Redis: ${HIREDIS_LIBRARIES}")
endif()
message(STATUS "==========================================")
message(STATUS "")

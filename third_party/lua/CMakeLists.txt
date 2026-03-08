cmake_minimum_required(VERSION 3.15)
project(lua VERSION 5.4.6 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LUA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(LUA_CORE_SRC
    ${LUA_SRC_DIR}/lapi.c
    ${LUA_SRC_DIR}/lcode.c
    ${LUA_SRC_DIR}/lctype.c
    ${LUA_SRC_DIR}/ldebug.c
    ${LUA_SRC_DIR}/ldo.c
    ${LUA_SRC_DIR}/ldump.c
    ${LUA_SRC_DIR}/lfunc.c
    ${LUA_SRC_DIR}/lgc.c
    ${LUA_SRC_DIR}/llex.c
    ${LUA_SRC_DIR}/lmem.c
    ${LUA_SRC_DIR}/lobject.c
    ${LUA_SRC_DIR}/lopcodes.c
    ${LUA_SRC_DIR}/lparser.c
    ${LUA_SRC_DIR}/lstate.c
    ${LUA_SRC_DIR}/lstring.c
    ${LUA_SRC_DIR}/ltable.c
    ${LUA_SRC_DIR}/ltm.c
    ${LUA_SRC_DIR}/lundump.c
    ${LUA_SRC_DIR}/lvm.c
    ${LUA_SRC_DIR}/lzio.c
)

set(LUA_LIB_SRC
    ${LUA_SRC_DIR}/lauxlib.c
    ${LUA_SRC_DIR}/lbaselib.c
    ${LUA_SRC_DIR}/lcorolib.c
    ${LUA_SRC_DIR}/ldblib.c
    ${LUA_SRC_DIR}/liolib.c
    ${LUA_SRC_DIR}/lmathlib.c
    ${LUA_SRC_DIR}/loadlib.c
    ${LUA_SRC_DIR}/loslib.c
    ${LUA_SRC_DIR}/lstrlib.c
    ${LUA_SRC_DIR}/ltablib.c
    ${LUA_SRC_DIR}/lutf8lib.c
    ${LUA_SRC_DIR}/linit.c
)

set(LUA_ALL_SRC ${LUA_CORE_SRC} ${LUA_LIB_SRC})

include_directories(${LUA_SRC_DIR})

# ==========================================
# Platform-specific definitions
# ==========================================
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# ==========================================
# Lua Static Library
# ==========================================
add_library(lua_static STATIC ${LUA_ALL_SRC})
set_target_properties(lua_static PROPERTIES
    OUTPUT_NAME lua54
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

if(UNIX)
    target_compile_options(lua_static PRIVATE -fPIC)
    target_link_libraries(lua_static PUBLIC m dl)
endif()

if(WIN32)
    target_link_libraries(lua_static PUBLIC ws2_32)
endif()

# ==========================================
# Lua Shared Library
# ==========================================
add_library(lua_shared SHARED ${LUA_ALL_SRC})
set_target_properties(lua_shared PROPERTIES
    OUTPUT_NAME lua54
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

if(WIN32)
    target_compile_definitions(lua_shared PRIVATE LUA_BUILD_AS_DLL)
    target_link_libraries(lua_shared PUBLIC ws2_32)
endif()

if(UNIX)
    target_link_libraries(lua_shared PUBLIC m dl)
endif()

# ==========================================
# Lua Interpreter Executable
# ==========================================
add_executable(lua_exe ${LUA_SRC_DIR}/lua.c)
set_target_properties(lua_exe PROPERTIES
    OUTPUT_NAME lua
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_link_libraries(lua_exe PRIVATE lua_static)

if(UNIX)
    target_link_libraries(lua_exe PRIVATE m dl)
endif()

if(WIN32)
    target_link_libraries(lua_exe PRIVATE ws2_32)
endif()

# ==========================================
# Lua Compiler Executable
# ==========================================
add_executable(luac_exe ${LUA_SRC_DIR}/luac.c)
set_target_properties(luac_exe PROPERTIES
    OUTPUT_NAME luac
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_link_libraries(luac_exe PRIVATE lua_static)

if(UNIX)
    target_link_libraries(luac_exe PRIVATE m dl)
endif()

if(WIN32)
    target_link_libraries(luac_exe PRIVATE ws2_32)
endif()

# ==========================================
# Export include directory
# ==========================================
set(LUA_INCLUDE_DIR ${LUA_SRC_DIR} CACHE PATH "Lua include directory" FORCE)

message(STATUS "Lua 5.4.6 configured")
message(STATUS "  Static library: lua_static")
message(STATUS "  Shared library: lua_shared")
message(STATUS "  Executable: lua_exe, luac_exe")
message(STATUS "  Include dir: ${LUA_INCLUDE_DIR}")

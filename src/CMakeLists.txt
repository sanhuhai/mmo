set(CORE_SOURCES
    core/logger.cpp
    core/config.cpp
    core/application.cpp
)

set(NETWORK_SOURCES
    network/connection.cpp
    network/connection_manager.cpp
    network/tcp_server.cpp
    network/network_processor.cpp
    network/network_server.cpp
)

set(SCRIPT_SOURCES
    script/lua_engine.cpp
    script/lua_register.cpp
)

set(SERVICE_SOURCES
    service/service_base.cpp
    service/gate_service.cpp
    service/game_service.cpp
)

if(ENABLE_SKYNET)
    list(APPEND SERVICE_SOURCES
        service/skynet_service.cpp
    )
endif()

set(UTILS_SOURCES
    utils/thread_pool.cpp
    utils/buffer.cpp
)

set(JSON_SOURCES)
if(ENABLE_RAPIDJSON)
    list(APPEND JSON_SOURCES
        utils/json_utils.cpp
    )
endif()

set(DB_SOURCES)
if(ENABLE_MYSQL)
    list(APPEND DB_SOURCES
        db/mysql_connection.cpp
        db/mysql_pool.cpp
        player/player_manager.cpp
        player/weapon.cpp
    )
else()
    list(APPEND DB_SOURCES
        player/weapon.cpp
    )
endif()

set(CACHE_SOURCES)
if(ENABLE_REDIS)
    list(APPEND CACHE_SOURCES
        cache/redis_connection.cpp
        cache/redis_pool.cpp
        cache/cache_manager.cpp
    )
endif()

set(TASK_SOURCES
    task/task_base.cpp
    task/task_manager.cpp
    task/task_db_loader.cpp
    task/task_reward.cpp
)

set(REWARD_SOURCES
    reward/reward_types.cpp
    reward/reward_loader.cpp
    reward/reward_giver.cpp
)

set(SQLITE3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/designer/third_party/sqlite3")
set(SQLITE3_SOURCE "${CMAKE_SOURCE_DIR}/designer/third_party/sqlite3/sqlite3.c")

if(EXISTS "${SQLITE3_INCLUDE_DIR}/sqlite3.h")
    if(NOT TARGET sqlite3_static)
        add_library(sqlite3_static STATIC "${SQLITE3_SOURCE}")
        target_include_directories(sqlite3_static PUBLIC ${SQLITE3_INCLUDE_DIR})
    endif()
    set(SQLITE3_FOUND TRUE)
    include_directories(${SQLITE3_INCLUDE_DIR})
    message(STATUS "Using SQLite3 for task system: ${SQLITE3_INCLUDE_DIR}")
else()
    set(SQLITE3_FOUND FALSE)
    message(STATUS "SQLite3 not found, task database storage will be disabled")
endif()

set(PROTO_SOURCES)
if(ENABLE_PROTOBUF)
    list(APPEND PROTO_SOURCES
        proto/message_codec.cpp
        proto/lua_protobuf.cpp
        network/proto_connection.cpp
        network/proto_server.cpp
    )
endif()

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${SCRIPT_SOURCES}
    ${SERVICE_SOURCES}
    ${UTILS_SOURCES}
    ${DB_SOURCES}
    ${CACHE_SOURCES}
    ${TASK_SOURCES}
    ${REWARD_SOURCES}
    ${PROTO_SOURCES}
    ${JSON_SOURCES}
)

# if(ENABLE_PROTOBUF)
#     list(APPEND ALL_SOURCES ${PROTO_SRCS})
# endif()

add_library(mmo_core STATIC ${ALL_SOURCES})

target_include_directories(mmo_core PUBLIC
    ${INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(mmo_core PUBLIC
    ${LUA_LIBRARIES}
    lua_shared
    Threads::Threads
)

if(ENABLE_SPDLOG AND spdlog_FOUND)
    target_link_libraries(mmo_core PUBLIC spdlog::spdlog)
    target_compile_definitions(mmo_core PUBLIC USE_SPDLOG)
endif()

if(ENABLE_PROTOBUF AND Protobuf_FOUND)
    target_link_libraries(mmo_core PUBLIC ${Protobuf_LIBRARIES} proto_module)
    target_compile_definitions(mmo_core PUBLIC USE_PROTOBUF)
    target_include_directories(mmo_core PUBLIC ${PROTO_DIR})
endif()

if(ENABLE_MYSQL AND MySQL_FOUND)
    target_link_libraries(mmo_core PUBLIC ${MySQL_LIBRARIES})
    target_compile_definitions(mmo_core PUBLIC USE_MYSQL)
endif()

if(ENABLE_REDIS)
    target_link_libraries(mmo_core PUBLIC ${HIREDIS_LIBRARIES})
    target_compile_definitions(mmo_core PUBLIC USE_REDIS)
    
    # Windows: hiredis requires Winsock
    if(WIN32)
        target_link_libraries(mmo_core PUBLIC ws2_32)
    endif()
endif()

if(ENABLE_RAPIDJSON)
    target_compile_definitions(mmo_core PUBLIC USE_RAPIDJSON)
endif()

if(SQLITE3_FOUND)
    target_link_libraries(mmo_core PUBLIC sqlite3_static)
    target_compile_definitions(mmo_core PUBLIC USE_SQLITE3)
endif()

if(ENABLE_SKYNET)
    target_link_libraries(mmo_core PUBLIC 
        skynet_core
        skynet_services
        skynet_lualib
    )
    target_compile_definitions(mmo_core PUBLIC USE_SKYNET)
endif()

add_executable(mmo_server main.cpp)
target_link_libraries(mmo_server PRIVATE mmo_core)

set_target_properties(mmo_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_executable(test_network_client ../test_network_client.cpp)

target_link_libraries(test_network_client PRIVATE 
    ws2_32
    proto_module
    $<TARGET_FILE:libprotobuf>
)

add_dependencies(test_network_client proto_module libprotobuf)

target_include_directories(test_network_client PRIVATE
    ${PROTO_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

set_target_properties(test_network_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ==========================================
# Platform-specific libraries
# ==========================================
if(WIN32)
    # Windows socket and IO libraries
    target_link_libraries(mmo_core PUBLIC 
        ws2_32      # Winsock2
        wsock32     # Winsock
        iphlpapi    # IP Helper
    )
    
    # Windows-specific compile definitions
    target_compile_definitions(mmo_core PUBLIC 
        _WIN32_WINNT=0x0601
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

if(UNIX)
    # Unix/Linux libraries
    target_link_libraries(mmo_core PUBLIC 
        m           # Math library
        dl          # Dynamic loader
        pthread     # POSIX threads (explicitly)
    )
    
    # Linux-specific compile definitions
    target_compile_definitions(mmo_core PUBLIC 
        _GNU_SOURCE
    )
endif()

if(APPLE)
    # macOS-specific libraries
    target_link_libraries(mmo_core PUBLIC 
        "-framework CoreFoundation"
    )
endif()

# ==========================================
# Compiler-specific options
# ==========================================
if(MSVC)
    # MSVC-specific options for the target
    target_compile_options(mmo_core PRIVATE /W4 /WX- /MP)
else()
    # GCC/Clang-specific options
    target_compile_options(mmo_core PRIVATE 
        -Wall 
        -Wextra 
        -Wno-unused-parameter 
        -Wno-unused-variable
    )
endif()

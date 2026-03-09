cmake_minimum_required(VERSION 3.10)
project(excel_converter VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX- /MP)
    add_compile_options(/wd4251 /wd4275 /wd4267 /wd4244)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-Wno-unused-parameter -Wno-unused-variable)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

option(USE_BUNDLED_SQLITE "Use bundled SQLite3" ON)
option(USE_XLSXIO "Enable XLSXIO support for Excel files" OFF)

set(DESIGNER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(INCLUDE_DIR ${DESIGNER_DIR}/include)
set(SRC_DIR ${DESIGNER_DIR}/src)
set(THIRD_PARTY_DIR ${DESIGNER_DIR}/third_party)

include_directories(${INCLUDE_DIR})

find_package(Threads REQUIRED)

if(USE_BUNDLED_SQLITE)
    set(SQLITE3_INCLUDE_DIR "${THIRD_PARTY_DIR}/sqlite3")
    set(SQLITE3_SOURCE "${THIRD_PARTY_DIR}/sqlite3/sqlite3.c")
    
    if(NOT EXISTS "${SQLITE3_INCLUDE_DIR}/sqlite3.h")
        message(STATUS "SQLite3 amalgamation not found, downloading...")
        file(MAKE_DIRECTORY "${SQLITE3_INCLUDE_DIR}")
        
        set(SQLITE_VERSION "3450100")
        set(SQLITE_URL "https://www.sqlite.org/2024/sqlite-amalgamation-${SQLITE_VERSION}.zip")
        set(SQLITE_ZIP "${CMAKE_BINARY_DIR}/sqlite.zip")
        set(SQLITE_EXTRACT_DIR "${CMAKE_BINARY_DIR}/sqlite-amalgamation-${SQLITE_VERSION}")
        
        file(DOWNLOAD ${SQLITE_URL} ${SQLITE_ZIP} SHOW_PROGRESS STATUS(download_status))
        
        if(download_status EQUAL 0)
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xzf ${SQLITE_ZIP}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )
            
            if(EXISTS "${SQLITE_EXTRACT_DIR}/sqlite3.h")
                file(COPY "${SQLITE_EXTRACT_DIR}/sqlite3.h" DESTINATION "${SQLITE3_INCLUDE_DIR}")
                file(COPY "${SQLITE_EXTRACT_DIR}/sqlite3.c" DESTINATION "${SQLITE3_INCLUDE_DIR}")
                message(STATUS "SQLite3 downloaded successfully")
            endif()
        else()
            message(WARNING "Failed to download SQLite3, creating minimal stub")
            file(WRITE "${SQLITE3_INCLUDE_DIR}/sqlite3.h" "
#ifndef SQLITE3_H
#define SQLITE3_H
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101
#define SQLITE_INTEGER 1
#define SQLITE_FLOAT 2
#define SQLITE_TEXT 3
#define SQLITE_BLOB 4
#define SQLITE_TRANSIENT ((void(*)(void*))-1)
#endif
")
            file(WRITE "${SQLITE3_INCLUDE_DIR}/sqlite3.c" "
#include \"sqlite3.h\"
int sqlite3_open(const char*, sqlite3**) { return -1; }
int sqlite3_close(sqlite3*) { return 0; }
int sqlite3_exec(sqlite3*, const char*, int(*)(void*,int,char**,char**), void*, char**) { return -1; }
int sqlite3_prepare_v2(sqlite3*, const char*, int, sqlite3_stmt**, const char**) { return -1; }
int sqlite3_step(sqlite3_stmt*) { return -1; }
int sqlite3_finalize(sqlite3_stmt*) { return 0; }
int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int, void(*)(void*)) { return 0; }
int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int, void(*)(void*)) { return 0; }
int sqlite3_bind_int(sqlite3_stmt*, int, int) { return 0; }
int sqlite3_bind_int64(sqlite3_stmt*, int, long long) { return 0; }
int sqlite3_bind_double(sqlite3_stmt*, int, double) { return 0; }
const char* sqlite3_errmsg(sqlite3*) { return \"stub\"; }
")
        endif()
    endif()
    
    if(EXISTS "${SQLITE3_INCLUDE_DIR}/sqlite3.c")
        add_library(sqlite3_static STATIC "${SQLITE3_INCLUDE_DIR}/sqlite3.c")
        target_include_directories(sqlite3_static PUBLIC ${SQLITE3_INCLUDE_DIR})
        set(SQLITE3_LIBRARIES sqlite3_static)
    else()
        set(SQLITE3_LIBRARIES "")
    endif()
    
    include_directories(${SQLITE3_INCLUDE_DIR})
    message(STATUS "Using bundled SQLite3")
else()
    find_package(SQLite3 REQUIRED)
    include_directories(${SQLite3_INCLUDE_DIRS})
    set(SQLITE3_LIBRARIES ${SQLite3_LIBRARIES})
    message(STATUS "Using system SQLite3: ${SQLite3_VERSION}")
endif()

if(USE_XLSXIO)
    find_path(XLSXIO_INCLUDE_DIR xlsxio_read.h)
    find_library(XLSXIO_LIBRARY xlsxio_read)
    
    if(XLSXIO_INCLUDE_DIR AND XLSXIO_LIBRARY)
        include_directories(${XLSXIO_INCLUDE_DIR})
        set(XLSXIO_LIBRARIES ${XLSXIO_LIBRARY})
        add_definitions(-DUSE_XLSXIO)
        message(STATUS "XLSXIO support enabled: ${XLSXIO_LIBRARY}")
    else()
        message(WARNING "XLSXIO not found, Excel support disabled. Only CSV files will be supported.")
        set(USE_XLSXIO OFF)
    endif()
else()
    message(STATUS "XLSXIO support disabled. Only CSV files will be supported.")
endif()

set(CONVERTER_SOURCES
    ${SRC_DIR}/excel_reader.cpp
    ${SRC_DIR}/binary_serializer.cpp
    ${SRC_DIR}/excel_converter.cpp
    ${SRC_DIR}/main.cpp
)

add_executable(excel_converter ${CONVERTER_SOURCES})

target_link_libraries(excel_converter
    ${SQLITE3_LIBRARIES}
    Threads::Threads
)

if(USE_XLSXIO)
    target_link_libraries(excel_converter ${XLSXIO_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(excel_converter shell32)
endif()

set_target_properties(excel_converter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS excel_converter
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "Excel Converter Configuration Summary")
message(STATUS "=====================================")
message(STATUS "SQLite3: ${SQLITE3_LIBRARIES}")
message(STATUS "XLSXIO: ${USE_XLSXIO}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Supported formats:")
message(STATUS "  - CSV: Yes (always supported)")
if(USE_XLSXIO)
    message(STATUS "  - XLSX: Yes")
else()
    message(STATUS "  - XLSX: No (install XLSXIO to enable)")
endif()
message(STATUS "")

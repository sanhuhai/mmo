@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ==========================================
echo MMO Server Build Script for Windows
echo Using Visual Studio 2022
echo ==========================================
echo.

:: 设置变量
set "BUILD_TYPE=Release"
set "BUILD_DIR=build_windows"
set "GENERATOR=Visual Studio 17 2022"
set "PLATFORM=x64"
set "VCPKG_ROOT="

:: 解析参数
:parse_args
if "%~1"=="" goto :done_parse
if /i "%~1"=="debug" set "BUILD_TYPE=Debug"
if /i "%~1"=="release" set "BUILD_TYPE=Release"
if /i "%~1"=="clean" set "CLEAN_BUILD=1"
if /i "%~1"=="--vcpkg" (
    set "VCPKG_ROOT=%~2"
    shift
)
shift
goto :parse_args
:done_parse

echo Build Type: %BUILD_TYPE%
echo Generator: %GENERATOR%
echo Platform: %PLATFORM%
echo.

:: 检查CMake
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found in PATH
    echo Please install CMake and add it to PATH
    exit /b 1
)

cmake --version
echo.

:: 检查Visual Studio 2022
if not exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" (
    if not exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe" (
        if not exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe" (
            echo [WARNING] Visual Studio 2022 not found in default location
            echo Please ensure Visual Studio 2022 is installed
        )
    )
)

:: 清理构建目录
if "%CLEAN_BUILD%"=="1" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Clean complete.
    echo.
)

:: 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: 配置CMake
echo Configuring CMake...
echo.

set "CMAKE_ARGS=-G "%GENERATOR%" -A %PLATFORM%"
set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%"
set "CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_LUA_FROM_SOURCE=ON"
set "CMAKE_ARGS=%CMAKE_ARGS% -DENABLE_PROTOBUF=ON"
set "CMAKE_ARGS=%CMAKE_ARGS% -DENABLE_MYSQL=ON"
set "CMAKE_ARGS=%CMAKE_ARGS% -DENABLE_SPDLOG=ON"

:: 如果指定了vcpkg
if not "%VCPKG_ROOT%"=="" (
    echo Using vcpkg from: %VCPKG_ROOT%
    set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)

echo CMake command: cmake %CMAKE_ARGS% ..
echo.

cmake %CMAKE_ARGS% ..
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed
    cd ..
    exit /b 1
)

echo.
echo CMake configuration successful!
echo.

:: 编译项目
echo Building project...
echo.

cmake --build . --config %BUILD_TYPE% --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed
    cd ..
    exit /b 1
)

echo.
echo ==========================================
echo Build successful!
echo ==========================================
echo.
echo Output files:
echo   - Executable: %BUILD_DIR%\bin\%BUILD_TYPE%\mmo_server.exe
echo   - Lua: %BUILD_DIR%\bin\%BUILD_TYPE%\lua.exe
echo   - Libraries: %BUILD_DIR%\lib\%BUILD_TYPE%\
echo.

cd ..

endlocal

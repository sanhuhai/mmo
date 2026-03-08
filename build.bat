@echo off
echo Building MMO Server...

if not exist build mkdir build
cd build

echo Running CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% NEQ 0 (
    echo CMake failed!
    exit /b 1
)

echo Building Release...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Binary: build\bin\Release\mmo_server.exe

cd ..

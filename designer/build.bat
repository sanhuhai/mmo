@echo off
setlocal

echo ========================================
echo Excel Converter Build Script
echo ========================================
echo.

set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build

echo Configuring project...
cmake -B "%BUILD_DIR%" -S "%SCRIPT_DIR%" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build "%BUILD_DIR%" --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Executable: %BUILD_DIR%\bin\Release\excel_converter.exe
echo.
echo Usage:
echo   excel_converter -i input.csv -o output.db
echo   excel_converter -i ./excel/ -o ./output/
echo ========================================
echo.

pause

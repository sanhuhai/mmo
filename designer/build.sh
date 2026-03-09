#!/bin/bash

echo "========================================"
echo "Excel Converter Build Script"
echo "========================================"
echo

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "Configuring project..."
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "Configuration failed!"
    exit 1
fi

echo
echo "Building project..."
cmake --build "${BUILD_DIR}" --config Release
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo
echo "========================================"
echo "Build completed successfully!"
echo "Executable: ${BUILD_DIR}/bin/excel_converter"
echo
echo "Usage:"
echo "  excel_converter -i input.csv -o output.db"
echo "  excel_converter -i ./excel/ -o ./output/"
echo "========================================"

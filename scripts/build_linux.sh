#!/bin/bash

# ==========================================
# MMO Server Build Script for Linux
# ==========================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 默认配置
BUILD_TYPE="Release"
BUILD_DIR="build_linux"
PARALLEL_JOBS=$(nproc)
CLEAN_BUILD=0
INSTALL_DEPS=0

# 打印信息
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示帮助
show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    debug           Build in Debug mode
    release         Build in Release mode (default)
    clean           Clean build directory before building
    install-deps    Install dependencies (requires sudo)
    -j N            Use N parallel jobs (default: $(nproc))
    -h, --help      Show this help message

Examples:
    $0                          # Build release version
    $0 debug                    # Build debug version
    $0 clean release            # Clean and build release
    $0 install-deps             # Install dependencies and build
    $0 -j 4                     # Build with 4 parallel jobs
EOF
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        release)
            BUILD_TYPE="Release"
            shift
            ;;
        clean)
            CLEAN_BUILD=1
            shift
            ;;
        install-deps)
            INSTALL_DEPS=1
            shift
            ;;
        -j)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

echo "=========================================="
echo "MMO Server Build Script for Linux"
echo "=========================================="
echo ""

info "Build Type: $BUILD_TYPE"
info "Build Directory: $BUILD_DIR"
info "Parallel Jobs: $PARALLEL_JOBS"
echo ""

# 安装依赖
if [ "$INSTALL_DEPS" -eq 1 ]; then
    info "Installing dependencies..."
    
    if command -v apt-get &> /dev/null; then
        # Debian/Ubuntu
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            git \
            wget \
            liblua5.4-dev \
            libmysqlclient-dev \
            libprotobuf-dev \
            protobuf-compiler \
            libspdlog-dev \
            libssl-dev
    elif command -v yum &> /dev/null; then
        # CentOS/RHEL/Fedora
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y \
            cmake3 \
            git \
            wget \
            lua-devel \
            mysql-devel \
            protobuf-devel \
            protobuf-compiler \
            spdlog-devel \
            openssl-devel
    elif command -v pacman &> /dev/null; then
        # Arch Linux
        sudo pacman -S --needed \
            base-devel \
            cmake \
            git \
            wget \
            lua \
            mariadb-libs \
            protobuf \
            spdlog \
            openssl
    else
        error "Unsupported package manager. Please install dependencies manually."
        exit 1
    fi
    
    info "Dependencies installed successfully!"
    echo ""
fi

# 检查CMake
if ! command -v cmake &> /dev/null; then
    error "CMake not found. Please install CMake."
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
info "CMake version: $CMAKE_VERSION"
echo ""

# 检查编译器
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    error "No C++ compiler found. Please install g++ or clang++."
    exit 1
fi

if command -v g++ &> /dev/null; then
    CXX_COMPILER="g++"
    CXX_VERSION=$(g++ --version | head -n1)
elif command -v clang++ &> /dev/null; then
    CXX_COMPILER="clang++"
    CXX_VERSION=$(clang++ --version | head -n1)
fi

info "C++ Compiler: $CXX_VERSION"
echo ""

# 清理构建目录
if [ "$CLEAN_BUILD" -eq 1 ]; then
    info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    info "Clean complete."
    echo ""
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置CMake
info "Configuring CMake..."
echo ""

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_LUA_FROM_SOURCE=ON \
    -DENABLE_PROTOBUF=ON \
    -DENABLE_MYSQL=ON \
    -DENABLE_SPDLOG=ON \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_C_STANDARD=99

if [ $? -ne 0 ]; then
    error "CMake configuration failed"
    exit 1
fi

echo ""
info "CMake configuration successful!"
echo ""

# 编译项目
info "Building project with $PARALLEL_JOBS parallel jobs..."
echo ""

cmake --build . --parallel "$PARALLEL_JOBS"

if [ $? -ne 0 ]; then
    error "Build failed"
    exit 1
fi

echo ""
echo "=========================================="
echo -e "${GREEN}Build successful!${NC}"
echo "=========================================="
echo ""
info "Output files:"
echo "  - Executable: $BUILD_DIR/bin/mmo_server"
echo "  - Lua: $BUILD_DIR/bin/lua"
echo "  - Libraries: $BUILD_DIR/lib/"
echo ""

cd ..

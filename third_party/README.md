# Third Party Dependencies

This directory contains third-party libraries required by the MMO Server.

## Required Libraries

### 1. Lua 5.4
- Website: https://www.lua.org/
- Download: https://www.lua.org/ftp/lua-5.4.6.tar.gz
- Install on Windows:
  - Use vcpkg: `vcpkg install lua:x64-windows`
  - Or download pre-built binaries

### 2. LuaBridge
- GitHub: https://github.com/vinniefalco/LuaBridge
- Version: 2.8
- Header-only library
- Clone: `git clone https://github.com/vinniefalco/LuaBridge.git LuaBridge`

### 3. Asio (Standalone)
- Website: https://think-async.com/Asio/
- GitHub: https://github.com/chriskohlhoff/asio
- Version: 1.28.0+
- Header-only library
- Clone: `git clone https://github.com/chriskohlhoff/asio.git`

### 4. spdlog (Optional, for logging)
- GitHub: https://github.com/gabime/spdlog
- Version: 1.12.0+
- Clone: `git clone https://github.com/gabime/spdlog.git`

### 5. Protocol Buffers (Required for proto3)
- Website: https://developers.google.com/protocol-buffers
- GitHub: https://github.com/protocolbuffers/protobuf
- Version: 3.21.0+
- Install on Windows:
  - Use vcpkg: `vcpkg install protobuf:x64-windows`
- Install on Linux:
  - `sudo apt-get install libprotobuf-dev protobuf-compiler`

## Directory Structure

```
third_party/
├── LuaBridge/
│   ├── Source/
│   │   └── LuaBridge/
│   │       ├── LuaBridge.h
│   │       └── ...
├── asio/
│   └── asio/
│       └── include/
├── spdlog/
│   └── include/
├── protobuf/
│   └── cmake/          (for protobuf cmake integration)
└── lua/
    └── (Lua source or include files)
```

## Quick Setup (Windows with vcpkg)

```powershell
# Install vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install lua:x64-windows
.\vcpkg install spdlog:x64-windows
.\vcpkg install asio:x64-windows
.\vcpkg install protobuf:x64-windows

# Integrate with Visual Studio
.\vcpkg integrate install
```

## Quick Setup (Linux)

```bash
# Ubuntu/Debian
sudo apt-get install lua5.4 liblua5.4-dev libspdlog-dev cmake g++
sudo apt-get install libprotobuf-dev protobuf-compiler

# CentOS/RHEL
sudo yum install lua lua-devel spdlog-devel protobuf-devel protobuf-compiler

# Or use vcpkg on Linux
./vcpkg install lua:x64-linux
./vcpkg install spdlog:x64-linux
./vcpkg install protobuf:x64-linux
```

## CMake Integration

The project's CMakeLists.txt will automatically find these libraries:
- Lua: via `find_package(Lua)`
- spdlog: via `find_package(spdlog)`
- Protobuf: via `find_package(Protobuf)`
- Asio: bundled in third_party directory
- LuaBridge: bundled in third_party directory

## Protocol Buffers Usage

### Compiling .proto files manually

```bash
# Generate C++ code from proto files
protoc --cpp_out=./include/proto --proto_path=./proto ./proto/*.proto
```

### CMake automatic generation

CMake will automatically generate C++ code from .proto files during build:
```cmake
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
```

## Lua Protobuf Integration

The server provides Lua bindings for Protocol Buffers:

```lua
-- Set proto file path
protobuf.set_path("proto")

-- Import proto files
protobuf.import("common.proto")
protobuf.import("login.proto")

-- Encode a message
local login_req = {
    account = "test_user",
    password = "test_pass",
    platform = "pc"
}
local data = protobuf.encode("mmo.LoginRequest", login_req)

-- Decode a message
local decoded = protobuf.decode("mmo.LoginRequest", data)
print(decoded.account)
```

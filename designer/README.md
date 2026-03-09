# Excel to SQLite 转译系统

将游戏配置Excel表格转换为SQLite数据库和二进制文件的工具。

## 功能特性

- **Excel读取**: 支持读取.xlsx格式的Excel文件
- **多Sheet支持**: 每个Sheet转换为一张数据库表
- **字段类型映射**: 支持多种数据类型自动识别
- **二进制序列化**: 将数据序列化为二进制格式存储
- **SQLite存储**: 每个字段对应数据库列，同时存储二进制数据
- **批量转换**: 支持目录批量转换

## 目录结构

```
designer/
├── include/excel_converter/    # 头文件
│   ├── excel_reader.h          # Excel读取器
│   ├── binary_serializer.h     # 二进制序列化器
│   └── excel_converter.h       # 转换器主类
├── src/                        # 源文件
│   ├── excel_reader.cpp
│   ├── binary_serializer.cpp
│   ├── excel_converter.cpp
│   └── main.cpp                # 命令行入口
├── excel/                      # 输入Excel文件目录
├── output/                     # 输出SQLite文件目录
├── third_party/                # 第三方库
│   ├── sqlite3/                # SQLite3源码
│   └── xlsxio/                 # XLSXIO库
└── CMakeLists.txt              # CMake配置
```

## 依赖库

| 库名 | 版本 | 说明 |
|------|------|------|
| SQLite3 | 3.45+ | 数据库存储 |
| XLSXIO | 0.2+ | Excel文件读取 |
| libexpat | 2.0+ | XML解析(XLSXIO依赖) |

## 编译

### Windows

1. 安装依赖库：
   - 下载 [XLSXIO](https://github.com/brechtsanders/xlsxio) 并解压到 `third_party/xlsxio/`
   - SQLite3会自动下载

2. 编译：
```powershell
cd designer
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Linux

```bash
# 安装依赖
sudo apt-get install libsqlite3-dev libxlsxio-dev libexpat1-dev cmake g++

# 编译
cd designer
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## 使用方法

### 命令行参数

```
Usage: excel_converter [options] <input_path>

Options:
  -i, --input <path>      输入Excel文件或目录
  -o, --output <path>     输出SQLite数据库路径
  -n, --name-row <n>      字段名所在行 (默认: 1)
  -t, --type-row <n>      字段类型所在行 (默认: 2)
  -c, --comment-row <n>   注释所在行 (默认: 3)
  -d, --data-row <n>      数据起始行 (默认: 4)
  -v, --verbose           详细输出
  -q, --quiet             静默模式
  -h, --help              显示帮助
```

### 使用示例

```bash
# 转换单个文件
./excel_converter -i config.xlsx -o output.db

# 转换目录下所有Excel文件
./excel_converter -i ./excel/ -o ./output/

# 使用默认设置
./excel_converter config.xlsx

# 详细模式
./excel_converter -v -i config.xlsx
```

## Excel表格格式

### 标准格式

Excel表格需要遵循以下格式：

| 行号 | 内容 | 说明 |
|------|------|------|
| 1 | 字段名 | 定义每列的字段名称 |
| 2 | 字段类型 | 定义每列的数据类型 |
| 3 | 注释 | 字段说明（可选） |
| 4+ | 数据行 | 实际数据内容 |

### 示例表格

**player.xlsx - 玩家配置表**

| id | name | level | exp | hp | mp | attack | defense | skills | is_vip |
|----|------|-------|-----|----|----|--------|---------|--------|--------|
| int32 | string | int32 | int64 | int32 | int32 | float | float | int32[] | bool |
| 玩家ID | 玩家名称 | 等级 | 经验值 | 生命值 | 魔法值 | 攻击力 | 防御力 | 技能列表 | 是否VIP |
| 1001 | 张三 | 50 | 100000 | 1000 | 500 | 150.5 | 80.0 | 1,2,3 | true |
| 1002 | 李四 | 30 | 50000 | 800 | 400 | 120.0 | 60.0 | 2,4 | false |
| 1003 | 王五 | 80 | 500000 | 2000 | 800 | 200.0 | 100.0 | 1,2,3,4,5 | true |

**item.xlsx - 道具配置表**

| id | name | type | quality | price | description | attrs |
|----|------|------|---------|-------|-------------|-------|
| int32 | string | int32 | int32 | int32 | string | string |
| 道具ID | 道具名称 | 类型 | 品质 | 价格 | 描述 | 属性JSON |
| 2001 | 铁剑 | 1 | 1 | 100 | 新手武器 | {"atk":10} |
| 2002 | 钢剑 | 1 | 2 | 500 | 进阶武器 | {"atk":30,"crit":5} |
| 2003 | 龙剑 | 1 | 5 | 10000 | 传说武器 | {"atk":100,"crit":20,"hp":500} |

## 支持的字段类型

| 类型名 | 别名 | 说明 | SQLite类型 |
|--------|------|------|------------|
| int32 | int, i32 | 32位整数 | INTEGER |
| int64 | long, i64 | 64位整数 | INTEGER |
| uint32 | u32 | 无符号32位整数 | INTEGER |
| uint64 | u64 | 无符号64位整数 | INTEGER |
| float | f32 | 单精度浮点数 | REAL |
| double | f64 | 双精度浮点数 | REAL |
| string | str, text | 字符串 | TEXT |
| bool | boolean | 布尔值 | INTEGER |
| bytes | binary, blob | 二进制数据 | BLOB |

### 数组类型

在类型后添加 `[]` 表示数组，例如：
- `int32[]` - 整数数组
- `string[]` - 字符串数组

数组值使用逗号 `,`、分号 `;` 或竖线 `|` 分隔。

## 数据库结构

### 表结构

每个Excel Sheet会生成一张对应的数据库表，表结构如下：

```sql
CREATE TABLE "sheet_name" (
    "_id" INTEGER PRIMARY KEY AUTOINCREMENT,
    "field1" TYPE,
    "field2" TYPE,
    ...
    "_binary" BLOB
);
```

### 字段说明

- `_id`: 自增主键
- 各字段: 与Excel列一一对应
- `_binary`: 该行数据的二进制序列化结果

## 二进制格式

二进制数据格式（小端序）：

```
字段1数据 + 字段2数据 + ... + 字段N数据
```

### 各类型编码方式

| 类型 | 编码方式 |
|------|----------|
| int32/int64/uint32/uint64 | 直接写入字节 |
| float/double | 直接写入字节 |
| string | 4字节长度 + UTF-8字符串 |
| bool | 1字节 (0或1) |
| bytes | 4字节长度 + 数据 |
| 数组 | 4字节元素个数 + 各元素数据 |

## API使用

```cpp
#include "excel_converter/excel_converter.h"

int main() {
    // 创建转换器
    excel_converter::ExcelConverter converter;
    
    // 配置选项
    excel_converter::ConvertOptions options;
    options.excel_path = "config.xlsx";
    options.output_db_path = "output.db";
    options.verbose = true;
    
    // 初始化
    converter.Initialize(options);
    
    // 设置进度回调
    converter.SetProgressCallback([](const std::string& stage, int current, int total) {
        std::cout << "[" << current << "/" << total << "] " << stage << std::endl;
    });
    
    // 执行转换
    auto result = converter.Convert();
    
    // 输出结果
    if (result.success) {
        std::cout << "转换成功!" << std::endl;
        std::cout << "共转换 " << result.success_sheets << " 个表" << std::endl;
        std::cout << "共 " << result.success_rows << " 行数据" << std::endl;
    }
    
    return 0;
}
```

## 错误处理

转换过程中可能出现的错误：

| 错误 | 原因 | 解决方法 |
|------|------|----------|
| Failed to open Excel file | 文件不存在或格式错误 | 检查文件路径和格式 |
| Insufficient rows for header | 表格行数不足 | 确保至少有4行(字段名、类型、注释、数据) |
| Failed to create table | 表名无效或已存在 | 检查Sheet名称 |
| Failed to serialize table | 数据类型不匹配 | 检查数据格式是否与类型定义一致 |

## 性能优化

- 使用事务批量插入数据
- WAL模式提高并发性能
- 内存映射读取大文件

## 注意事项

1. Excel文件必须是.xlsx格式
2. Sheet名称会作为数据库表名，避免使用SQL关键字
3. 字段名不能重复
4. 空行会被自动跳过
5. 数组元素不能包含分隔符(,;|)

## License

MIT License

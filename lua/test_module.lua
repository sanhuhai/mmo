-- test_module.lua
-- 测试模块调用功能

local test_module = {}

function test_module.hello(name)
    return "Hello, " .. (name or "World") .. "!"
end

function test_module.add(a, b)
    return (a or 0) + (b or 0)
end

function test_module.get_info()
    return {
        name = "Test Module",
        version = "1.0.0",
        author = "LuaPanda"
    }
end

-- 全局导出
g_test_module = test_module

return test_module

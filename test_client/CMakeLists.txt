# ==========================================
# 模拟 客户端的编译配置
# ==========================================

# 搜集当前目录下的源码
file(GLOB CLIENT_SRCS "*.cpp")

# 生成可执行文件
add_executable(mock_client ${CLIENT_SRCS})

# 链接 rpc_core 库
target_link_libraries(mock_client rpc_core)
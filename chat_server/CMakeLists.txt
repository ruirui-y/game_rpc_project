# ==========================================
# 聊天服务节点
# ==========================================

# 搜集当前目录下的源码
file(GLOB CHAT_SRCS "*.cpp")

# 生成可执行文件
add_executable(chat_server ${CHAT_SRCS})

# 链接 rpc_core 库
target_link_libraries(chat_server rpc_core)
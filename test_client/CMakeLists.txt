# ==========================================
# 模拟 客户端的编译配置
# ==========================================

# 确保能找到生成的 protobuf 头文件
include_directories(${CMAKE_SOURCE_DIR}/pb)

# 编译可执行文件
add_executable(mock_client mock_client.cpp)

# 链接依赖：
# 1. rpc_core (直接白嫖里面已经编译好的 client_gateway.pb.o 等所有协议对象)
# 2. Protobuf_LIBRARIES (反序列化必备)
# 3. pthread (多线程/网络基础依赖)
target_link_libraries(mock_client rpc_core ${Protobuf_LIBRARIES} pthread)
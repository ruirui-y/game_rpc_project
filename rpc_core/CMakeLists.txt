# ==========================================
# 专门负责 rpc_core 静态库的编译和 proto 生成
# ==========================================

set(PROTO_SRC_DIR ${CMAKE_SOURCE_DIR}/proto)
set(PB_OUT_DIR ${CMAKE_SOURCE_DIR}/pb)

# 1. 找到所有的 .proto 文件
file(GLOB PROTO_FILES "${PROTO_SRC_DIR}/*.proto")

# 2. 预期生成的文件
set(PROTO_PCH_SRCS
    ${PB_OUT_DIR}/login.pb.cc
    ${PB_OUT_DIR}/rpcheader.pb.cc
)
set(PROTO_PCH_HDRS
    ${PB_OUT_DIR}/login.pb.h
    ${PB_OUT_DIR}/rpcheader.pb.h
)

# 3. 自定义命令：执行 protoc
add_custom_command(
    OUTPUT ${PROTO_PCH_SRCS} ${PROTO_PCH_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${PB_OUT_DIR} -I${PROTO_SRC_DIR} ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    COMMENT "正在使用 protoc 编译 .proto 文件到 pb/ 目录..."
)

# 4. 获取 rpc_core/src 下的源码 (目前可能为空，没关系)
file(GLOB RPC_CORE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 5. 生成静态库 rpc_core
add_library(rpc_core STATIC ${RPC_CORE_SRCS} ${PROTO_PCH_SRCS})

# 6. 链接依赖的底层库：Protobuf、pthread、以及你之前写的 mymuduo
target_link_libraries(rpc_core PUBLIC ${Protobuf_LIBRARIES} pthread mymuduo)
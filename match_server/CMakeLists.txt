# ==========================================
# 匹配服务节点
# ==========================================
file(GLOB MATCH_SRCS "*.cpp")
add_executable(match_server ${MATCH_SRCS})
target_link_libraries(match_server rpc_core)
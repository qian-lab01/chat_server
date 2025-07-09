cmake_minimum_required(VERSION 3.0)
project(web_server)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置项目根目录变量（避免硬编码绝对路径）
set(ROOT_DIR "${CMAKE_SOURCE_DIR}")

# 包含目录
include_directories(${ROOT_DIR}/include)

# 收集源文件（建议明确列出文件而非使用 glob）
file(GLOB SRC_LIST ${ROOT_DIR}/src/*.cpp)
message(STATUS "Source files: ${SRC_LIST}")  # 打印源文件列表，方便调试

# 添加可执行文件
add_executable(main ${SRC_LIST})

# 链接依赖库
target_link_libraries(main pthread mysqlcppconn curl)  # 示例：链接 pthread 库
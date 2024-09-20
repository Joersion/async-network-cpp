cmake_minimum_required(VERSION 3.12)
project(asio_socket)

ADD_DEFINITIONS(
-O3 
)

#高版本防止告警
cmake_policy(SET CMP0144 NEW)

#指定编译器
set(CMAKE_C_COMPILER "aarch64-rockchip1031-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-rockchip1031-linux-gnu-g++")

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 设置 Boost 的路径
set(BOOST_ROOT ~/work/boost/aarch64)

#优先静态库
set(Boost_USE_STATIC_LIBS ON)

# 查找 Boost 库的组件
find_package(Boost REQUIRED COMPONENTS system thread)

# 添加可执行文件和源文件
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(asio_socket SHARED ${SOURCES})

# 包含头文件路径（包括其他目录）
target_include_directories(asio_socket PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 包含 Boost 头文件路径
target_include_directories(asio_socket PRIVATE ${Boost_INCLUDE_DIRS})

# 链接 Boost 库
target_link_libraries(asio_socket PRIVATE pthread ${Boost_LIBRARIES})
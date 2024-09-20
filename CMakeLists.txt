cmake_minimum_required(VERSION 3.12)
project(AsioSocket)

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

#连接库路径
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/build)

# 添加可执行文件和源文件
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ServerTest.cpp)

add_executable(serverTest ${SOURCES})

# 包含头文件路径
target_include_directories(serverTest PRIVATE ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/)

# 链接 Boost 库
target_link_libraries(serverTest PRIVATE pthread ${Boost_LIBRARIES} asio_socket)
#------------------------------------------------------------------------------------------------------------
# 添加可执行文件和源文件
file(GLOB SOURCES2 ${CMAKE_CURRENT_SOURCE_DIR}/ClientTest.cpp)

add_executable(clientTest ${SOURCES2})

# 包含头文件路径
target_include_directories(clientTest PRIVATE ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/)

# 链接 Boost 库
target_link_libraries(clientTest PRIVATE pthread ${Boost_LIBRARIES} asio_socket)
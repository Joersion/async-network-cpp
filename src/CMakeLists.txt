cmake_minimum_required(VERSION 3.12)
project(async_network)

ADD_DEFINITIONS(
-O3 
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# 设置 Boost 的路径
set(BOOST_ROOT ~/work/boost/x86)

#优先静态库
set(Boost_USE_STATIC_LIBS ON)

# 查找 Boost 库的组件
find_package(Boost REQUIRED COMPONENTS system thread)

# 添加可执行文件和源文件
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(async_network SHARED ${SOURCES})

target_compile_options(async_network PRIVATE -g)

# 包含头文件路径（包括其他目录）
target_include_directories(async_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 包含 Boost 头文件路径
target_include_directories(async_network PRIVATE ${Boost_INCLUDE_DIRS})

# 链接 Boost 库
target_link_libraries(async_network PRIVATE pthread ${Boost_LIBRARIES})
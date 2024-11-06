cmake_minimum_required(VERSION 3.12)
project(AsioSocket)

ADD_DEFINITIONS(
-O3 
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g -Wall")

# set(CMAKE_CXX_COMPILER "aarch64-rockchip1031-linux-gnu-g++")
# 设置 Boost 的路径
set(BOOST_ROOT ~/work/boost/x86)

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
target_link_libraries(serverTest PRIVATE pthread ${Boost_LIBRARIES} async_network)
#------------------------------------------------------------------------------------------------------------
# 添加可执行文件和源文件
file(GLOB SOURCES2 ${CMAKE_CURRENT_SOURCE_DIR}/ClientTest.cpp)

add_executable(clientTest ${SOURCES2})

# 包含头文件路径
target_include_directories(clientTest PRIVATE ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/)

# 链接 Boost 库
target_link_libraries(clientTest PRIVATE pthread ${Boost_LIBRARIES} async_network)

#------------------------------------------------------------------------------------------------------------
# 添加可执行文件和源文件
file(GLOB SOURCES3 ${CMAKE_CURRENT_SOURCE_DIR}/HttpTest.cpp)

add_executable(httpTest ${SOURCES3})
target_compile_options(httpTest PRIVATE -g)

# 包含头文件路径
target_include_directories(httpTest PRIVATE ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/)

# 链接 Boost 库
target_link_libraries(httpTest PRIVATE pthread ${Boost_LIBRARIES} async_network)

#------------------------------------------------------------------------------------------------------------
# 添加可执行文件和源文件
file(GLOB SOURCES4 ${CMAKE_CURRENT_SOURCE_DIR}/UartTest.cpp)

add_executable(uartTest ${SOURCES4})
target_compile_options(uartTest PRIVATE -g)

# 包含头文件路径
target_include_directories(uartTest PRIVATE ${Boost_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/)

# 链接 Boost 库
target_link_libraries(uartTest PRIVATE pthread ${Boost_LIBRARIES} async_network)
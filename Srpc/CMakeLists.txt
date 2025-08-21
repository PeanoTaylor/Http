cmake_minimum_required(VERSION 3.15)

# 项目名称
project(example01)

# 指定C++的版本
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../)

# 生成的可执行程序
add_executable(server
    server.cpp
    example.pb.cpp
)

target_compile_options(server PRIVATE
    -g
)

target_link_libraries(server PRIVATE 
    srpc
    lz4
    snappy
    protobuf
    workflow
)

add_executable(client
    client.cpp
    example.pb.cpp
)

target_compile_options(client PRIVATE
    -g
)

target_link_libraries(client PRIVATE
    srpc
    lz4
    snappy
    protobuf
    workflow
)
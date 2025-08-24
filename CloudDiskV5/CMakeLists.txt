cmake_minimum_required(VERSION 3.15)
project(CloudDisk)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)

# ========== server (RPC服务端) ==========
add_executable(rpc_server
    rpcServer.cpp
    clouddisk.pb.cpp
    CryptoUtil.cpp
    OssClientWrapper.cpp
    MqClientWrapper.cpp
    check_healthy.cpp
)
target_link_libraries(rpc_server PRIVATE
    workflow
    wfrest
    srpc
    protobuf
    crypto
    ssl
    jwt
    pthread
    curl
    alibabacloud-oss-cpp-sdk
    SimpleAmqpClient
    rabbitmq
    ppconsul
    pthread
    boost_system
    boost_chrono
)


# ========== cloudisk_server (HTTP网关) ==========
add_executable(cloudisk_server
    CloudiskServer.cpp
    clouddisk.pb.cpp
    CryptoUtil.cpp
    OssClientWrapper.cpp
    MqClientWrapper.cpp
    main.cpp
    check_healthy.cpp
)
target_link_libraries(cloudisk_server PRIVATE
    workflow
    wfrest
    srpc
    protobuf
    crypto
    ssl
    jwt
    pthread
    curl
    alibabacloud-oss-cpp-sdk
    SimpleAmqpClient
    rabbitmq
    ppconsul
    pthread
    boost_system
    boost_chrono
)

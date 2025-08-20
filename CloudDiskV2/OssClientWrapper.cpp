#include "OssClientWrapper.hpp"
#include <iostream>
using namespace AlibabaCloud::OSS;
using namespace std;

OssClientWrapper::OssClientWrapper(const string &endpoint, const string &accessKeyId, const string &accessKeySecret)
        :_conf(), 
         _client(endpoint, accessKeyId, accessKeySecret, _conf){}

OssClientWrapper::~OssClientWrapper(){
    // 释放 OSS SDK 资源
    ShutdownSdk();
}

void OssClientWrapper::putObject(const string& bucketName,const string& objectName,const string& localFile){
    
    auto outcome = _client.PutObject(bucketName, objectName, localFile);
    if (!outcome.isSuccess()) {
        /* 异常处理。*/
        std::cout << "PutObject fail" <<
        ",code:" << outcome.error().Code() <<
        ",message:" << outcome.error().Message() <<
        ",requestId:" << outcome.error().RequestId() << std::endl;
    }
}


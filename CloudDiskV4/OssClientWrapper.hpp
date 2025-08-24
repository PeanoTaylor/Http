#pragma once
#include <string>
#include <alibabacloud/oss/OssClient.h>
using std::string;
class OssClientWrapper
{
public:
    OssClientWrapper(const string &endpoint, const string &accessKeyId, const string &accessKeySecret);
    ~OssClientWrapper();

    // 上传文件（失败抛异常）
    void putObject(const string &bucketName, const string &objectName, const string &localFile);

private:
    AlibabaCloud::OSS::ClientConfiguration _conf;
    AlibabaCloud::OSS::OssClient _client;
};

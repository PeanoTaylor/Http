#include <alibabacloud/oss/OssClient.h>

using namespace AlibabaCloud::OSS;

int main(void)
{
    /* 初始化网络等资源。*/
    InitializeSdk();

    // 1. 初始化OSS账号信息
    std::string endpoint = "";
    std::string accessKeyId = "";
    std::string accessKeySercret = "";
    //std::string region = "cn-wuhan";

    ClientConfiguration conf;
    OssClient client { endpoint, accessKeyId, accessKeySercret, conf };
    //client.SetRegion(region);

    // 2. 上传文件
    std::string bucketName = "peano-taylor";
    std::string objectName = "dir/a.txt";
    auto outcome = client.PutObject(bucketName, objectName, "a.txt");

    if (!outcome.isSuccess()) {
        /* 异常处理。*/
        std::cout << "PutObject fail" <<
        ",code:" << outcome.error().Code() <<
        ",message:" << outcome.error().Message() <<
        ",requestId:" << outcome.error().RequestId() << std::endl;
        return -1;
    }

    /* 释放网络等资源。*/
    ShutdownSdk();
    return 0;
}
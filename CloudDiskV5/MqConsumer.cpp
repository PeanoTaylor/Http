#include <iostream>
#include "MqClientWrapper.hpp"
#include "OssClientWrapper.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <chrono>

using namespace std;
using json = nlohmann::json;

int main()
{
    // 使用默认 vhost "/"
    MqClientWrapper mq("127.0.0.1", 5672, "guest", "guest", "/");

    mq.declare("cloudisk.exchange", "file.upload.queue", "file.upload");
    Envelope::ptr_t env;

    cout << " [*] MQ Consumer 已启动，等待消息..." << endl;

    while (true)   // 永不退出
    {
        if (mq.consume("file.upload.queue", env))   // 收到消息
        {
            try {
                string body = env->Message()->Body();
                json msg = json::parse(body);

                if (msg["operation"] == "upload")
                {
                    string filename = msg["filename"];
                    string hashcode = msg["hashcode"];
                    string local_path = "./static/view/upload_files/" + hashcode;

                    OssClientWrapper oss{"",
                                         "",
                                         ""};

                    oss.putObject("peano-taylor", "dir/" + filename, local_path);
                    cout << " [x] 异步上传完成: " << filename << endl;
                }

                // 消费成功要 ack，否则消息可能会被重复投递
                mq.ack(env);
            }
            catch (const std::exception& e) {
                cerr << " [!] 处理消息时发生错误: " << e.what() << endl;
            }
        }
        else
        {
            // 没有消息 → 休眠 1 秒后继续循环
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0; // 实际上不会走到这里
}

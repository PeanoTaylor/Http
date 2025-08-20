/*
 *  [[Redis URL]]
 *
 *  redis://:password@host:port/dbnum?query#fragment
 *
 *  password: 可选
 *  port: 默认为 6379
 *  dbnum: 默认为0，范围是0-15
 *
 *  示例：
 *    - redis://127.0.0.1/
 *    - redis://12345678@redis.example.com/1
 */

#include <wfrest/HttpServer.h>
#include <iostream>

using namespace std;
using namespace wfrest;
using namespace protocol;

int main()
{
    HttpServer server;

    /*
     * void Redis(const std::string& url, const std::string& command,
     *            const std::vector<std::string>& params);
     */
    server.GET("/redis0", [](const HttpReq* req, HttpResp* resp)
    {
        // 原理：创建Redis任务，push_back到SeriesWork中，再返回结果
        resp->Redis("redis://127.0.0.1/", "SET", { "test_key", "test_val" });   
    });

    server.GET("/redis1", [](const HttpReq* req, HttpResp* resp)
    {
        resp->Redis("redis://127.0.0.1", "GET", { "test_key" });
    });

    server.GET("/redis3", [](const HttpReq* req, HttpResp* resp)
    {
        resp->Redis("redis://127.0.0.1", "GET", { "test_key" }, [resp](WFRedisTask* task) {
            RedisValue value;
            task->get_resp()->get_result(value);
            resp->String(value.string_value());
        });
    });

    if (server.track().start(8888) == 0) {
        server.list_routes();
        getchar();              /* 按任意键终止程序 */
        server.stop();
    } else {
        cerr << "Error: cannot start server!\n";
        exit(1);
    }
    return 0;
}


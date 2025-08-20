#include <netdb.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/RedisMessage.h"
#include "workflow/Workflow.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

using namespace std;
using namespace protocol;

#define REDIRECT_MAX 5
#define RETRY_MAX 2

struct SeriesContext
{
    string httpUrl;
    size_t size;  // 网页的大小
    bool success; // 序列是否成功
};

void redis_callback(WFRedisTask *redisTask)
{
    // 1.判断状态
    int state = redisTask->get_state();
    if (state != WFT_STATE_SUCCESS)
    {
        cerr << WFGlobal::get_error_string(state, redisTask->get_error()) << endl;
        return;
    }

    // 2.获取Redis返回值
    RedisValue redisValue;
    redisTask->get_resp()->get_result(redisValue);
    if (redisValue.is_error())
    {
        cerr << "Redis error reply!" << endl;
        return;
    }
    // 3.获取上下文
    SeriesContext *context = static_cast<SeriesContext *>(series_of(redisTask)->get_context());
    // 序列执行成功，context->success设为true
    context->success = true;
}

void http_callback(WFHttpTask *httpTask)
{
    int state = httpTask->get_state();
    if (state != WFT_STATE_SUCCESS)
    {
        cerr << WFGlobal::get_error_string(state, httpTask->get_error()) << endl;
        return;
    }

    // 获取任务所在序列和上下文
    SeriesWork *series = series_of(httpTask);
    SeriesContext *context = static_cast<SeriesContext *>(series->get_context());

    // 获取响应体
    HttpResponse *resp = httpTask->get_resp();

    const void *body;
    size_t body_len;
    resp->get_parsed_body(&body, &body_len);
    context->size = body_len;
    // 创建Redis任务
    string redisUrl = "redis://127.0.0.1";
    WFRedisTask *redisTask = WFTaskFactory::create_redis_task(redisUrl, RETRY_MAX, redis_callback);
    ;
    string value{(char *)body, body_len};
    redisTask->get_req()->set_request("SET", {context->httpUrl, value});
    series->push_back(redisTask);
}

int main(int argc, char *argv[])
{

    // 1.检查参数
    if (argc != 2)
    {
        cerr << "USAGE: %s <http URL>" << endl;
        exit(1);
    }

    // 2.创建HTTP任务
    WFHttpTask *httpTask = WFTaskFactory::create_http_task(argv[1], REDIRECT_MAX, RETRY_MAX, http_callback);
    // 设置请求
    HttpRequest *req = httpTask->get_req();
    req->set_header_pair("Accept", "/");                         // 可接受任意类型——OK
    req->set_header_pair("User-Agent", "Wget/1.14 (linux-gnu)"); // UA-OK
    req->set_header_pair("Connection", "close");                 // 禁用keep-alive,不建议常用

    // 设置响应时间
    httpTask->set_receive_timeout(30 * 1000);
    // 设置响应最大大小
    httpTask->get_resp()->set_size_limit(20 * 1024 * 1024);
    WFFacilities::WaitGroup wait_group{1};
    // 创建任务序列
    SeriesWork *series = Workflow::create_series_work(httpTask, [&wait_group](const SeriesWork *series)
                                                      {
        SeriesContext *context = static_cast<SeriesContext*>(series->get_context());

        if(context->success){
            cout << "Success"<<endl;
        }else{
            cerr<<"Failed"<<endl;
        }// 序列结束
        delete context;// 释放上下文
        wait_group.done();
    });

    // 设置序列的上下文
    SeriesContext *context = new SeriesContext{argv[1],0,false};// 堆上
    series->set_context(context);
    // 启动序列
    series->start();

    wait_group.wait();

    return 0;
}

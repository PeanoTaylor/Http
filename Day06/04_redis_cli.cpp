#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "workflow/RedisMessage.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
using namespace std;
using namespace protocol;
#define RETRY_MAX 2

struct tutorial_task_data
{
    std::string url;
    std::string key;
    std::string value;
};

void print_redis_value(const RedisValue &val)
{
    if (val.is_string())
    {
        cout << val.string_value() << endl;
    }
    else if (val.is_int())
    {
        cout << val.int_value() << endl;
    }
    else if (val.is_nil())
    {
        cout << "nil" << endl;
    }
    else if (val.is_error())
    {
        cout << "error" << endl;
    }
    else if (val.is_array())
    {
        int size = val.arr_size();
        for (int i = 0; i < size; ++i)
        {
            print_redis_value(val);
            cout << " ";
        }
    }
    cout << "Finish. Press Ctrl-C to exit." << endl;
}

void redis_callback(WFRedisTask *task)
{
    RedisRequest *req = task->get_req();
    RedisResponse *resp = task->get_resp();
    int state = task->get_state();
    RedisValue val;
    resp->get_result(val);

    if (state != WFT_STATE_SUCCESS)
    {
        fprintf(stderr, "Failed. Press Ctrl-C to exit.\n");
        return;
    }

    // 2.获取执行命令
    std::string cmd;
    req->get_command(cmd);
    if (cmd == "SET")
    {
        tutorial_task_data *data = (tutorial_task_data *)task->user_data;
        WFRedisTask *next = WFTaskFactory::create_redis_task(data->url, RETRY_MAX, redis_callback);
        next->get_req()->set_request("GET", {data->key});
        series_of(task)->push_back(next);
        cout << "Redis SET request success. Trying to GET..." << endl;
    }
    else if (cmd == "GET")
    {
        print_redis_value(val);
    }
}

static WFFacilities::WaitGroup wait_group{1};

void sig_handler(int signum)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    struct tutorial_task_data data;

    // 1.创建任务
    data.url = "redis://127.0.0.1";
    WFRedisTask *task = WFTaskFactory::create_redis_task(data.url, RETRY_MAX, redis_callback);

    // 2.设置请求
    RedisRequest *req = task->get_req();
    data.key = "name";
    data.value = "zhangsan";
    req->set_request("SET", {data.key, data.value});

    task->user_data = &data;
    task->start();
    wait_group.wait();
    return 0;
}

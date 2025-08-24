#include <iostream>
#include <wfrest/HttpServer.h>
#include <chrono>
#include "ppconsul/agent.h"

using ppconsul::Consul;
using namespace ppconsul::agent;
using std::cout;
using std::endl;

void timer_callback(WFTimerTask *task, Agent &agent)
{
    agent.servicePass("UserService1"); // 发送心跳包
    WFTimerTask *timerTask = WFTaskFactory::create_timer_task(5, 0, std::bind(timer_callback, std::placeholders::_1, std::ref(agent)));
    series_of(task)->push_back(timerTask);
}

int main()
{
    using namespace wfrest;

    HttpServer server{};

    if (server.start(8888) == 0)
    {
        // 指定注册中心 Consul 的ip、port和数据中心
        Consul consul{"http://127.0.0.1:8500"};
        // 创建代理
        Agent agent{consul};

        // 注册服务
        agent.registerService(
            kw::id = "UserService1",
            kw::name = "UserService",
            kw::address = "127.0.0.1",
            kw::port = 8888,
            kw::check = TtlCheck(std::chrono::seconds{10}));

        // 定时发送心跳包
        WFTimerTask *timerTask = WFTaskFactory::create_timer_task(5, 0, std::bind(timer_callback, std::placeholders::_1, std::ref(agent)));

        SeriesWork *series = Workflow::create_series_work(timerTask, nullptr);
        series->set_context(&agent);
        series->start();

        getchar();
        server.stop();
    }

    return 0;
}

#include <wfrest/HttpServer.h>
#include <iostream>

using namespace wfrest;

int main()
{
    HttpServer server;

    server.GET("/series",[](const HttpReq* req, HttpResp* resp,SeriesWork *series){
        WFTimerTask *timeTask = WFTaskFactory::create_timer_task(3,0,[](WFTimerTask*){
            std::cout << "定时器任务完成(3秒)\n";
        });
        series->push_back(timeTask);
        resp->String("TimeTask");
    });


    if (server.start(8888) == 0) {
        getchar();
        server.stop();
    } else {
        std::cerr << "Error: cannot start server!\n";
    }
    return 0;
}


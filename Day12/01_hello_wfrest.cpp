#include <iostream>
#include <wfrest/HttpServer.h>
using namespace wfrest;
using namespace std;

int main()
{
    HttpServer server;

    server.GET("/hello",[](const HttpReq *req,HttpResp *resp){
        resp->String("hello wfrest");
    });

    if(server.track().start(8888)==0){// 链式调用
        server.list_routes();// 打印所有注册的路由
        getchar();
        server.stop();
    }else{
        cerr << "Error: cannot start server!" << endl;
        exit(1);
    }
    return 0;
}


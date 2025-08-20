#include <iostream>
#include "wfrest/HttpServer.h"
using namespace std;
using namespace wfrest;

int main()
{
    HttpServer server;
    // 301永久重定向
    server.GET("/test301",[](const HttpReq *req,HttpResp *resp){
        resp->set_status(301);
        resp->set_header_pair("Location", "http://127.0.0.1:8888/target");
    });

    server.GET("/target",[](const HttpReq *req,HttpResp *resp){
        resp->String("<html>This is 301 redirect.</html>");
    });


    // 303查看其他地址
    server.GET("/test303",[](const HttpReq *req,HttpResp *resp){
        resp->set_status(303);
        resp->set_header_pair("Location", "http://127.0.0.1:8888/target");
        resp->String("This is 303 See Other [GET].");
    });

    server.POST("/test303",[](const HttpReq *req,HttpResp *resp){
        resp->set_status(303);
        resp->set_header_pair("Location", "http://127.0.0.1:8888/target");
        resp->String("This is 303 See Other [POST].");
    });

    // 307临时重定向
    server.GET("/test307",[](const HttpReq *req,HttpResp *resp){
        resp->set_status(307);
        resp->set_header_pair("Location", "http://127.0.0.1:8888/target");
        resp->String("<html>This is 307 redirect.</html>");
    });

    if (server.start(8888) == 0) {
        getchar();  // 按任意键停止服务器
        server.stop();
    } else {
        cerr << "Error: server start failed!" << endl;
        exit(1);
    }
    return 0;
}


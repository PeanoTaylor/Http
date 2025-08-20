#include <iostream>
#include <string>
#include "wfrest/HttpServer.h"
using std::cout;
using std::endl;
using std::string;
using namespace wfrest;
using namespace protocol;

int main()
{
    HttpServer server; // 创建server对象

    // GET
    server.GET("/*", [](const HttpReq *req, HttpResp *resp) {
        cout<< req->get_method()<<" "<<req->get_request_uri() << " "<< req->get_http_version()<<"\r\n";

        HttpHeaderCursor cursor {req};
        string name,value;
        while(cursor.next(name,value)){
            cout << name << ":"<<value<<"\r\n";
        }
        cout << "\r\n";
        // GET没有body
    });

    // POST
    server.POST("/*", [](const HttpReq *req, HttpResp *resp) {
        cout<< req->get_method()<<" "<<req->get_request_uri() << " "<< req->get_http_version()<<"\r\n";

        HttpHeaderCursor cursor {req};
        string name,value;
        while(cursor.next(name,value)){
            cout << name << ":"<<value<<"\r\n";
        }
        cout << "\r\n";
        cout << req->body() << "\n";//解析body
    });

    if (server.start(8888) == 0)
    {
        getchar();
        server.stop();
    }
    else
    {
        cout << "start failed" << endl;
    }
    return 0;
}

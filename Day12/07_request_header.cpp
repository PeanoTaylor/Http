#include "wfrest/HttpServer.h"
#include <workflow/HttpUtil.h>
#include <iostream>

using namespace wfrest;

int main()
{
    HttpServer server;

    server.POST("/post", [](const HttpReq* req, HttpResp* resp)
    {
        // 获取请求头
        const std::string& host = req->header("Host");  
        const std::string& contentType = req->header("Content-Type");
        // 判断请求头是否存在
        if (req->has_header("User-Agent")) {    
            std::cout << "User-Agent: " << req->header("User-Agent") << "\n";
        }
        resp->String(host + " " + contentType + "\n");

        // 遍历所有请求头
        std::cout << "------------------\n";
        using namespace protocol;
        HttpHeaderCursor cursor { req };
        std::string name;
        std::string value;
        while (cursor.next(name, value)) {
            std::cout << name << ": " << value << "\n";
        }
    });

    if (server.start(8888) == 0) {
        getchar();  // 按任意键终止程序
        server.stop();
    } else {
        std::cerr << "Error: cannot start server!\n";
        std::exit(1);
    }
    return 0;
}


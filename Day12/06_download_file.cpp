#include <wfrest/HttpServer.h>

using namespace wfrest;

int main()
{
    
    HttpServer server;

    server.GET("/file1", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("todo.txt");
    });

    server.GET("/file2", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("html/index.html");
    });

    server.GET("/file3", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("/home/he/备课/wfrest/api/html/index.html");
    });

    server.GET("/file4", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("todo.txt", 0);
    });

    server.GET("/file5", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("todo.txt", 0, 10);
    });

    server.GET("/file6", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("html/index.html", 5, 10); //[start, end)
    });

    server.GET("/file7", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("todo.txt", 5, -1);
    });

    server.GET("/file8", [](const HttpReq*, HttpResp* resp)
    {
        resp->File("html/index.html", -5, -1);
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

/*
 * 总结：
 *   1. resp->File(filename, start=0, end=-1);
 */

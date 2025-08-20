#include <wfrest/HttpServer.h>

using namespace wfrest;
int main()
{
    
    HttpServer server;

    // 2. 设置路由
    server.Static("/static", "./file");   // url.path 映射 filepath(目录，文件)
    //server.Static("/public", "./www");
    //server.Static("/", "./www/index.html");

    if (server.start(8888) == 0) {
        getchar();
        server.stop();
    } else {
        std::cerr << "Error: cannot start server!\n";
        std::exit(1);
    }

    return 0;
}


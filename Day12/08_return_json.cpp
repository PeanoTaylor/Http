#include <wfrest/HttpServer.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main()
{
    using namespace wfrest;

    HttpServer server {};

    server.GET("/peanut", [](const HttpReq* req, HttpResp* resp)
    {
        json result = { {"name", "peanut"}, {"age", 18} };
        // resp->String(result.dump());
        resp->Json(result.dump());
    });

    if (server.track().start(8888) == 0) {
        server.list_routes();
        getchar();  /* 按任意键终止程序 */
        server.stop();
    } else {
        std::cerr << "Error: cannot start server!\n";
        std::exit(1);
    }
    return 0;
}


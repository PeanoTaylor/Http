#include <iostream>
#include <wfrest/HttpServer.h>
#include <map>
using namespace wfrest;
using namespace std;

int main()
{
    HttpServer server;

    server.GET("/query_list", [](const HttpReq *req, HttpResp *resp)
               {
                   const map<string, string> &queryList = req->query_list();

                   for (const auto &[name, value] : queryList)
                   {
                       cout << name << ":" << value << endl;
                   }

                   cout << req->query("username") << endl;
                   cout << req->query("password") << endl; });

    server.GET("/query", [](const HttpReq *req, HttpResp *resp)
               {
        const string& username = req->query("username");
        const string& password = req->query("password");
        const string& info = req->query("info");                        // 没有这个字段
        assert(info == "" && "info doesn't equal \"\"");
        
        const string& address = req->default_query("address", "china");

        cout << username << " " << password << " " << info << " " << address << endl; });

    // URL: /query_has?username=peanut&password=
    // 查询参数的值可以为空，所以不能通过查询参数的值是否为空，来判断参数的值是否存在
    // 而应该通过 has_query() 来判断
    server.GET("/query_has", [](const HttpReq *req, HttpResp *resp)
               {
        cout << req->query("password") << "\n";
        if (req->has_query("password")) {
            cout << "查询参数password存在!\n";
        }
        if (req->has_query("info")) {
            cout << "查询参数info存在!\n";
        } });

    if (server.track().start(8888) == 0)
    {                         // 链式调用
        server.list_routes(); // 打印所有注册的路由
        getchar();
        server.stop();
    }
    else
    {
        cerr << "Error: cannot start server!" << endl;
        exit(1);
    }
    return 0;
}

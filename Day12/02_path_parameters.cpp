#include <iostream>
#include <wfrest/HttpServer.h>
using namespace wfrest;
using namespace std;

int main()
{
    HttpServer server;

    // 2. 设置路由(相当于在process()函数中设置一个分支)

    // a. 参数路由
    // 此处理程序将匹配 /user/peanut，但不匹配 /user/ 或 /user
    server.GET("/user/{name}", [](const HttpReq *req, HttpResp *resp)
               {
        const string &name = req->param("name");
        cout << "name: " << name << endl;
        // 设置响应状态码
        resp->set_status(HttpStatusOK);
        resp->String("hello " + name); });

    // b. 精确路由
    // 无论定义的顺序如何，精确路由都会在参数路由之前解析
    // /user/xixi 永远不会被解释为 /user/{name} 路由
    server.GET("/user/lisi", [](const HttpReq *req, HttpResp *resp)
               {

        // 设置响应状态码
        resp->set_status(HttpStatusOK);
        resp->String("hi "); });

    // c. 通配符路由
    server.GET("/user/{name}/action*", [](const HttpReq *req, HttpResp *resp)
               {
        const string &name = req->param("name");
        cout << "name: " << name << endl;
        const string &matchPath = req->match_path();
        cout << "match_path: " << matchPath << endl;
        resp->String("[name: " + name + "] [math path: " + matchPath + "]\n");
        // 设置响应状态码
        resp->set_status(HttpStatusOK);
        resp->String("hello " + name); });

    server.GET("/user/{name}/match*", [](const HttpReq *req, HttpResp *resp)
               {
        cout << "full_path: " << req->full_path() << endl;
        cout << "current_path: " << req->current_path() << endl;
        cout << "match_path: " << req->match_path() << endl;
    
        const string &full_path = req->full_path();
        string current_path = req->current_path();

        string result;
        if(full_path == "/user/{name}/match*"){
            result = full_path + "匹配: " + current_path;
        } else {
            result = full_path + "不匹配: " + current_path;
        }
        resp->String(result); });

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

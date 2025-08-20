#include <workflow/MySQLResult.h>
#include <wfrest/HttpServer.h>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;
using namespace wfrest;
using namespace protocol;

int main()
{
    HttpServer server;

    // 设置路由
    // 提示：REST API返回JSON数据
    server.GET("/select", [](const HttpReq *req, HttpResp *resp)
               {
        // 原理：创建MySQL任务，push_back到SeriesWork中，再返回结果
        resp->MySQL("mysql://root:ZYJ.01234@localhost", "SHOW DATABASES"); });

    server.GET("/multiselect", [](const HttpReq *req, HttpResp *resp)
               {
        string url = "mysql://root:ZYJ.01234@localhost";
        string sql = {"SHOW DATABASES"};

        resp->MySQL(url,sql,[resp](MySQLResultCursor* cursor){
            string result;
            vector<MySQLCell> record;
            while(cursor->fetch_row(record)){
                result.append(record[0].as_string());
                result.append("\n");
            }
            resp->String(std::move(result));
        }); });

    server.GET("/multisql", [](const HttpReq *req, HttpResp *resp)
               {
        string url { "mysql://root:ZYJ.01234@localhost/http" };
        string sql { "SHOW DATABASES; SELECT * FROM tbl_user" };
        resp->MySQL(url, sql); });
    if (server.start(8888) == 0)
    {
        getchar(); /* 按任意键终止程序 */
        server.stop();
    }
    else
    {
        cerr << "Error: cannot start server!\n";
        exit(1);
    }
    return 0;
}

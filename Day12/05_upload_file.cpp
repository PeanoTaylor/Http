#include <wfrest/HttpServer.h>
#include <wfrest/PathUtil.h>
#include <iostream>

using namespace std;
using namespace wfrest;

int main()
{
    HttpServer server;

    server.POST("/file_upload", [](const HttpReq *req, HttpResp *resp)
                {
        string &body = req->body();
        resp->Save("file1.txt",std::move(body),"upload success\n"); });

    // 去除边界，并获取上传文件的名字
    // // curl -v -X POST "ip:port/upload" -F "file=@demo.txt; filename=../demo.txt" -H "Content-Type: multipart/form-data"
    server.POST("/file_write", [](const HttpReq *req, HttpResp *resp)
                {
        // 1. 校验请求体的类型：multipart/form-data
        if (req->content_type() != MULTIPART_FORM_DATA) {
            resp->set_status(HttpStatusBadRequest);     // 400: BadRequest
            return ;
        }
        // using map<string, pair<string, string>>
        //           key          filename, content
        const Form& form = req->form();

        for (const auto& [_, fileInfo]: form) {
            const auto& [filename, content] = fileInfo;
#ifdef DEBUG
            std::cout << filename << ": " << content << "\n";
#endif
            // filename 不应该被信任，不应该被应用程序盲目使用。
            // 参见 MDN 上的 Content-Disposition
            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition#directives
            // 应去除路径信息，并应用服务器文件系统规则

            // resp->Save(filename, std::move(content));
            resp->Save(PathUtil::base(filename), std::move(content));
        }
        resp->String("Save files success!\n"); });
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

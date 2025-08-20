#include <iostream>
#include <wfrest/HttpServer.h>
#include <map>
using namespace wfrest;
using namespace std;

int main()
{

    HttpServer server;
    // URL编码表单
    // curl -v http://ip:port/post \
    // -H "Content-Type: application/x-www-form-urlencoded" \
    // -d 'user=admin&password=123456'
    server.POST("/post", [](const HttpReq *req, HttpResp *resp)
                {

        if(req->content_type() != APPLICATION_URLENCODED){
            resp->set_status(HttpStatusBadRequest);// 响应状态码400
            return;
        }

        map<string,string> &formData = req->form_kv();

        for(const auto&[key,value]:formData){
            cout << key<<":"<<value << endl;
        } });

    server.POST("/form", [](const HttpReq *req, HttpResp *resp)
                {

        if(req->content_type() != MULTIPART_FORM_DATA){
            resp->set_status(HttpStatusBadRequest);// 响应状态码400
            return;
        }

        const Form& form = req->form();


        for(const auto&[_,fileInfo]:form){
            {
                const auto&[filename,body] = fileInfo;
                cout << filename<<":"<<body<<endl;
            }
        } });

    server.POST("/form_send", [](const HttpReq *req, HttpResp *resp)
                {

        MultiPartEncoder encoder;
        encoder.add_param("zhangsan","lisi");
        encoder.add_file("file1","./file/info.txt");
        //encoder.add_file("file2","./file/b.txt");
        resp->String(std::move(encoder));
    });

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

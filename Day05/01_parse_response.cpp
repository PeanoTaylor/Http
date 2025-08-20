#include <iostream>
#include "workflow/WFTaskFactory.h"
#include "workflow/HttpUtil.h"

using namespace protocol;
using namespace std;

// 回调函数
void http_callback(WFHttpTask* task)
{
    int state = task->get_state();
    if (state != WFT_STATE_SUCCESS)
    {
        cerr << "HTTP 请求失败，state: " << state << endl;
        return;
    }

    // 1. 响应行
    HttpResponse* resp = task->get_resp();
    cout << "==== 响应行 ====" << endl;
    cout << resp->get_http_version() << " "
         << resp->get_status_code() << " "
         << resp->get_reason_phrase() << endl;

    // 2. 响应头
    cout << "\n==== 响应头 ====" << endl;
    HttpHeaderCursor cursor(resp);
    string name, value;
    while (cursor.next(name, value))
    {
        cout << name << ": " << value << endl;
    }

    // 3. 响应体
    cout << "\n==== 响应体 ====" << endl;
    const void* body;
    size_t size;
    resp->get_parsed_body(&body, &size);
    // 按 size 输出，防止乱码和截断
    cout.write(static_cast<const char*>(body), size);
    cout << endl;
}

int main()
{
    // 创建 HTTP GET 任务（必须带 http:// 前缀）
    WFHttpTask* task = WFTaskFactory::create_http_task(
        "http://www.baidu.com",   // 目标 URL
        3,                        // 最大重定向次数
        2,                        // 最大重试次数
        http_callback             // 回调
    );

    // 设置请求方法与请求头
    HttpRequest* req = task->get_req();
    req->set_method("GET");
    req->set_header_pair("Host", "www.baidu.com");

    // 启动异步任务
    task->start();

    getchar(); // 简单阻塞，防止主线程提前退出
    return 0;
}

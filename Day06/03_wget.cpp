#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
using namespace protocol;
using namespace std;

#define REDIRECT_MAX 5
#define RETRY_MAX 2

static WFFacilities::WaitGroup wait_group{1};

void wget_callback(WFHttpTask *task)
{
    HttpRequest *req = task->get_req();
    HttpResponse *resp = task->get_resp();
    int state = task->get_state();

    // 1.检查状态
    if (state != WFT_STATE_SUCCESS)
    {
        fprintf(stderr, "Failed, Press Ctrl-C to exit.\n");
        return;
    }

    if (!freopen("output.txt", "w", stdout))
    {
        perror("freopen");
        return;
    }
    // 2.解析响应
    cout << resp->get_http_version() << " " << resp->get_status_code() << " " << resp->get_reason_phrase() << "\r\n";

    string name, value;
    HttpHeaderCursor cursor{resp};
    while (cursor.next(name, value))
    {
        cout << name << ": " << value << "\r\n";
    }
    cout << "\r\n";

    const void *body;
    size_t body_len;
    resp->get_parsed_body(&body, &body_len);
    cout << (const char*)body << endl;
    cout << "Success. Press Ctrl-C to exit." << endl;
}

void sig_handler(int signum)
{
    wait_group.done();
}

int main(int argc, char *argv[])
{
    WFHttpTask *task;
    if (argc != 2)
    {
        fprintf(stderr, "USAGE: %s <http URL>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sig_handler);

    string url = "www.baidu.com";
    if (strncasecmp(argv[1], "http://", 7) != 0 && strncasecmp(argv[1], "https://", 8) != 0)
    {
        url = "http://" + url;
    }

    task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, RETRY_MAX,
                                           wget_callback);
    HttpRequest *req = task->get_req();
    req->add_header_pair("Accept", "*/*");
    req->add_header_pair("User-Agent", "Wget/1.14(linux-gnu)");
    req->add_header_pair("Connection", "close");
    task->start();

    wait_group.wait();

    return 0;
}

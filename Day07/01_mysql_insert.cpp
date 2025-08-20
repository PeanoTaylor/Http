#include <iostream>
#include <string>
#include <signal.h>

#include "workflow/WFTaskFactory.h"
#include "workflow/Workflow.h"
#include "workflow/WFFacilities.h"
#include "workflow/MySQLResult.h"
using namespace std;
using namespace protocol;
#define RETRY_MAX 3

void mysql_callback(WFMySQLTask *task)
{
    int state = task->get_state();
    if (state != WFT_STATE_SUCCESS)
    {
        cerr << WFGlobal::get_error_string(state, task->get_error()) << endl;
        return;
    }

    MySQLResponse *resp = task->get_resp();
    if (resp->get_packet_type() == MYSQL_PACKET_ERROR)
    {
        cerr << "MYSQL task Failed!\n";
        return;
    }

    // 处理结果集
    MySQLResultCursor cursor{resp};
    if (cursor.get_cursor_status() == MYSQL_STATUS_OK)
    {
        unsigned long long affectedRows = cursor.get_affected_rows();
        cout << affectedRows << " row affected\n";
        cout << "id:" << cursor.get_insert_id() << "\n";
    }
}
WFFacilities::WaitGroup wait_group{1};

void sig_handler(int signum)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);

    // 1.创建任务
    string url = "mysql://root:ZYJ.01234@127.0.0.1/http";

    WFMySQLTask *task = WFTaskFactory::create_mysql_task(url, RETRY_MAX, mysql_callback);

    // 2.设置请求
    string sql = "INSERT INTO tbl_user (username,password,salt) values ('zhangsan', 'qwert', 'random_salt');";
    MySQLRequest *req = task->get_req();
    req->set_query(sql);

    task->start();

    wait_group.wait();
    return 0;
}

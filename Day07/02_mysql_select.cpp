#include <iostream>
#include <string>
#include <signal.h>
#include <vector>
#include "workflow/WFTaskFactory.h"
#include "workflow/Workflow.h"
#include "workflow/WFFacilities.h"
#include "workflow/MySQLResult.h"
using namespace std;
using namespace protocol;
#define RETRY_MAX 3

void print_cell(const MySQLCell &cell)
{
    if (cell.is_null())
    {
        cout << "(NULL)";
    }
    else if (cell.is_int())
    {
        cout << cell.as_int();
    }
    else if (cell.is_ulonglong())
    {
        cout << cell.as_ulonglong();
    }
    else if (cell.is_float())
    {
        cout << cell.as_float();
    }
    else if (cell.is_double())
    {
        cout << cell.as_double();
    }
    else if (cell.is_string())
    {
        cout << cell.as_string();
    }
    else if (cell.is_date())
    {
        cout << cell.as_date();
    }
    else if (cell.is_time())
    {
        cout << cell.as_time();
    }
    else if (cell.is_datetime())
    {
        cout << cell.as_datetime();
    }
}

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
    if (cursor.get_cursor_status() == MYSQL_STATUS_GET_RESULT)
    {
        int rows = cursor.get_rows_count();
        int columns = cursor.get_field_count();
        cout << "rows: " << rows << ", columns: " << columns << endl;

        vector<MySQLCell> record;
        while (cursor.fetch_row(record))
        {
            for (const auto &cell : record)
                print_cell(cell);
            cout << "\t";
        }
        cout << endl;
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
    string sql = "SELECT * FROM tbl_user;";
    MySQLRequest *req = task->get_req();
    req->set_query(sql);

    task->start();

    wait_group.wait();
    return 0;
}

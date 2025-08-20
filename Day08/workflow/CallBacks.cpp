#include "CallBacks.hpp"
#include <unistd.h>
#include <workflow/HttpUtil.h>
#include <map>
using std::cerr;
using std::map;
// 验证token
bool checkAuth(HttpResponse *http_resp, HttpRequest *http_req, User &user)
{
    HttpHeaderCursor http_cursor{http_req};
    string key, value;
    string auth_header;

    // 1. 遍历请求头找 Authorization
    while (http_cursor.next(key, value))
    {
        if (strcasecmp(key.c_str(), "Authorization") == 0)
        {
            auth_header = value;
            break;
        }
    }

    // 2. 如果没有 Authorization 头，则尝试从 Cookie 获取
    if (auth_header.empty())
    {
        string cookie_header;
        HttpHeaderCursor cookie_cursor{http_req};
        while (cookie_cursor.next(key, value))
        {
            if (strcasecmp(key.c_str(), "Cookie") == 0)
            {
                cookie_header = value;
                break;
            }
        }
        if (!cookie_header.empty())
        {
            auto pos = cookie_header.find("token=");
            if (pos != string::npos)
            {
                pos += 6; // 跳过 token=
                auto end = cookie_header.find(';', pos);
                string token_val = (end == string::npos)
                                       ? cookie_header.substr(pos)
                                       : cookie_header.substr(pos, end - pos);
                // 拼成 Bearer 形式
                auth_header = "Bearer " + token_val;
            }
        }
    }

    // 3. 检查格式
    if (auth_header.empty() || auth_header.find("Bearer ") != 0)
    {
        http_resp->set_status_code("401");
        http_resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
        http_resp->append_output_body("缺少Token");
        return false;
    }

    // 4. 提取 token 并验证
    string token = auth_header.substr(7);
    if (!CryptoUtil::verifyToken(token, user))
    {
        http_resp->set_status_code("401");
        http_resp->append_output_body("Token 无效或已过期");
        return false;
    }
    return true;
}

void pread_callback(WFFileIOTask *task, HttpResponse *resp, string filename)
{
    FileIOArgs *args = task->get_args();
    close(args->fd);

    long bytes = task->get_retval();

    // 判断任务状态
    if (task->get_state() != WFT_STATE_SUCCESS || bytes < 0)
    {
        resp->set_status_code("500");
        resp->append_output_body_nocopy("<html>Server Internal Error!</html>");
        return;
    }

    // 设置Content-Disposition标头：需要文件名
    resp->add_header_pair("Content-Disposition", "attachment; filename=" + filename);
    resp->append_output_body_nocopy(args->buf, bytes);
}

void register_callback(WFMySQLTask *task, HttpResponse *http_resp)
{
    int state = task->get_state();
    if (state != WFT_STATE_SUCCESS)
    {
        http_resp->set_status_code("500");
        cerr << WFGlobal::get_error_string(state, task->get_error()) << endl;
        return;
    }

    MySQLResponse *resp = task->get_resp();
    if (resp->get_packet_type() == MYSQL_PACKET_ERROR)
    {
        http_resp->set_status_code("400");
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
    http_resp->append_output_body("注册成功");
}

void login_callback(WFMySQLTask *task, HttpResponse *http_resp, string username, string password)
{
    int state = task->get_state();
    if (state != WFT_STATE_SUCCESS)
    {
        http_resp->set_status_code("500");
        cerr << WFGlobal::get_error_string(state, task->get_error()) << endl;
        return;
    }

    MySQLResponse *resp = task->get_resp();
    if (resp->get_packet_type() == MYSQL_PACKET_ERROR)
    {
        http_resp->set_status_code("400");
        cerr << "MYSQL task Failed!\n";
        return;
    }

    // 处理结果集
    MySQLResultCursor cursor{resp};
    if (cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT)
    {
        http_resp->set_status_code("401");
        http_resp->append_output_body("用户名或密码错误!");
        return;
    }

    map<string, MySQLCell> row;
    if (!cursor.fetch_row(row))
    {
        http_resp->set_status_code("401");
        http_resp->append_output_body("用户不存在!");
        return;
    }

    int user_id = row["id"].as_int();
    string user_salt = row["salt"].as_string();
    string user_pwd = row["password"].as_string();

    string cal_hash = CryptoUtil::hashPassword(password, user_salt, EVP_sha256());
    if (cal_hash != user_pwd)
    {
        http_resp->set_status_code("401");
        http_resp->append_output_body("用户名或密码错误");
        return;
    }
    // 登录成功生成token
    User user;
    user.id = user_id;
    user.username = username;
    string token = CryptoUtil::generateToken(user, JWT_ALG_HS256);

    // 设置 Cookie
    http_resp->add_header_pair("Set-Cookie", "token=" + token + "; Path=/; HttpOnly");

    // 返回 JSON，让前端跳转
    http_resp->set_status_code("200");
    http_resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
    http_resp->append_output_body("{\"success\":true,\"message\":\"登录成功\"}");
}

#include "CryptoUtil.hpp"
#include "CallBacks.hpp"
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <workflow/WFFacilities.h>
#include <workflow/WFHttpServer.h>
#include <workflow/HttpMessage.h>
#include <nlohmann/json.hpp>
#include "workflow/MySQLResult.h"
#include <wfrest/HttpServer.h>
using namespace wfrest;
using namespace std;
using namespace protocol;
using json = nlohmann::json;
#define RETRY_MAX 3

static WFFacilities::WaitGroup g_wait_group(1);
static const string mysql_url = "mysql://root:ZYJ.01234@127.0.0.1/http";

// 根据请求构建响应
/* void process(WFHttpTask *httpTask)
{

    HttpRequest *http_req = httpTask->get_req();
    HttpResponse *http_resp = httpTask->get_resp();

    // 1.通过请求路径，找到对应的文件路径
    const char *uri = http_req->get_request_uri();
    if (string(uri) == "/user/signup" && strcmp(http_req->get_method(), "GET") == 0)
    {
        int fd = open("static/register.html", O_RDONLY);
        if (fd == -1)
        {
            http_resp->set_status_code("404");
            http_resp->append_output_body("Not Found");
        }
        else
        {
            http_resp->add_header_pair("Content-Type", "text/html; charset=utf-8");
            char buf[4096];
            ssize_t n;
            while ((n = read(fd, buf, sizeof(buf))) > 0)
            {
                http_resp->append_output_body(buf, n);
            }
            close(fd);
        }
    }
    else if (string(uri) == "/user/signup" && strcmp(http_req->get_method(), "POST") == 0)
    {
        const void *body;
        size_t body_len;
        http_req->get_parsed_body(&body, &body_len);
        string body_str((const char *)body, body_len);

        http_resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
        nlohmann::json info = nlohmann::json::parse(body_str);
        string username = info["username"];
        string password = info["password"];

        // 生成盐值，哈希加密
        string salt = CryptoUtil::generateSalt(16);
        string hash_salt_pwd = CryptoUtil::hashPassword(password, salt, EVP_sha256());

        string sql = "INSERT INTO tbl_user(username, password, salt) VALUES('" + username + "', '" + hash_salt_pwd + "', '" + salt + "');";
        WFMySQLTask *task = WFTaskFactory::create_mysql_task(mysql_url, RETRY_MAX, std::bind(register_callback, std::placeholders::_1, http_resp));
        // 执行语句
        MySQLRequest *req = task->get_req();
        req->set_query(sql);

        // 加入Http序列
        series_of(httpTask)->push_back(task);
    }
    else if (string(uri) == "/user/signin" && strcmp(http_req->get_method(), "GET") == 0)
    {
        int fd = open("static/login.html", O_RDONLY);
        if (fd == -1)
        {
            http_resp->set_status_code("404");
            http_resp->append_output_body("Not Found");
        }
        else
        {
            http_resp->add_header_pair("Content-Type", "text/html; charset=utf-8");
            char buf[4096];
            ssize_t n;
            while ((n = read(fd, buf, sizeof(buf))) > 0)
            {
                http_resp->append_output_body(buf, n);
            }
            close(fd);
        }
    }
    else if (string(uri) == "/user/signin" && strcmp(http_req->get_method(), "POST") == 0)
    {
        // 1.读取Http请求体
        const void *body;
        size_t body_len;
        http_req->get_parsed_body(&body, &body_len);
        string body_str((const char *)body, body_len);

        http_resp->add_header_pair("Content-Type", "application/json; charset=utf-8");
        nlohmann::json info = nlohmann::json::parse(body_str);
        string username = info["username"];
        string password = info["password"];

        // 2.读取数据库中数据做验证
        string sql = "SELECT id, salt, password FROM tbl_user WHERE username = '" + username + "' LIMIT 1;";

        WFMySQLTask *task = WFTaskFactory::create_mysql_task(mysql_url, RETRY_MAX, std::bind(login_callback, std::placeholders::_1, http_resp, username, password));
        // 执行语句
        MySQLRequest *req = task->get_req();
        req->set_query(sql);

        // 加入Http序列
        series_of(httpTask)->push_back(task);
        std::cout << "uri: " << uri
                  << ", method: " << http_req->get_method() << std::endl;
    }
    else if (strncmp(uri, "/resources", 10) == 0 && strcmp(http_req->get_method(), "GET") == 0)
    {
        User user;
        if (!checkAuth(http_resp, http_req, user))
        {
            return; // Token验证失败
        }
        cout << "token success" << endl;


        // 解析路径
        string request_path = http_req->get_request_uri();
        cout << request_path << endl;
        auto pos = request_path.find("?");
        if (pos != string::npos)
            request_path = request_path.substr(0, pos);

        // 拼接成本地路径 (./resources/...)
        string path = "." + request_path; // "/resources/info.txt" → "./resources/info.txt"
        cout << "本地路径: " << path << endl;

        // 如果是目录，默认取 index.html
        if (path.back() == '/' || path.back() == '\\')
        {
            path += "index.html";
        }

        // 找到最后一个/ 定位文件位置
        string filename = path.substr(path.find_last_of("/\\") + 1);
        cout << filename << endl;
        // 判断文件是否存在
        if (access(path.c_str(), R_OK) == -1)
        {
            http_resp->set_status_code("404");
            http_resp->append_output_body_nocopy("<html>File Not Found!</html>");
            return;
        }
        // 文件存在，并且有读权限
        int fd = open(path.c_str(), O_RDONLY);
        if (fd >= 0)
        {
            // 打开文件，获取文件的大小(lseek,fstat)
            lseek(fd, 0, SEEK_END);
            size_t size = lseek(fd, 0, SEEK_CUR);

            void *buf = malloc(size);
            assert(buf != nullptr && "malloc failed");
            // 值捕获，因为引用捕获，引用的是栈上的指针变量
            httpTask->set_callback([buf](WFHttpTask *task)
                                   { free(buf); });

            WFFileIOTask *preadTask = WFTaskFactory::create_pread_task(fd, buf, size, 0, std::bind(pread_callback, std::placeholders::_1, http_resp, filename));
            series_of(httpTask)->push_back(preadTask);
        }
        else
        {
            // 打开失败
            http_resp->set_status_code("500");
            http_resp->append_output_body_nocopy("<html>Server Internal Error!</html>");
            return;
        }
    }
    else
    {
        std::cout << "uri: " << uri
                  << ", method: " << http_req->get_method() << std::endl;
        http_resp->set_status_code("404");
        http_resp->append_output_body_nocopy("<html>404 Not Found.</html>");
    }
} */

void register_get_routes(HttpServer &server)
{
    server.GET("/user/signup", [](const HttpReq *req, HttpResp *resp)
               { resp->File("../static/register.html"); });
}

void register_post_routes(HttpServer &server, const string &mysql_url)
{
    server.POST("/user/signup", [mysql_url](const HttpReq * req, HttpResp * resp, SeriesWork * series) {
    auto info = req -> json();
    string username = info["username"];
    string password = info["password"];

    string salt = CryptoUtil::generateSalt(16);
    string hash_salt_pwd = CryptoUtil::hashPassword(password, salt, EVP_sha256());

    string sql = "INSERT INTO tbl_user(username,password,salt) VALUES ('" + username + "','" + hash_salt_pwd + "','" + salt + "');";

    // 创建MYSQL任务
    WFMySQLTask * mysqlTask = WFTaskFactory::create_mysql_task(mysql_url, RETRY_MAX, [resp](WFMySQLTask * task) {
        if (task -> get_state() != WFT_STATE_SUCCESS || task -> get_resp() -> get_packet_type() == MYSQL_PACKET_ERROR) {
            resp -> set_status(HttpStatusInternalServerError);
            resp -> String("数据库错误");
            return;
        }
        resp -> String("注册成功");
    });
    mysqlTask -> get_req() -> set_query(sql);
    series -> push_back(mysqlTask); // 加入串行流
});
}

void login_get_routes(HttpServer &server)
{
    server.GET("/user/signin", [](const HttpReq *req, HttpResp *resp)
               { resp->File("../static/login.html"); });
}

void login_post_routes(HttpServer & server,const string & mysql_url) {
    server.POST("/user/signin", [mysql_url](const HttpReq * req, HttpResp * resp, SeriesWork * series) {
        auto info = req -> json();
        string username = info["username"];
        string password = info["password"];

        string sql = "SELECT id,salt,password FROM tbl_user WHERE username = '" + username + "'LIMIT 1;";

        // 创建MYSQL任务
        WFMySQLTask * mysqlTask = WFTaskFactory::create_mysql_task(mysql_url, RETRY_MAX, [resp, username, password](WFMySQLTask * task) {
            if (task -> get_state() != WFT_STATE_SUCCESS || task -> get_resp() -> get_packet_type() == MYSQL_PACKET_ERROR) {
                resp -> set_status(HttpStatusInternalServerError);
                resp -> String("数据库错误");
                return;
            }

            // 处理结果集
            MySQLResponse * mysql_resp = task -> get_resp();
            MySQLResultCursor cursor { mysql_resp };
            if (cursor.get_cursor_status() != MYSQL_STATUS_GET_RESULT) {
                resp -> set_status_code("401");
                resp -> append_output_body("用户名或密码错误!");
                return;
            }

            map < string, MySQLCell > row;
            if (!cursor.fetch_row(row)) {
                resp -> set_status_code("401");
                resp -> append_output_body("用户不存在!");
                return;
            }

            int user_id = row["id"].as_int();
            string user_salt = row["salt"].as_string();
            string user_pwd = row["password"].as_string();

            string cal_hash = CryptoUtil::hashPassword(password, user_salt, EVP_sha256());
            if (cal_hash != user_pwd) {
                resp -> set_status_code("401");
                resp -> append_output_body("用户名或密码错误");
                return;
            }
            resp -> String("登录成功");
        });
        mysqlTask -> get_req() -> set_query(sql);
        series -> push_back(mysqlTask); // 加入串行流
    });
}

void download_get_routes(HttpServer &server){
    server.GET("/resources/*", [](const HttpReq *req, HttpResp *resp, SeriesWork *series){
        const string static_dir = "/home/zhang/Http/Day08";

        string request_path = req->full_path();
        cout << request_path << endl;
        // 去掉 ? 参数
        auto pos = request_path.find('?');
        if (pos != string::npos)
            request_path = request_path.substr(0, pos);

        // 拼成本地路径
        string path = static_dir + request_path;
        cout << "本地路径: " << path << endl;

        // 如果是目录，默认取 index.html
        if (path.back() == '/' || path.back() == '\\')
        {
            path += "index.html";
        }

        // 判断文件是否存在
        if (access(path.c_str(), R_OK) == -1)
        {
            resp->set_status(HttpStatusNotFound);
            resp->String("<html>File Not Found!</html>");
            return;
        }

        // 直接用 wfrest 提供的文件发送接口
        resp->File(path);
    });
}

int main()
{

    HttpServer server;
    register_get_routes(server);
    register_post_routes(server, mysql_url);

    login_get_routes(server);
    login_post_routes(server, mysql_url);

    download_get_routes(server);

    if (server.start(8080) == 0)
    {
        g_wait_group.wait();
        // 服务有序退出
        server.stop();
    }
    else
    {
        // 启动失败
        std::cerr << "Server start FAILED!\n";
        std::exit(1);
    }

    return 0;
}

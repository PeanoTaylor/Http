#include "clouddisk.srpc.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/MySQLResult.h"
#include "CryptoUtil.hpp"
#include <iostream>
#include <map>
#include <signal.h>
#include <chrono>
#include "ppconsul/agent.h"

using ppconsul::Consul;
using namespace ppconsul::agent;

using namespace srpc;
using namespace std;
using namespace protocol;
static const string mysql_url = "mysql://root:ZYJ.01234@127.0.0.1/http";

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

class UserServiceServiceImpl : public ::clouddisk::UserService::Service
{
public:

	void SignUp(::clouddisk::SignUpRequest *req, ::clouddisk::SignUpResponse *resp, srpc::RPCContext *ctx) override
	{
		// TODO: fill server logic here
		// 1. 校验请求参数，解析请求参数

        string username = req->username();
        string password = req->password();

        if (username.empty() || password.empty())
        {
            resp->set_status("FAIL");
            resp->set_msg("missing username or password");
            return;
        }

        // 2.生成盐值
        string salt = CryptoUtil::generateSalt();
        string hash = password + salt;

        string store_password = CryptoUtil::hashPassword(password, salt, EVP_sha256());

        string sql_signup = "INSERT INTO tbl_user(username, password, salt) VALUES ('" + username + "', '" + store_password + "', '" + salt + "')";

        WFMySQLTask *mysqlTask = WFTaskFactory::create_mysql_task(mysql_url,1,[resp,ctx](WFMySQLTask *task){
			int state = task->get_state();
			if(state != WFT_STATE_SUCCESS){
				resp->set_status("FAIL");
                resp->set_msg("mysql task error");
                return;
			}
			 // 用 MySQLResultCursor 包装 resp
			MySQLResultCursor cursor(task->get_resp());

            if (cursor.get_cursor_status() == MYSQL_STATUS_ERROR)
            {
                resp->set_status("FAIL");
                resp->set_msg("insert failed (maybe username exists)");
                return;
            }

            resp->set_status("SUCCESS");
            resp->set_msg("signup ok");
		});
		mysqlTask->get_req()->set_query(sql_signup);
		ctx->get_series()->push_back(mysqlTask);
	}

	void SignIn(::clouddisk::SignUpRequest *req, ::clouddisk::SignUpResponse *resp, srpc::RPCContext *ctx) override
	{
		// TODO: fill server logic here
		string username = req->username();
		string password = req->password();

		string sql_signin = "SELECT id,password,salt FROM tbl_user WHERE username = '" + username + "' LIMIT 1;";

		WFMySQLTask *mysqlTask = WFTaskFactory::create_mysql_task(mysql_url,1,[resp,username,password](WFMySQLTask *task){
			int state = task->get_state();
			if(state != WFT_STATE_SUCCESS){
				resp->set_status("FAIL");
				resp->set_msg("mysql task error");
				return;
			}
			// 用 MySQLResultCursor 包装 resp
			MySQLResultCursor cursor(task->get_resp());
			if (cursor.get_cursor_status() == MYSQL_STATUS_ERROR)
            {
                resp->set_status("FAIL");
                resp->set_msg("insert failed (maybe username exists)");
                return;
            }

			std::map<string,MySQLCell> record;
			if(!cursor.fetch_row(record)){
				resp->set_status("FAIL");
				resp->set_msg("user not found");
				return;
			}

			string password_hash = record["password"].as_string();
			string salt = record["salt"].as_string();

			string cal_hash = CryptoUtil::hashPassword(password,salt,EVP_sha256());
			if(cal_hash != password_hash){
				resp->set_status("FAIL");
				resp->set_msg("password incorrect");
				return;
			}

			User user;
            user.id = record["id"].as_int();
            user.username = username;
            user.password = password_hash;
            user.salt = salt;

            string token = CryptoUtil::generateToken(user, JWT_ALG_HS256);

            resp->set_status("SUCCESS");
            resp->set_msg("signin ok");
            resp->set_token(token); 

		});

		mysqlTask->get_req()->set_query(sql_signin);
		ctx->get_series()->push_back(mysqlTask);

		
	}
};


void timer_callback(WFTimerTask *task, Agent &agent)
{
    agent.servicePass("UserService1"); // 发送心跳包
    WFTimerTask *timerTask = WFTaskFactory::create_timer_task(5, 0, std::bind(timer_callback, std::placeholders::_1, std::ref(agent)));
    series_of(task)->push_back(timerTask);
}

int main()
{
	signal(SIGINT,sig_handler);
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1412;
	SRPCServer server;

	UserServiceServiceImpl userservice_impl;
	server.add_service(&userservice_impl);

	server.start(port);

	// 指定注册中心
	Consul consul{"http://127.0.0.1:8500"};
        // 创建代理
        Agent agent{consul};

        // 注册服务
        agent.registerService(
            kw::id = "UserService1",
            kw::name = "UserService",
            kw::address = "127.0.0.1",
            kw::port = port,
            kw::check = TtlCheck(std::chrono::seconds{10}));

        // 定时发送心跳包
        WFTimerTask *timerTask = WFTaskFactory::create_timer_task(5, 0, std::bind(timer_callback, std::placeholders::_1, std::ref(agent)));

        SeriesWork *series = Workflow::create_series_work(timerTask, nullptr);
        series->set_context(&agent);
        series->start();

	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}

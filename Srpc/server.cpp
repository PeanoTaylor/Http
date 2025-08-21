#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include "example.srpc.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFTaskFactory.h"
using std::cout;
using std::endl;
using namespace std;
using namespace srpc;
using namespace std::literals;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

class ExampleServiceImpl : public ::srpc::Example::Service
{
public:

	void Echo(::srpc::EchoRequest *request, ::srpc::EchoResponse *response, srpc::RPCContext *ctx) override
	{
		// TODO: fill server logic here
		// 1.解析请求
		cout << "EchoRequest{ message=" <<  request->message()
		     << ", name=" << request->name() << " }" << endl;

		// 2. 处理业务逻辑
		string url = "http://www.sogou.com";
		WFHttpTask *httpTask = WFTaskFactory::create_http_task(url,0,0,[response](WFHttpTask *task){
			int state = task->get_state();
			if(state != WFT_STATE_SUCCESS){
				response->set_message(WFGlobal::get_error_string(state,task->get_state()));
				return;
			}
			// 获取响应体
			const void* body;
			size_t body_len;
			task->get_resp()->get_parsed_body(&body,&body_len);
			response->set_message((const char*)body);
		});
		ctx->get_series()->push_back(httpTask);
	}
};

int main()
{
	signal(SIGINT,sig_handler);
	// 1.创建SRPC服务器
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1412;
	SRPCServer server;

	// 2.注册服务
	ExampleServiceImpl example_impl;
	server.add_service(&example_impl);

	// 3.启动服务器
	server.start(port);
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}

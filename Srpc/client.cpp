#include "example.srpc.h"
#include "workflow/WFFacilities.h"
#include <iostream>
#include <signal.h>
using std::cout;
using std::endl;
using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

// 回调函数：收到RPC的响应后执行
static void echo_done(::srpc::EchoResponse *response, srpc::RPCContext *context)
{
	if (!context->success())
	{
		cout << "error code = :" << context->get_error() << "errmsg = " << context->get_errmsg() << endl;
		return;
	}
	cout << response->message() << endl;
}

int main()
{
	signal(SIGINT, sig_handler);
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	const char *ip = "127.0.0.1";
	unsigned short port = 1412;

	::srpc::Example::SRPCClient client(ip, port);

	// 和 workflow 集成
	srpc::SRPCClientTask* task = client.create_Echo_task(echo_done);
	// example for RPC method call
	::srpc::EchoRequest echo_req;
	echo_req.set_message("Hello, srpc!");
	echo_req.set_name("Zhang");
	client.Echo(&echo_req, echo_done); // 异步非阻塞

	task->serialize_input(&echo_req);
	// 2. 启动任务
	task->start();

	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}

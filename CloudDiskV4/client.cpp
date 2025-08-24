#include "clouddisk.srpc.h"
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

static void signup_done(::clouddisk::SignUpResponse *response, srpc::RPCContext *context)
{
	if (!context->success())
	{
		cout << "error code = :" << context->get_error() << "errmsg = " << context->get_errmsg() << endl;
		return;
	}
	else
	{
		cout << "[SignUp] status=" << response->status()
			 << ", msg=" << response->msg() << endl;
	}
}

static void signin_done(::clouddisk::SignUpResponse *response, srpc::RPCContext *context)
{
	if (!context->success())
	{
		cout << "error code = :" << context->get_error() << "errmsg = " << context->get_errmsg() << endl;
		return;
	}
	else
	{
		cout << "[SignIn] status=" << response->status()
			 << ", msg=" << response->msg() << endl;
	}
}

int main()
{
	signal(SIGINT, sig_handler);
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	const char *ip = "127.0.0.1";
	unsigned short port = 1412;

	::clouddisk::UserService::SRPCClient client(ip, port);

	// example for RPC method call
	::clouddisk::SignUpRequest signup_req;
	signup_req.set_username("alice");
	signup_req.set_password("123456");
	// signup_req.set_message("Hello, srpc!");
	client.SignUp(&signup_req, signup_done);

	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}

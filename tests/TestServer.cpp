#include "TestCommon.hpp"

using namespace ZstTest;

void test_external_server()
{
}

void test_server_object()
{
	auto stage = zst_create_server("test_server_obj");
	zst_init("test_server_client", true);
	zst_join("127.0.0.1");
	Assert(zst_is_connected());
	zst_leave();
	stage->destroy();
}

int main(int argc,char **argv)
{
	TestRunner runner("TestServer", argv[0], false, false);
	test_external_server();
	test_server_object();
}

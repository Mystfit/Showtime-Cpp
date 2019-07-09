#include "TestCommon.hpp"

using namespace ZstTest;

void test_plugins(){

}

int main(int argc,char **argv)
{
    FixtureInit runner("TestPlugins", argv[0]);
    test_plugins();
    return 0;
}

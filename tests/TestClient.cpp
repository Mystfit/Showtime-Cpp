#include <chrono>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include <sstream>
#include <exception>

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  
#include <crtdbg.h>  
#pragma warning(push) 
#pragma warning(disable:4189 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic pop
#endif


#ifdef WIN32
#pragma warning(pop)
#endif


int main(int argc,char **argv){

    bool testing = true;
    if (argc > 1) {
        if (argv[1][0] == 't') {
            ZstLog::app(LogLevel::warn, "In test mode. Launching internal stage server.");
            testing = true;
        }
    }
    
    

    
    //Tests
    test_leaving();
    test_cleanup();

    if (testing) {
        
    }

#ifdef WIN32
    //Dump memory leaks to console
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}

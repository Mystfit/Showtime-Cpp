#include <string>
#include <iostream>
#include "ZstSection.h"
#include "ZstStage.h"

using namespace Showtime;

int main(int argc,char **argv){
	
    cout << "Starting stage runner" << endl;

	ZstStage *stage = ZstStage::create_stage();

	zsock_t *req = zsock_new_req(">tcp://127.0.0.1:6000");
    zsock_set_identity(req, "showtime_client");
    cout << "Set req identity to " << zsock_identity(req) << endl;
    cout << "Starting greeting loop" << endl;
    
	while (true) {
        sleep(1);
        cout << "Sending greeting" << endl;
        string greeting = "ping";

        zmsg_t *msg = zmsg_new();
        zmsg_addstr(msg, "ping");
        
        zmsg_send(&msg, req);
        zmsg_t *responseMsg = zmsg_recv(req);
        
        cout << "Client received message of " << zmsg_size(responseMsg) << " frames" << endl;

        zmsg_print(responseMsg);
        
        cout << "Client received: " << zmsg_popstr(responseMsg) << endl;
	}

}

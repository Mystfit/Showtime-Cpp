#include <string>
#include <iostream>
#include "ZstSection.h"
#include "ZstStage.h"

using namespace Showtime;

int main(int argc,char **argv){
	
	ZstStage *stage = ZstStage::create_stage();

	zsock_t *req = zsock_new_req(">tcp://127.0.0.1:6000");

	stage->start();


	while (true) {
		//cout << "Sending hello" << endl;
		zstr_send(req, "Hello world!");
	}

}
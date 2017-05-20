%module ZstStage
%{
	#include "ZstExports.h"
	#include "ZstStage.h"
%}

%include <windows.i>
%include "ZstExports.h"
%include "ZstStage.h"

/*
%nodefaultctor ZstStage;
class ZstStage{
    //static ZstStage create_stage();
};

//We extend the python __new__ function to let us call the static factory instead
%extend ZstStage {
    static ZstStage * __new__() { return ZstStage::create_stage(); }
};
*/

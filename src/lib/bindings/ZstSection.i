%module ZstSection
%{
	#include "ZstExports.h"
	#include "ZstSection.h"
%}

%include <windows.i>
%include "ZstExports.h"
%include "ZstSection.h"

/*
%nodefaultctor ZstSection;
class ZstSection{
    void register_to_stage();
};

//We extend the python __new__ function to let us call the static factory instead
%extend ZstSection {
    static ZstSection * __new__(std::string name) { return ZstSection::create_section(name); }
};
*/
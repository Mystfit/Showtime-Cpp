%ignore ZstPlugIterator;
%ignore ZstPlug::begin;
%ignore ZstPlug::end;

%nodefaultctor;
%include "entities/ZstPlug.h"
%clearnodefaultctor;

%include "std_string.i"
%extend ZstPlug{
    std::string char_at(size_t position) {
        char * buf = new char[$self->size_at(position) + 1]();
        $self->char_at(buf,position);
        std::string s = std::string(buf);
        delete[] buf;
        return s;
    }
};

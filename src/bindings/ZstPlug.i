%ignore ZstPlugIterator;
%ignore ZstPlugIterator::operator++;
%ignore ZstPlug::begin;
%ignore ZstPlug::end;

%inline %{
	ZstPlug* cast_to_plug(ZstEntityBase * entity){
		return dynamic_cast<ZstPlug*>(entity);
	}
%}

%nodefaultctor ZstPlug;
%include "entities/ZstPlug.h"

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

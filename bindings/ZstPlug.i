%ignore ZstPlugIterator;
%ignore ZstPlugIterator::operator++;
%ignore ZstPlug::begin;
%ignore ZstPlug::end;
%ignore ZstPlug::raw_value;

%inline %{
	ZstOutputPlug* cast_to_output_plug(ZstSynchronisable * synchronisable){
		return dynamic_cast<ZstOutputPlug*>(synchronisable);
	}

    ZstInputPlug* cast_to_input_plug(ZstSynchronisable * synchronisable){
        return dynamic_cast<ZstInputPlug*>(synchronisable);
    }
%}

%nodefaultctor ZstPlug;
%include <entities/ZstPlug.h>

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

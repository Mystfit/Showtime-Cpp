namespace showtime {
    %ignore ZstPlugIterator;
    %ignore ZstPlugIterator::operator++;
    %ignore ZstPlug::begin;
    %ignore ZstPlug::end;
    %ignore ZstPlug::raw_value;
    %nodefaultctor ZstPlug;

    enum ValueList {
        ValueList_NONE = 0,
        ValueList_IntList = 1,
        ValueList_FloatList = 2,
        ValueList_StrList = 3,
        ValueList_MIN = ValueList_NONE,
        ValueList_MAX = ValueList_StrList
    };
}

%inline %{
    showtime::ZstOutputPlug* cast_to_output_plug(showtime::ZstSynchronisable * synchronisable){
        return dynamic_cast<showtime::ZstOutputPlug*>(synchronisable);
    }

    showtime::ZstInputPlug* cast_to_input_plug(showtime::ZstSynchronisable * synchronisable){
        return dynamic_cast<showtime::ZstInputPlug*>(synchronisable);
    }
%}

%include <entities/ZstPlug.h>

%include "std_string.i"
%extend showtime::ZstPlug{
    std::string char_at(size_t position) {
        char * buf = new char[$self->size_at(position) + 1]();
        $self->char_at(buf,position);
        std::string s = std::string(buf);
        delete[] buf;
        return s;
    }
};

namespace showtime {
    %ignore ZstPlugIterator;
    %ignore ZstPlugIterator::operator++;
    %ignore ZstPlug::begin;
    %ignore ZstPlug::end;
    %ignore ZstPlug::raw_value;
    //%nodefaultctor ZstPlug;

    class Plug;
    class PlugData;
    %ignore ZstPlug::ZstPlug(Plug);
    %ignore ZstOutputPlug::ZstOutputPlug(Plug);
    %ignore ZstInputPlug::ZstInputPlug(Plug);
    %ignore ZstPlug::ZstPlug(Plug);
    %ignore ZstPlug::serialize;
    %ignore ZstPlug::deserialize;
    %ignore ZstPlug::serialize_partial;
    %ignore ZstPlug::deserialize_partial;
    //%ignore PlugValueData;
}

%inline %{
    showtime::ZstOutputPlug* cast_to_output_plug(showtime::ZstSynchronisable * synchronisable){
        return dynamic_cast<showtime::ZstOutputPlug*>(synchronisable);
    }

    showtime::ZstInputPlug* cast_to_input_plug(showtime::ZstSynchronisable * synchronisable){
        return dynamic_cast<showtime::ZstInputPlug*>(synchronisable);
    }

    showtime::ZstPlug* cast_to_plug(showtime::ZstSynchronisable * synchronisable){
        return dynamic_cast<showtime::ZstPlug*>(synchronisable);
    }
%}

%include <showtime/entities/ZstPlug.h>

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

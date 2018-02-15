%module(directors="1") showtime
%{
	#include <Showtime.h>
%}

%include <std_string.i>

%typemap(out) ZstComponent * ZstEntityBase::as_component {
    const std::string lookup_typename = "ZstComponent*";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), outtype, $owner);
}

%typemap(out) ZstContainer * ZstEntityBase::as_container {
    const std::string lookup_typename = "ZstContainer*"
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), outtype, $owner);
}

%include "showtime.i"
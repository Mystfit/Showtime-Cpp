%module(directors="1") showtime
%{
	#include <Showtime.h>
	#include <ShowtimeServer.h>
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

%include "../preflight.i"
%include "ZstBundle.i"
%include "../showtime.i"

%pythoncode %{
	def children(self):
		_children = []
		for i in range(self.num_children()):
			ent = self.get_child_at(i)
			if ent.entity_type() == COMPONENT_TYPE: 
				_children.append(cast_to_component(ent))
			elif ent.entity_type() == PERFORMER_TYPE: 
				_children.append(cast_to_performer(ent))
			elif ent.entity_type() == PLUG_TYPE: 
				_children.append(cast_to_plug(ent))
		return _children

	ZstContainer.children = children
%}

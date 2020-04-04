%typemap(out) showtime::ZstComponent * showtime::ZstEntityBase::as_component {
    const std::string lookup_typename = "ZstComponent*";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), outtype, $owner);
}

%pythoncode %{
def entity_bundle_to_list(bundle):
	list = []
	for entity in bundle.items():
		if entity.entity_type() == ZstEntityType_COMPONENT: 
			list.append(cast_to_component(entity))
		elif entity.entity_type() == ZstEntityType_PERFORMER: 
			list.append(cast_to_performer(entity))
		elif entity.entity_type() == ZstEntityType_PLUG: 
			plug = cast_to_input_plug(entity)
			if not plug:
				plug = cast_to_output_plug(entity)
			list.append(plug)
		elif entity.entity_type() == ZstEntityType_FACTORY: 
			list.append(cast_to_factory(entity))
	return list

def bundle_to_list(bundle):
	list = []
	for item in bundle.items():
		list.append(item)
	return list
%}

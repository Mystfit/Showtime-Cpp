%feature("director") ZstContainer;

%inline %{
	ZstContainer* cast_to_container(ZstEntityBase * entity){
		return dynamic_cast<ZstContainer*>(entity);
	}
%}

%nodefaultctor ZstContainer;
%include <entities/ZstContainer.h>

%pythoncode %{
	def children(self):
		_children = []
		for i in range(self.num_children()):
			ent = self.get_child_at(i)
			if ent.entity_type() == CONTAINER_TYPE:
				_children.append(cast_to_container(ent))
			elif ent.entity_type() == COMPONENT_TYPE: 
				_children.append(cast_to_component(ent))
			elif ent.entity_type() == PERFORMER_TYPE: 
				_children.append(cast_to_performer(ent))
			elif ent.entity_type() == PLUG_TYPE: 
				_children.append(cast_to_plug(ent))
		return _children

	ZstContainer.children = children
%}
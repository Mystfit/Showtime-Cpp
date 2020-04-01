namespace showtime {
	%extend ZstComponent {
		%insert("python") %{
			def get_plugs(self):
				bundle = ZstEntityBundle()
				return entity_bundle_to_list(self.get_plugs(bundle, include_parent))
		%}
	}

	%feature("nodirector") ZstComponent::get_child_entities;
	%ignore ZstComponent::get_child_entities;
	//%ignore ZstComponent::get_child_bundle;

	%feature("nodirector") ZstComponent::get_child_cables;
	%ignore ZstComponent::get_child_cables;
	//%ignore ZstComponent::get_cable_bundle;
}
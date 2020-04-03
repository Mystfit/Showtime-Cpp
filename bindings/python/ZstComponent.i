namespace showtime {
	%extend ZstComponent {
		%insert("python") %{
			def get_plugs(self):
				bundle = ZstEntityBundle()
				self.get_plugs(bundle, include_parent)
				return entity_bundle_to_list(bundle)
		%}
	}

	%feature("nodirector") ZstComponent::get_child_cables;
	%ignore ZstComponent::get_child_cables;
}
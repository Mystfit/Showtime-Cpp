namespace showtime {
	%rename(get_plug_bundle) ZstComponent::get_plugs;

	%extend ZstComponent {
		%insert("python") %{
			def get_plugs(self):
				bundle = ZstEntityBundle()
				self.get_plug_bundle(bundle)
				return entity_bundle_to_list(bundle)
		%}
	}

	%feature("nodirector") ZstComponent::get_child_cables;
	%ignore ZstComponent::get_child_cables;
}
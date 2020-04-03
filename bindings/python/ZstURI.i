namespace showtime {
	%extend ZstURI {
		%insert("python") %{
			def __str__(self):
				return str(self.path())
		%}
	}

	%feature("nodirector") ZstPerformer::get_child_entities;
	%feature("nodirector") ZstPerformer::get_child_cables;
}
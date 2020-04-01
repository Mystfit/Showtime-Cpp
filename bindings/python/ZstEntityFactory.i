namespace showtime {
	%extend ZstEntityFactory {
		%insert("python") %{
			def get_creatables(self):
				bundle = ZstURIBundle()
				return bundle_to_list(self.get_creatables(bundle))
		%}
	}
}
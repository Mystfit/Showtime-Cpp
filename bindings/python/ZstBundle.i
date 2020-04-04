namespace showtime {
	%extend ZstBundle {
		%insert("python") %{
			def items(self):
				for i in range(0, self.size()):
					yield self.item_at(i)
		%}
	}
}


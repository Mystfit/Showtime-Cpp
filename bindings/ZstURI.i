namespace showtime {
	%rename(add) ZstURI::operator+(const ZstURI &) const;
	%rename(less_than) ZstURI::operator<(const ZstURI &) const;
	%rename(equal_to) ZstURI::operator==(const ZstURI &) const;
	%rename(not_equal) ZstURI::operator!=(const ZstURI &) const;
	%rename(path_range) ZstURI::range(size_t start, size_t end) const;

	%rename(__lshift__) operator<<;

	%ignore ZstURI::ZstURI(ZstURI && source);
	%ignore ZstURI::operator=(const ZstURI & other);
	%ignore ZstURI::operator=(ZstURI && source);
	%ignore ZstURI::operator!=;
	%ignore ZstURIHash;
}

%include <showtime/ZstURI.h>

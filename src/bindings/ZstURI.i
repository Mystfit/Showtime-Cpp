%rename(add) ZstURI::operator+(const ZstURI &) const;
%rename(less_than) ZstURI::operator<(const ZstURI &) const;
%rename(equal_to) ZstURI::operator==(const ZstURI &) const;
%rename(not_equal) ZstURI::operator!=(const ZstURI &) const;

%ignore ZstURI::operator==;
%ignore ZstURI::operator=;
%ignore ZstURI::operator!=;
%ignore ZstURIHash;

%include <ZstURI.h>

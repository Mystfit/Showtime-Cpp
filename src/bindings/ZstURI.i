%rename(add) ZstURI::operator+(const ZstURI &) const;
%rename(less_than) ZstURI::operator<(const ZstURI &) const;
%rename(equal_to) ZstURI::operator==(const ZstURI &) const;
%rename(not_equal) ZstURI::operator!=(const ZstURI &) const;

%ignore ZstURI::ZstURI(ZstURI && source);
%ignore ZstURI::operator=(const ZstURI & other);
%ignore ZstURI::operator=(ZstURI && source);
%ignore ZstURI::operator!=;
%ignore ZstURIHash;

%include <ZstURI.h>

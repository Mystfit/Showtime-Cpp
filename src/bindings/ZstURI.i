%rename(add) ZstURI::operator+;
%rename(less_than) ZstURI::operator<;
%ignore ZstURI::operator==;
%ignore ZstURI::operator=;
%ignore ZstURI::operator!=;
%ignore ZstURIHash;

%include "std_string.i"
%include <ZstURI.h>

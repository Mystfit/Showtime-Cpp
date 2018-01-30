%rename(equal) ZstCable::operator==;
%ignore ZstSerialisable;
%ignore ZstCable::operator!=;
%ignore ZstCable::write;
%ignore ZstCable::read;
%ignore ZstCableEq;
%ignore ZstCableHash;

%include <ZstCable.h>

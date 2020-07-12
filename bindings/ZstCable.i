namespace showtime {
	%rename(equal) ZstCable::operator==;
	%ignore ZstCable::ZstCable(Cable);
	%ignore ZstSerialisable;
	%ignore ZstCable::operator!=;
	%ignore ZstCable::write;
	%ignore ZstCable::read;
	%ignore ZstCableEq;
	%ignore ZstCableHash;
	%ignore ZstCableCompare;
	//%ignore ZstCable::serialize;
	//%ignore ZstCable::deserialize;
	//%ignore ZstCable::serialize_partial;
	//%ignore ZstCable::deserialize_partial;
}

%include <showtime/ZstCable.h>

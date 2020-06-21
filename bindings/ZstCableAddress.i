namespace showtime {
	%ignore ZstCableAddress::ZstCableAddress(ZstCableAddress &&);
	%ignore ZstCableAddress::operator=;
	%ignore ZstCableAddress::operator==;
	%ignore ZstCableAddress::operator!=;
	%ignore ZstCableAddress::operator<;
	%ignore ZstCableAddressHash;
	%ignore ZstCableAddressEq;

	%ignore ZstCableAddress::ZstCableAddress(Cable);
	%ignore ZstCableAddress::serialize;
	%ignore ZstCableAddress::deserialize;
	%ignore ZstCableAddress::serialize_partial;
	%ignore ZstCableAddress::deserialize_partial;

}

%include <showtime/ZstCableAddress.h>

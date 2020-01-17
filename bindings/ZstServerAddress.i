namespace showtime {
	%ignore ZstServerAddress::operator=;
	%ignore ZstServerAddress::operator==;
	%ignore ZstServerAddress::operator<;
	%ignore ZstServerAddress::ZstServerAddress(ZstServerAddress &&);
}

%include <ZstServerAddress.h>

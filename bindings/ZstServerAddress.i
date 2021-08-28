namespace showtime {
	%copyctor ZstServerAddress;
	%ignore ZstServerAddress::operator=;
	%ignore ZstServerAddress::operator==;
	%ignore ZstServerAddress::operator<;
	%ignore ZstServerAddress::ZstServerAddress(ZstServerAddress &&);
}

%include <showtime/ZstServerAddress.h>

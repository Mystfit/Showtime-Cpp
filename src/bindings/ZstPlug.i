%module ZstPlug
%{
	#include "ZstExports.h"
	#include "ZstPlug.h"
%}

%include <windows.i>
%include "ZstExports.h"

%feature("director") PlugCallback;
class PlugCallback {
public:
	virtual void run(ZstPlug * plug);
}

struct PlugAddress {
	std::string performer;
	std::string instrument;
	std::string name;
	PlugDir direction;
	inline bool operator==(const PlugAddress& other) const;
    bool operator < (const PlugAddress& a) const;
	std::string to_s() const;
}


class ZstPlug {
public:
	%immutable;
	std::string get_name();
	std::string get_instrument();
	std::string get_performer();
	PlugDir get_direction();
	PlugAddress get_address();
    
    void attach_recv_callback(PlugCallback * callback);
    void destroy_recv_callback(PlugCallback * callback);
    
    void fire();
    static PlugAddress address_from_str(std::string s);
}
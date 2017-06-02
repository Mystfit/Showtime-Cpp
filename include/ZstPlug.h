#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <czmq.h>
#include <msgpack.hpp>
#include "ZstExports.h"
#include "ZstUtils.hpp"
#include "ZstURI.h"

class PlugCallback;
class ZstPlug {
public:
	//Constructor
	ZST_EXPORT ZstPlug(ZstURI uri);
	ZST_EXPORT ~ZstPlug();

	ZST_EXPORT ZstURI get_URI() const;
    
    //Plug callbacks
    ZST_EXPORT void attach_recv_callback(PlugCallback * callback);
    ZST_EXPORT void destroy_recv_callback(PlugCallback * callback);
    
	//IO
    ZST_EXPORT void fire();
	ZST_EXPORT virtual void recv(msgpack::object obj) = 0;

protected:
	msgpack::sbuffer * m_buffer;
	msgpack::packer<msgpack::sbuffer> * m_packer;
    
    virtual void run_recv_callbacks() = 0;
    std::vector<PlugCallback*> m_received_data_callbacks;
private:
	ZstURI m_uri;
};


enum PlugTypes {
	INT_PLUG = 0,
	INT_ARR_PLUG,
	FLOAT_PLUG,
	FLOAT_ARR_PLUG,
	STRING_PLUG
};


class PlugCallback{
public:
    virtual ~PlugCallback() { std::cout << "Callback::~Callback()" << std:: endl; }
    virtual void run(ZstPlug * plug) { std::cout << "PlugCallback::run()" << std::endl; }
};


// ----------------------
// Plug types
// ----------------------
class ZstIntPlug : public ZstPlug {
public:
	typedef int value_type;

	ZstIntPlug(ZstURI uri) : ZstPlug(uri) {};
	ZST_EXPORT void fire(int value);
	ZST_EXPORT void recv(msgpack::object object) override;
    ZST_EXPORT int get_value();
private:
    int m_last_value;
    int m_value;
};
 

template <typename BASE, typename T = typename BASE::value_type>
class ZstReactingPlug {

};
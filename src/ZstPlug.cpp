#include "ZstPlug.h"
#include "Showtime.h"
#include "ZstEndpoint.h"

#ifdef USEPYTHON
#include <python.h>
#endif

using namespace std;

ZstPlug::ZstPlug(ZstURI uri) : m_uri(uri)
{
	m_buffer = new msgpack::sbuffer();
	m_packer = new msgpack::packer<msgpack::sbuffer>(m_buffer);
}

ZstPlug::~ZstPlug() {
	delete m_buffer;
	delete m_packer;
}

ZstURI ZstPlug::get_URI() const
{
	return m_uri;
}

void ZstPlug::attach_recv_callback(PlugCallback *callback){
    m_received_data_callbacks.push_back(callback);
}

void ZstPlug::destroy_recv_callback(PlugCallback *callback){
    m_received_data_callbacks.erase(std::remove(m_received_data_callbacks.begin(), m_received_data_callbacks.end(), callback), m_received_data_callbacks.end());
    delete callback;
}

void ZstPlug::run_recv_callbacks(){
	cout << "PERFORMER: Running input plug callbacks" << endl;

    if(m_received_data_callbacks.size() > 0){
#ifdef USEPYTHON
		PyGILState_STATE gstate;
		if (Showtime::get_runtime_language() == RuntimeLanguage::PYTHON_RUNTIME) {
			cout << "PERFORMER: Host runtime is Python. Obtain GIL" << endl;
			gstate = PyGILState_Ensure();
		}
		else {
			cout << "PERFORMER: Host runtime is native." << endl;
		}
#endif
        for(vector<PlugCallback*>::iterator callback = m_received_data_callbacks.begin(); callback != m_received_data_callbacks.end(); ++callback){
            (*callback)->run(this);
        }
#ifdef USEPYTHON
		if (Showtime::get_runtime_language() == RuntimeLanguage::PYTHON_RUNTIME) {
			cout << "PERFORMER: Release GIL" << endl;
			PyGILState_Release(gstate);
		}
#endif
    }
}

void ZstPlug::fire()
{
	zmsg_t * msg = zmsg_new();

	//First frame is the address of the sender plug
	zmsg_addstr(msg, get_URI().to_char());

	//Second frame is the plug payload
	zframe_t * payload  = zframe_new(m_buffer->data(), m_buffer->size());
	zmsg_append(msg, &payload);

	Showtime::endpoint().send_to_graph(msg);

	//Clear our buffer when finished
	m_buffer->clear();
}

// -----

void ZstIntPlug::fire(int value)
{
	m_packer->pack_int(value);
	ZstPlug::fire();
}

void ZstIntPlug::recv(msgpack::object object){
    int out;
    object.convert<int>(out);
    m_value = out;
    run_recv_callbacks();
}

int ZstIntPlug::get_value(){
    return m_value;
}

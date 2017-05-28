#include "ZstPlug.h"
#include "Showtime.h"

using namespace std;

ZstPlug::ZstPlug(string name, string instrument, string performer, PlugDir direction)
{
	m_direction = direction;
	m_name = name;
	m_instrument = instrument;
	m_performer = performer;

	m_buffer = new msgpack::sbuffer();
	m_packer = new msgpack::packer<msgpack::sbuffer>(m_buffer);
}

ZstPlug::~ZstPlug() {
	delete m_buffer;
	delete m_packer;
}

string ZstPlug::get_name()
{
	return m_name;
}

std::string ZstPlug::get_instrument()
{
	return m_instrument;
}

string ZstPlug::get_performer()
{
    return m_performer;
}

PlugDir ZstPlug::get_direction()
{
	return m_direction;
}

PlugAddress ZstPlug::get_address()
{
	PlugAddress address;
	address.performer = m_performer;
	address.instrument = m_instrument;
	address.name = m_name;
	address.direction = m_direction;
	return address;
}

void ZstPlug::attach_recv_callback(PlugCallback *callback){
    m_received_data_callbacks.push_back(callback);
}

void ZstPlug::destroy_recv_callback(PlugCallback *callback){
    m_received_data_callbacks.erase(std::remove(m_received_data_callbacks.begin(), m_received_data_callbacks.end(), callback), m_received_data_callbacks.end());
    delete callback;
}

void ZstPlug::run_recv_callbacks(){
    if(m_received_data_callbacks.size() > 0){
        for(vector<PlugCallback*>::iterator callback = m_received_data_callbacks.begin(); callback != m_received_data_callbacks.end(); ++callback){
            (*callback)->run(this);
        }
    }
}

void ZstPlug::fire()
{
	zmsg_t * msg = zmsg_new();

	//First frame is the address of the sender plug
	zmsg_addstr(msg, get_address().to_s().c_str());

	//Second frame is the plug payload
	zframe_t * payload  = zframe_new(m_buffer->data(), m_buffer->size());
	zmsg_append(msg, &payload);

	Showtime::instance().send_to_graph(msg);

	//Clear our buffer when finished
	m_buffer->clear();
}



void ZstIntPlug::fire(int value)
{
	m_packer->pack_int(value);
	ZstPlug::fire();
}

void ZstFloatPlug::fire(float value)
{
	m_packer->pack_float(value);
	ZstPlug::fire();
}

void ZstIntListPlug::fire(std::vector<int> value)
{
	m_packer->pack_array(value.size());
	for (int i = 0; i < sizeof(value); ++i) {
		m_packer->pack(value[i]);
	}
	ZstPlug::fire();
}

void ZstFloatListPlug::fire(std::vector<float> value)
{
	m_packer->pack_array(value.size());
	for (int i = 0; i < sizeof(value); ++i) {
		m_packer->pack(value[i]);
	}
	ZstPlug::fire();
}

void ZstStringPlug::fire(string value)
{
	m_packer->pack(value);
	ZstPlug::fire();
}


void ZstIntPlug::recv(msgpack::object object){
    int out;
    object.convert<int>(out);
    m_value = out;
    run_recv_callbacks();
}

void ZstFloatPlug::recv(msgpack::object object){
    float out;
    object.convert<float>(out);
    m_value = out;
    run_recv_callbacks();
}

void ZstIntListPlug::recv(msgpack::object object){
    std::vector<int> out_arr;
    object.convert<std::vector<int>>(out_arr);
    m_value = out_arr;
    run_recv_callbacks();
}

void ZstFloatListPlug::recv(msgpack::object object){
    vector<float> out;
    object.convert<vector<float>>(out);
    m_value = out;
    run_recv_callbacks();
}

void ZstStringPlug::recv(msgpack::object object){
    string out;
    object.convert<string>(out);
    m_value = out;
    run_recv_callbacks();
}


int ZstIntPlug::get_value(){
    return m_value;
}

float ZstFloatPlug::get_value(){
    return m_value;
}

vector<int> ZstIntListPlug::get_value(){
    return m_value;
}

vector<float> ZstFloatListPlug::get_value(){
    return m_value;
}

string ZstStringPlug::get_value(){
    return m_value;
}


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
	m_packer->pack_int(sizeof(PlugTypes::INT_PLUG));
	m_packer->pack_int(value);
	ZstPlug::fire();
}

void ZstFloatPlug::fire(float value)
{
	m_packer->pack_int(PlugTypes::FLOAT_PLUG);
	m_packer->pack_float(value);
	ZstPlug::fire();
}

void ZstIntArrayPlug::fire(std::vector<int> value)
{
	m_packer->pack_int(PlugTypes::INT_ARR_PLUG);
	m_packer->pack_array(value.size());
	for (int i = 0; i < sizeof(value); ++i) {
		m_packer->pack(value[i]);
	}
	ZstPlug::fire();
}

void ZstFloatArrayPlug::fire(std::vector<float> value)
{
	m_packer->pack_int(PlugTypes::FLOAT_ARR_PLUG);
	m_packer->pack_array(value.size());
	for (int i = 0; i < sizeof(value); ++i) {
		m_packer->pack(value[i]);
	}
	ZstPlug::fire();
}

void ZstStringPlug::fire(string value)
{
	m_packer->pack_int(PlugTypes::STRING_PLUG);
	m_packer->pack_str(value.size());
	m_packer->pack(value);
	ZstPlug::fire();
}


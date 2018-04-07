# include "ZstTransportLayer.h"

ZstTransportLayer::ZstTransportLayer(ZstClient * client) : 
	ZstClientModule(client),
	m_num_graph_recv_messages(0),
	m_num_graph_send_messages(0)
{
}

ZstTransportLayer::~ZstTransportLayer()
{
}

int ZstTransportLayer::graph_recv_tripmeter()
{
	return m_num_graph_recv_messages;
}

void ZstTransportLayer::reset_graph_recv_tripmeter()
{
	m_num_graph_recv_messages = 0;
}

int ZstTransportLayer::graph_send_tripmeter()
{
	return m_num_graph_send_messages;
}

void ZstTransportLayer::reset_graph_send_tripmeter()
{
	m_num_graph_send_messages = 0;
}

void ZstTransportLayer::inc_graph_recv()
{
	m_num_graph_recv_messages++;
}

void ZstTransportLayer::inc_graph_send()
{
	m_num_graph_send_messages++;
}

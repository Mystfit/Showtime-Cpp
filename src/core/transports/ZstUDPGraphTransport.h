#pragma once

#include <showtime/ZstExports.h>
#include "ZstGraphTransport.h"

namespace showtime {
	class ZstUDPGraphTransport :
		public ZstGraphTransport
	{
	public:
		ZST_EXPORT ZstUDPGraphTransport();
		ZST_EXPORT ~ZstUDPGraphTransport();
		ZST_EXPORT virtual void connect(const std::string& address) override;
		ZST_EXPORT void set_incoming_port(uint16_t port);
		ZST_EXPORT uint16_t get_incoming_port();

		ZST_EXPORT virtual int bind(const std::string& address) override;
		ZST_EXPORT virtual void disconnect() override;

	protected:
		ZST_EXPORT virtual void init_graph_sockets() override;

	private:
		uint16_t m_port;
	};
}
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
		ZST_EXPORT virtual int bind(const std::string& address) override;
		ZST_EXPORT virtual void disconnect() override;

	protected:
		ZST_EXPORT virtual void init_graph_sockets() override;
	};
}
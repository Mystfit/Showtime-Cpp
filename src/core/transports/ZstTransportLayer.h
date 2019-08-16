/*
	ZstTransportLayer

	Base class for transports to communicate with a performance
*/

#pragma once

#include "../ZstMessagePool.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"


template<typename T>
class ZstTransportLayer : 
	public ZstTransportLayerBase, 
	public ZstTransportAdaptor
{
public:
	T* get_msg() 
	{
		return m_msg_pool.get_msg();
	}

	void release_msg(T* msg)
	{
		m_msg_pool.release(msg);
	}


	// ----------------
	// Publish messages
	// ----------------

	void send_msg(ZstMsgKind kind)  override
	{
		begin_send_message(get_msg()->init(kind));
	}

	void send_msg(ZstMsgKind kind, const ZstTransportArgs& args)  override
	{
		begin_send_message(get_msg()->init(kind, args.msg_args, args.msg_payload), args);
	}
protected:
	ZstMessagePool<T> m_msg_pool;
};

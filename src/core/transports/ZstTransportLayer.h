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

	void on_send_msg(ZstMsgKind kind)  override
	{
		begin_send_message(get_msg()->init(kind));
	}

	void on_send_msg(ZstMsgKind kind, const ZstMsgArgs & args)  override
	{
		begin_send_message(get_msg()->init(kind, args));
	}

	void on_send_msg(ZstMsgKind kind, const ZstMsgArgs & args, const ZstMsgArgs& payload) override
	{
		begin_send_message(get_msg()->init(kind, payload, args));
	}


	// -----------------
	// Response messages
	// -----------------
	
	void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const MessageReceivedAction & action) override
	{
		begin_send_message(get_msg()->init(kind), sendtype, action);
	}

	void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & args, const MessageReceivedAction & action)  override
	{
		begin_send_message(get_msg()->init(kind, args), sendtype, action);
	}
	
	void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & payload, const ZstMsgArgs & args, const MessageReceivedAction & action) override
	{
		begin_send_message(get_msg()->init(kind, payload, args), sendtype, action);
	}

protected:
	ZstMessagePool<T> m_msg_pool;
};

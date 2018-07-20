/*
	ZstTransportLayer

	Base class for transports to communicate with a performance
*/

#pragma once

#include "../core/ZstMessagePool.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"


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
    
	virtual void send_sock_msg(zsock_t * sock, ZstMessage * msg) override
	{
		ZstTransportLayerBase::send_sock_msg(sock, msg);
		m_msg_pool.release(static_cast<T*>(msg));
	}


	// ----------------
	// Publish messages
	// ----------------

	void send_message(ZstMessage * msg)  override
	{
		begin_send_message(msg);
	}

	void send_message(ZstMsgKind kind, const ZstMsgArgs & args)  override
	{
		T * msg = get_msg()->init(kind, args);
		begin_send_message(msg);
	}

	void send_message(ZstMsgKind kind, const ZstSerialisable & serialisable) override
	{
		begin_send_message(get_msg()->init(kind, serialisable, {}));
	}

	void send_message(ZstMsgKind kind, const ZstMsgArgs & args, const ZstSerialisable & serialisable) override
	{
		T * msg = get_msg()->init(kind, serialisable, args);
		begin_send_message(msg);
	}


	// -----------------
	// Response messages
	// -----------------
	
	void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const MessageReceivedAction & action) override
	{
		begin_send_message(get_msg()->init(kind), sendtype, action);
	}

	void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & args, const MessageReceivedAction & action)  override
	{
		T * msg = get_msg()->init(kind, args);
		begin_send_message(msg, sendtype, action);
	}
	
	void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const MessageReceivedAction & action) override
	{
		begin_send_message(get_msg()->init(kind, serialisable, {}), sendtype, action);
	}

	void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const ZstMsgArgs & args, const MessageReceivedAction & action) override
	{
		T * msg = get_msg()->init(kind, serialisable, args);
		begin_send_message(msg, sendtype, action);
	}

protected:
	ZstMessagePool<T> m_msg_pool;
};

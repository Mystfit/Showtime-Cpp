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
	void on_send_msg(ZstMessage * msg) override
	{
		begin_send_message(msg);
	}

	void on_send_msg(ZstMsgKind kind)  override
	{
		begin_send_message(get_msg()->init(kind));
	}

	void on_send_msg(ZstMsgKind kind, const ZstMsgArgs & args)  override
	{
		begin_send_message(get_msg()->init(kind, args));
	}

	void on_send_msg(ZstMsgKind kind, const ZstSerialisable & serialisable) override
	{
		begin_send_message(get_msg()->init(kind, serialisable, {}));
	}

	void on_send_msg(ZstMsgKind kind, const ZstMsgArgs & args, const ZstSerialisable & serialisable) override
	{
		begin_send_message(get_msg()->init(kind, serialisable, args));
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
	
	void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const MessageReceivedAction & action) override
	{
		begin_send_message(get_msg()->init(kind, serialisable, {}), sendtype, action);
	}

	void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const ZstMsgArgs & args, const MessageReceivedAction & action) override
	{
		begin_send_message(get_msg()->init(kind, serialisable, args), sendtype, action);
	}

protected:
	ZstMessagePool<T> m_msg_pool;
};

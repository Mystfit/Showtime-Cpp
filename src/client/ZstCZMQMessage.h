#pragma once

#include <czmq.h>
#include "../core/ZstMessage.h"

/**
 * Class:	ZstCZMQMessagePayload
 *
 * Summary:	CZMQ variation of a ZstMessagePayload.
 */
class ZstCZMQMessagePayload : public ZstMessagePayload {
public:
	/**
	 * Fn:	ZstCZMQMessagePayload::ZstCZMQMessagePayload(ZstMsgKind k, const zframe_t * p);
	 *
	 * Summary:	Construct a single payload frame for a ZstCZMQMessage.
	 *
	 * Parameters:
	 * k - 		  	The ZstMsgKind to assign to this payload.
	 * p - 		  	The zframe_t data to assign to this payload.
	 */
	ZstCZMQMessagePayload(ZstMsgKind k, const zframe_t * p);

	/**
	 * Fn:	ZstCZMQMessagePayload::ZstCZMQMessagePayload(const ZstCZMQMessagePayload & other);
	 *
	 * Summary:	Copy construct a ZstCZMQMessagePayload.
	 *
	 * Parameters:
	 * other - 	Payload to copy.
	 */
	ZstCZMQMessagePayload(const ZstCZMQMessagePayload & other);

	/**
	 * Fn:	ZstCZMQMessagePayload::~ZstCZMQMessagePayload();
	 *
	 * Summary:	Destructor.
	 */
	~ZstCZMQMessagePayload();

	/**
	 * Fn:	const void * ZstCZMQMessagePayload::data() override;
	 *
	 * Summary:	Gets the payload data.
	 *
	 * Returns:	Null if it fails, else a pointer to a const void.
	 */
	const void * data() override;
};


/**
 * Class:	ZstCZMQMessage
 *
 * Summary:
 *  A ZstCZMQMessage encapsulates a single message sent to or from the performance stage server
 *  using ZeroMQ sockets.
 */
class ZstCZMQMessage : public ZstMessage {
public:
	ZstCZMQMessage();

	/**
	 * Fn:	ZstCZMQMessage::~ZstCZMQMessage();
	 *
	 * Summary:	Destructor.
	 */
	~ZstCZMQMessage();

	/**
	 * Fn:	ZstCZMQMessage::ZstCZMQMessage(const ZstCZMQMessage & other);
	 *
	 * Summary:	Copy construct a ZstCZMQMessage.
	 *
	 * Parameters:
	 * other - 	The message to copy.
	 */
	ZstCZMQMessage(const ZstCZMQMessage & other);

	/**
	 * Fn:	void ZstCZMQMessage::reset() override;
	 *
	 * Summary:	Resets this message.
	 */
	void reset() override;

	/**
	 * Fn:	void ZstCZMQMessage::copy_id(const ZstCZMQMessage * msg);
	 *
	 * Summary:	Copies the identifier from the provided message.
	 *
	 * Parameters:
	 * msg - 	The message to copy the id from.
	 */
	void copy_id(const ZstCZMQMessage * msg);
	
	void unpack(void * msg) override;

	zmsg_t * handle();

private:
	ZstMsgKind unpack_kind();
	ZstMsgKind unpack_kind(zframe_t * kind_frame);

	void append_kind_frame(ZstMsgKind k) override;
	void append_id_frame() override;
	void append_payload_frame(const ZstSerialisable & streamable) override;
	void append_str(const char * s, size_t len) override;
	void append_serialisable(ZstMsgKind k, const ZstSerialisable & s) override;

	zmsg_t * m_msg_handle;
};


class ZstCZMQStageMessage : 
	public ZstStageMessage, 
	public ZstCZMQMessage 
{};


class ZstCZMQPerformanceMessage : 
	public ZstPerformanceMessage, 
	public ZstCZMQMessage 
{};
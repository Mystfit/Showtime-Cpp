#pragma once

#include <czmq.h>
#include <ZstExports.h>
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
	 * Summary:	The data stored in this payload.
	 *
	 * Returns:	Null if it fails, else a pointer to a const void.
	 */
	const virtual void * data() override;

	/**
	* Fn:	const void * ZstCZMQMessagePayload::size() override;
	*
	* Summary:	Size of the payload.
	*
	* Returns:	size_t
	*/
	const size_t size() override;
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
	ZST_EXPORT ZstCZMQMessage();

	/**
	 * Fn:	ZstCZMQMessage::~ZstCZMQMessage();
	 *
	 * Summary:	Destructor.
	 */
	ZST_EXPORT ~ZstCZMQMessage();

	/**
	 * Fn:	ZstCZMQMessage::ZstCZMQMessage(const ZstCZMQMessage & other);
	 *
	 * Summary:	Copy construct a ZstCZMQMessage.
	 *
	 * Parameters:
	 * other - 	The message to copy.
	 */
	ZST_EXPORT ZstCZMQMessage(const ZstCZMQMessage & other);

	/**
	 * Fn:	void ZstCZMQMessage::reset() override;
	 *
	 * Summary:	Resets this message.
	 */
	ZST_EXPORT void reset() override;

	/**
	 * Fn:	void ZstCZMQMessage::copy_id(const ZstCZMQMessage * msg);
	 *
	 * Summary:	Copies the identifier from the provided message.
	 *
	 * Parameters:
	 * msg - 	The message to copy the id from.
	 */
	ZST_EXPORT void copy_id(const ZstMessage * msg) override;

	ZST_EXPORT ZstMessagePayload & payload_at(size_t index) override;
	ZST_EXPORT size_t num_payloads() override;
	
	ZST_EXPORT void unpack(void * msg) override;

	ZST_EXPORT zmsg_t * handle();

	void append_str(const char * s, size_t len) override;
	void append_serialisable(ZstMsgKind k, const ZstSerialisable & s) override;

private:
	ZstMsgKind unpack_kind();
	ZstMsgKind unpack_kind(zframe_t * kind_frame);

	void append_kind_frame(ZstMsgKind k) override;
	void append_id_frame() override;
	void append_payload_frame(const ZstSerialisable & streamable) override;

	zmsg_t * m_msg_handle;
	std::vector<ZstCZMQMessagePayload> m_payloads;
};

#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <ZstCore.h>

#define KIND_FRAME_SIZE 1
#define UUID_LENGTH 33	//Size of a CZMQ uuid (32 bytes + null terminator)

/**
 * Enum:	ZstMsgKind
 *
 * Summary:	Values that represent possible messages.
 */
enum ZstMsgKind  {
    EMPTY = 0,
    
    //Regular signals
    OK,
    
    //Error signals
    ERR_STAGE_MSG_TYPE_UNKNOWN,
    ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST,
    ERR_STAGE_BAD_CABLE_CONNECT_REQUEST,
    ERR_STAGE_PERFORMER_NOT_FOUND,
    ERR_STAGE_PERFORMER_ALREADY_EXISTS,
    ERR_STAGE_ENTITY_NOT_FOUND,
    ERR_STAGE_ENTITY_ALREADY_EXISTS,
    ERR_STAGE_PLUG_ALREADY_EXISTS,
	ERR_STAGE_TIMEOUT,
    
    //Client registration
    CLIENT_JOIN,
    CLIENT_SYNC,
    CLIENT_LEAVING,
    CLIENT_HEARTBEAT,
    GRAPH_SNAPSHOT,
    
    //Entity registration
    CREATE_ENTITY_FROM_TEMPLATE,
    REGISTER_COMPONENT_TEMPLATE,
    UNREGISTER_COMPONENT_TEMPLATE,
    CREATE_COMPONENT,
    CREATE_CONTAINER,
    CREATE_PERFORMER,
    DESTROY_ENTITY,
    
    //Plug registration
    CREATE_PLUG,
	    
    //Connection registration
    CREATE_CABLE,
    DESTROY_CABLE,
    
    //P2P endpoint connection requests
    SUBSCRIBE_TO_PERFORMER,
    CREATE_PEER_ENTITY
};
MSGPACK_ADD_ENUM(ZstMsgKind);


/**
 * Class:	ZstMessagePayload
 *
 * Summary:	A single payload frame in a ZstMessage
 */
class ZstMessagePayload {
public:

    /**
     * Fn:	ZST_EXPORT Summary::ZstMessagePayload(ZstMsgKind k, const void * p);
     *
     * Summary:	Construct a single payload frame for a ZstMessage.
     *
     * Parameters:
     * k - 		  	The ZstMsgKind to assign to this payload.
     * p - 		  	The data to assign to this payload.
     *
     * Returns:	A ZstMessagePayload.
     */
    ZST_EXPORT ZstMessagePayload(ZstMsgKind k, const void * p);

    /**
     * Fn:	ZST_EXPORT Summary::ZstMessagePayload(const ZstMessagePayload & other);
     *
     * Summary:	Copy construct a ZstMessagePayload.
     *
     * Parameters:
     * other - 	Payload to copy.
     */
    ZST_EXPORT ZstMessagePayload(const ZstMessagePayload & other);

	/**
	 * Fn:	ZST_EXPORT virtual ZstMessagePayload::~ZstMessagePayload()
	 *
	 * Summary:	Destructor.
	 */
	ZST_EXPORT virtual ~ZstMessagePayload() {};

	/**
	 * Fn:	ZST_EXPORT ZstMsgKind ZstMessagePayload::kind();
	 *
	 * Summary:	Gets the payload kind.
	 *
	 * Returns:	A ZstMsgKind.
	 */
	ZST_EXPORT ZstMsgKind kind();

    /**
     * Fn:	ZST_EXPORT size_t ZstMessagePayload::size();
     *
     * Summary:	Gets the payload size.
     *
     * Returns:	A size_t.
     */
    ZST_EXPORT size_t size();

	/**
	 * Fn:	ZST_EXPORT virtual const void * ZstMessagePayload::data();
	 *
	 * Summary:	Gets the payload data.
	 *
	 * Returns:	Null if it fails, else a const void*.
	 */
	ZST_EXPORT virtual const void * data();

protected:
	/** Summary:	The payload. */
	const void * m_payload;
	
	/** Summary:	The size. */
	size_t m_size;
	
private:
    /** Summary:	The kind. */
    ZstMsgKind m_kind;
};


/**
 * Class:	ZstMessage
 *
 * Summary:
 *  A ZstMessage encapsulates a single message sent to or from the performance stage server. This
 *  class can be extended to account for your transport mechanism of choice.
 */
class ZstMessage {
public:

	/**
	* Fn:	ZstMessage::ZstMessage();
	*
	* Summary:	Default constructor.
	*/
	ZST_EXPORT ZstMessage();

	/**
	 * Fn:	ZST_EXPORT virtual ZstMessage::~ZstMessage();
	 *
	 * Summary:	Destructor.
	 */
	ZST_EXPORT virtual ~ZstMessage();

	/**
	 * Fn:	ZST_EXPORT ZstMessage::ZstMessage(const ZstMessage & other);
	 *
	 * Summary: Copy-construct a ZstMessage.
	 *
	 * Parameters:
	 * other - 	The message to copy
	 */
	ZST_EXPORT ZstMessage(const ZstMessage & other);

	/**
	 * Fn:	ZST_EXPORT virtual void ZstMessage::reset();
	 *
	 * Summary:	Resets this message.
	 */
	ZST_EXPORT virtual void reset();

	/**
	 * Fn:	ZST_EXPORT virtual void ZstMessage::copy_id(const ZstMessage * msg);
	 *
	 * Summary:	Copies the identifier from the provided message.
	 *
	 * Parameters:
	 * msg - 	The message to copy the id from.
	 */
	ZST_EXPORT virtual void copy_id(const ZstMessage * msg);

	/**
	 * Fn:	ZST_EXPORT virtual void ZstMessage::unpack(void * msg) = 0;
	 *
	 * Summary:	Unpacks a transport specific message into this ZstMessage.
	 *
	 * Parameters:
	 * msg - 	[in,out] If non-null, the transport-specific message data to unpack. May modify input message depending on transport used.
	 */
	ZST_EXPORT virtual void unpack(void * msg) = 0;

	/**
	 * Fn:	ZST_EXPORT virtual void ZstMessage::append_str(const char * s, size_t len) = 0;
	 *
	 * Summary:	Appends a string payload to this ZstMessage.
	 *
	 * Parameters:
	 * s - 		  	A char to append to this message.
	 * len - 	  	The length of the string.
	 */
	ZST_EXPORT virtual void append_str(const char * s, size_t len) = 0;

	/**
	 * Fn:
	 *  ZST_EXPORT virtual void ZstMessage::append_serialisable(ZstMsgKind k,
	 *  const ZstSerialisable & s) = 0;
	 *
	 * Summary:	Appends a serialisable.
	 *
	 * Parameters:
	 * k - 		  	The kind of message to initialise.
	 * s - 		  	The serialisable object to append.
	 */
	ZST_EXPORT virtual void append_serialisable(ZstMsgKind k, const ZstSerialisable & s) = 0;

	/**
	 * Fn:	ZST_EXPORT const char * ZstMessage::id();
	 *
	 * Summary:	Gets the identifier for this message.
	 *
	 * Returns:	Null if it fails, else a pointer to a const char.
	 */
	ZST_EXPORT const char * id();

	/**
	 * Fn:	ZST_EXPORT ZstMsgKind ZstMessage::kind();
	 *
	 * Summary:	Gets the ZstMsgKind of this message.
	 *
	 * Returns:	A ZstMsgKind.
	 */
	ZST_EXPORT ZstMsgKind kind();

	/**
	 * Fn:	ZST_EXPORT ZstMessagePayload & ZstMessage::payload_at(size_t index);
	 *
	 * Summary:	Gets the payload at a specified index.
	 *
	 * Parameters:
	 * index - 	Zero-based index of the payload.
	 *
	 * Returns:	A reference to a ZstMessagePayload.
	 */
	ZST_EXPORT ZstMessagePayload & payload_at(size_t index);

	/**
	 * Fn:	ZST_EXPORT size_t ZstMessage::num_payloads();
	 *
	 * Summary:	Number of payloads in this message.
	 *
	 * Returns:	A size_t of the total number of payloads this message contains.
	 */
	ZST_EXPORT size_t num_payloads();

	/**
	 * Fn:	template <typename T> T ZstMessage::unpack_payload_serialisable(size_t payload_index)
	 *
	 * Summary:	Unpack a serialisable payload.
	 *
	 * Typeparams:
	 * T -  Type of serialisable to unpack.
	 * Parameters:
	 * payload_index - 	Zero-based index of the payload to unpack from.
	 *
	 * Returns:	A serialisable.
	 */
	template <typename T>
	T unpack_payload_serialisable(size_t payload_index) {
		T serialisable;
		size_t offset = 0;
		ZstMessagePayload & payload = payload_at(payload_index);
		serialisable.read((char*)payload.data(), payload.size(), offset);
		return serialisable;
	}

	/**
	* Fn:	void ZstMessage::append_entity_kind_frame(const ZstEntityBase * entity);
	*
	* Summary:	Appends an entity kind frame to this message.
	*
	* Parameters:
	* entity - 	The entity to append.
	*/
	void append_entity_kind_frame(const ZstEntityBase * entity);

	/**
	* Fn:	virtual void ZstMessage::append_kind_frame(ZstMsgKind k) = 0;
	*
	* Summary:	Appends a kind frame.
	*
	* Parameters:
	* k - 	A ZstMsgKind to append.
	*/
	virtual void append_kind_frame(ZstMsgKind k) = 0;

	/**
	* Fn:	virtual void ZstMessage::append_id_frame() = 0;
	*
	* Summary:	Appends the identifier frame.
	*/
	virtual void append_id_frame() = 0;

	/**
	* Fn:	virtual void ZstMessage::append_payload_frame(const ZstSerialisable & streamable) = 0;
	*
	* Summary:	Appends a payload frame.
	*
	* Parameters:
	* streamable - 	The streamable to append.
	*/
	virtual void append_payload_frame(const ZstSerialisable & streamable) = 0;

protected:
	/** Summary:	The message kind. */
	ZstMsgKind m_msg_kind;

	/** Summary:	The payloads. */
	std::vector<ZstMessagePayload> m_payloads;

	/** Summary:	The message id. */
	char m_msg_id[UUID_LENGTH];
};


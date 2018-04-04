#pragma once
#include <map>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.h"

/**
 * Struct:	ZstTimeoutException
 *
 * Summary:	ZstTimeoutException will throw if the stage doesn't respond to our request.
 */
struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

/**
 * Class:	ZstMessageDispatcher
 *
 * Summary:
 *  ZstRequestDispatcher prepares request messages that will be sent to the stage
 *  		server.
 *  
 *  		Uses sync and async communication methods to talk with the stage asyncronously, but the
 *  		sync message will block until the async reply returns from the server or times out.
 */
class ZstMessageDispatcher{
public:

	/**
	 * Fn:	ZstMessageDispatcher::ZstMessageDispatcher();
	 *
	 * Summary:	Constructor.
	 */
	ZstMessageDispatcher();

	/**
	 * Fn:	ZstMessageDispatcher::~ZstMessageDispatcher();
	 *
	 * Summary:	Destructor.
	 */
	~ZstMessageDispatcher();

	/**
	 * Fn:	ZstMsgKind ZstMessageDispatcher::prepare_sync_message(const ZstMessage * msg);
	 *
	 * Summary:	Prepare a message to be sent syncronously.
	 *
	 * Parameters:
	 * msg - 	The message.
	 *
	 * Returns:	A ZstMsgKind.
	 */
	ZstMsgKind prepare_sync_message(const ZstMessage * msg);

	/**
	 * Fn:	void ZstMessageDispatcher::prepare_async_message(const ZstMessage * msg);
	 *
	 * Summary:	Prepare message to be sent asynchronously.
	 *
	 * Parameters:
	 * msg - 	The message.
	 */
	void prepare_async_message(const ZstMessage * msg);

	/**
	 * Fn:	virtual void ZstMessageDispatcher::complete(ZstMsgKind status);
	 *
	 * Summary:	Completed fires when it receives a response from the server.
	 *
	 * Parameters:
	 * status - 	Status returned from the server.
	 */
	virtual void complete(ZstMsgKind status);

	/**
	 * Fn:	virtual void ZstMessageDispatcher::failed(ZstMsgKind status);
	 *
	 * Summary:	Failed fires when a non-OK message response is received from the server.
	 *
	 * Parameters:
	 * status - 	Status returned from the server.
	 */
	virtual void failed(ZstMsgKind status);

	/**
	 * Fn:	ZstMessagePool & ZstMessageDispatcher::msg_pool();
	 *
	 * Summary:	Message pool.
	 *
	 * Returns:	A reference to a ZstMessagePool.
	 */
	ZstMessagePool & msg_pool();
	
private:

	/**
	 * Typedef:	cf::promise<ZstMsgKind> MessagePromise
	 *
	 * Summary:	Defines an alias representing the message promise.
	 */
	typedef cf::promise<ZstMsgKind> MessagePromise;

	/**
	 * Typedef:	cf::future<ZstMsgKind> MessageFuture
	 *
	 * Summary:	Defines an alias representing the message future.
	 */
	typedef cf::future<ZstMsgKind> MessageFuture;

	/**
	 * Fn:	MessageFuture ZstMessageDispatcher::register_response_message(const ZstMessage * msg);
	 *
	 * Summary:	Register a message that will receive a server response.
	 *
	 * Parameters:
	 * msg - 	The message.
	 *
	 * Returns:	A MessageFuture.
	 */
	MessageFuture register_response_message(const ZstMessage * msg);

	/**
	 * Fn:	int ZstMessageDispatcher::process_response_message(const ZstMessage * msg);
	 *
	 * Summary:	Process messages waiting for a server response.
	 *
	 * Parameters:
	 * msg - 	The message.
	 *
	 * Returns:	An int. 0 for success, anything under 0 is considered a fail.
	 */
	int process_response_message(const ZstMessage * msg);

	/** Summary:	The promise messages. */
	std::unordered_map<std::string, MessagePromise > m_promise_messages;

	/** Summary:	The timeout watcher. */
	cf::time_watcher m_timeout_watcher;
	
	/** Summary:	The message pool. */
	ZstMessagePool m_message_pool;
};

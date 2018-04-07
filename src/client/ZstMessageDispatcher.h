#pragma once
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>
#include "../core/ZstMessage.h"
#include "ZstTransportLayer.h"
#include "ZstClientModule.h"

/**
 * Struct:	ZstTimeoutException
 *
 * Summary:	ZstTimeoutException will throw if the stage doesn't respond to our request.
 */
struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct ZstMessageReceipt {
	ZstMsgKind status;
	bool async;
};

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
 * Typedef:	boost::function<void(ZstMsgKind)> MessageBoundAction
 *
 * Summary: Defines an alias representing a message completion action.
 */
typedef std::function<void(ZstMessageReceipt)> MessageBoundAction;


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
class ZstMessageDispatcher : public ZstClientModule {
public:

	/**
	 * Fn:	ZstMessageDispatcher::ZstMessageDispatcher(ZstTransportLayer * transport);
	 *
	 * Summary:	Constructor.
	 *
	 * Parameters:
	 * transport - 	[in,out] If non-null, the transport for sending/receiving messages.
	 */
	ZstMessageDispatcher(ZstClient * client, ZstTransportLayer * transport);
	
	/**
	 * Fn:	ZstMessageDispatcher::~ZstMessageDispatcher();
	 *
	 * Summary:	Destructor.
	 */
	~ZstMessageDispatcher();

	/**
	 * Fn:	ZstMsgKind ZstMessageDispatcher::send_to_stage(ZstMessage * msg, bool async);
	 *
	 * Summary:	Sends a message to the stage.
	 *
	 * Parameters:
	 * msg - 	  	[in,out] If non-null, the message to send.
	 * action -		Callback method to run when the response is received
	 * async -    	Send message asynchronously.
	 *
	 * Returns:	A ZstMsgKind.
	 */
	ZstMessageReceipt send_to_stage(ZstMessage * msg, MessageBoundAction action, bool async);
	

	/**
	 * Fn:	ZstMsgKind ZstMessageDispatcher::send_sync_stage_message(const ZstMessage * msg);
	 *
	 * Summary:	Prepare a message to be sent syncronously.
	 *
	 * Parameters:
	 * msg - 	The message.
	 *
	 * Returns:	A ZstMsgKind.
	 */
	ZstMessageReceipt send_sync_stage_message(ZstMessage * msg);

	/**
	 * Fn:	void ZstMessageDispatcher::send_async_stage_message(const ZstMessage * msg);
	 *
	 * Summary:	Prepare message to be sent asynchronously.
	 *
	 * Parameters:
	 * msg - 	The message.
	 */
	ZstMessageReceipt send_async_stage_message(ZstMessage * msg, MessageBoundAction completed_action);

	/**
	 * Fn:	virtual void ZstMessageDispatcher::complete(ZstMsgKind status);
	 *
	 * Summary:	Completed fires when it receives a response from the server.
	 *
	 * Parameters:
	 * status - 	Status returned from the server.
	 */
	virtual void complete(ZstMessageReceipt response);

	/**
	 * Fn:	virtual void ZstMessageDispatcher::failed(ZstMsgKind status);
	 *
	 * Summary:	Failed fires when a non-OK message response is received from the server.
	 *
	 * Parameters:
	 * status - 	Status returned from the server.
	 */
	virtual void failed(ZstMessageReceipt status);

	ZstMessage * init_entity_message(const ZstEntityBase * entity);
	ZstMessage * init_message(ZstMsgKind kind);
	ZstMessage * init_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable);
			
private:
	ZstMessageDispatcher();

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
	MessageFuture register_response_message(ZstMessage * msg);

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
	int process_response_message(ZstMessage * msg);

	/** Summary:	The promise messages. */
	std::unordered_map<std::string, MessagePromise > m_promise_messages;

	/** Summary:	The timeout watcher. */
	cf::time_watcher m_timeout_watcher;

	/** Summary:	The message transporter. */
	ZstTransportLayer * m_transport;
};

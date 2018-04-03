

#pragma once
#include <map>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.h"

///-------------------------------------------------------------------------------------------------
/// Struct:	ZstTimeoutException
///
/// Summary:	ZstTimeoutException will throw if the stage doesn't respond to our request.

struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

///-------------------------------------------------------------------------------------------------
/// Class:	ZstRequestDispatcher
///
/// Summary:	ZstRequestDispatcher prepares request messages that will be sent to the stage
/// 	server.
/// 	
/// 	Uses sync and async communication methods to talk with the stage asyncronously, but the
/// 	sync message will block until the async reply returns from the server or times out.

class ZstRequestDispatcher{
public:

	/**
	 * Constructor
	 */
	ZstRequestDispatcher();

	/**
	 * Destructor
	 */
	~ZstRequestDispatcher();

	/**
	 * @brief 
	 * 
	 * @param msg 
	 * @return ZstMsgKind 
	 */
	ZstMsgKind prepare_sync_message(const ZstMessage * msg);

	/**
	 * Prepare message to be sent asynchronously
	 *
	 * @param msg If non-null, the message.
	 *
	 * @return A ZstMsgKind.
	 */

	ZstMsgKind prepare_async_message(const ZstMessage * msg);

	//--------------
	//Request events
	//--------------

	/**
	 * Completed fires when it receives a response from the server.
	 *
	 * @param status Status returned from the server.
	 */

	virtual void complete(ZstMsgKind status);

	/**
	 * Failed fires when a non-OK message response is received from the server.
	 *
	 * @param status Status returned from the server.
	 */

	virtual void failed(ZstMsgKind status);
	
private:

	/**
	* Defines an alias representing the message promise.
	*/

	typedef cf::promise<ZstMsgKind> MessagePromise;

	/**
	* Defines an alias representing the message future.
	*/

	typedef cf::future<ZstMsgKind> MessageFuture;

	/**
	 * Register a message that will receive a server response.
	 *
	 * @param msg The message.
	 *
	 * @return A MessageFuture.
	 */

	MessageFuture register_response_message(const ZstMessage * msg);

	/**
	 * Check for existing message promises matching a message ID.
	 *
	 * @param msg The message to check.
	 *
	 * @return Int status. 0 for success, anything less than 0 is an error.
	 */

	int process_message_promise(const ZstMessage * msg);

	/**
	 * Promise messages waiting for server responses
	 */

	std::unordered_map<std::string, MessagePromise > m_promise_messages;

	/**
	 * Timeout watcher.
	 */

	cf::time_watcher m_timeout_watcher;
};

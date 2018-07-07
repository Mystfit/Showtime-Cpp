#include "../core/ZstTransportLayer.h"
#include "../core/ZstActor.h"
#include "../core/ZstMessagePool.hpp"

#define HEARTBEAT_DURATION 1000
#define MAX_MISSED_HEARTBEATS 10
#define STAGE_MESSAGE_POOL_BLOCK 512

class ZstStageTransport : 
	public ZstTransportLayer,
	public ZstActor
{
public:
	ZstStageTransport();
	~ZstStageTransport();
	void init() override;
	void destroy() override;

	//Incoming socket handlers
	static int s_handle_router(zloop_t *loop, zsock_t *sock, void *arg);

	//Client communication
	void send_to_client(ZstStageMessage * msg, ZstPerformer * destination);

	void publish_stage_update(ZstStageMessage * msg);

private:

	//Stage pipes
	zsock_t * m_performer_router;
	zsock_t * m_graph_update_pub;

	//Message pools
	ZstMessagePool<ZstStageMessage> * m_message_pool;
	ZstMessagePool<ZstStageMessage> * msg_pool();
};
#pragma once

namespace showtime {
	class IEventDispatcher {
	public:
		virtual void prune_missing_adaptors() = 0;
	};
}

#pragma once

#define GLOBAL_CONSOLE "console"

#include <spdlog/spdlog.h>

#define LOGGER spdlog::get(GLOBAL_CONSOLE)

extern "C" {
	static inline void zst_log_init(const char * pattern = "[%H:%M:%S.%e] [PID:%P] [TID:%t] [%l] %v") {
		//spdlog::set_async_mode(4096);
		
		try {
			auto logger = spdlog::stdout_color_mt(GLOBAL_CONSOLE);
			logger->set_pattern(pattern);
		}
		catch (spdlog::spdlog_ex e) {
			//Logger already exists, be quiet
		}
	}
}

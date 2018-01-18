#pragma once

#define GLOBAL_CONSOLE "console"

#include <spdlog/spdlog.h>

#define LOGGER spdlog::get(GLOBAL_CONSOLE)

static void ZST_init_log(){
	spdlog::set_async_mode(4096);
	try {
		spdlog::stdout_color_mt(GLOBAL_CONSOLE);
	}
	catch (spdlog::spdlog_ex e) {
		//Logger already exists, be quiet
	}
}

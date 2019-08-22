#pragma once

#include <stdexcept>

struct ZstTimeoutException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

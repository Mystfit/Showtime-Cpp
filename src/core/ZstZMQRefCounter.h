#pragma once

#include <showtime/ZstExports.h>
#include <unordered_map>
#include <string>

ZST_EXPORT void zst_zmq_inc_ref_count(const char* ref_name);
ZST_EXPORT void zst_zmq_dec_ref_count(const char* ref_name);
ZST_EXPORT int zst_zmq_total_ref_count();
ZST_EXPORT std::unordered_map<std::string, int> zst_zmq_ref_counts();
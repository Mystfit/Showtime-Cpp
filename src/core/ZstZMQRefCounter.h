#pragma once

#include <showtime/ZstExports.h>

ZST_EXPORT void zst_zmq_inc_ref_count();
ZST_EXPORT void zst_zmq_dec_ref_count();
ZST_EXPORT int zst_zmq_ref_count();

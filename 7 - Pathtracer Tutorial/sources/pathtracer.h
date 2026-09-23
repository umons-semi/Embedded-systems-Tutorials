#pragma once

#include "ap_axi_sdata.h"
//#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_stream.h"
#include "hls_math.h"

#include "types.h"

typedef  hls::axis<int, 0, 0, 0, (AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST | AXIS_ENABLE_STRB), false> packet;

extern void pathtracer_compute(hls::stream<packet> &r_stream, hls::stream<packet> &g_stream, hls::stream<packet> &b_stream, int &width, int &height, int &samples_per_pixel);
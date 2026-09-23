#pragma once

#include "ap_axi_sdata.h"
#include "hls_stream.h"
//#include "ap_int.h"
#include "ap_fixed.h"
#include "types.h"

#define MAX_WIDTH 256

typedef ap_fixed<32, 16> data_type;
typedef  hls::axis<int, 0, 0, 0, (AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST | AXIS_ENABLE_STRB), false> packet;

extern int imagefiltering_compute(hls::stream<packet> &input, hls::stream<packet> &output, int &in_width, int &in_height, float kernel[9]);

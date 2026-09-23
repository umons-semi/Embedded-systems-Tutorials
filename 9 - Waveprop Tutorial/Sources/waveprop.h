#pragma once

#include "ap_axi_sdata.h"
//#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_stream.h"
#include "hls_math.h"

//#include "types.h"

#define size 100

typedef ap_fixed<32, 16> data_type;

typedef  hls::axis<int, 0, 0, 0, (AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST | AXIS_ENABLE_STRB), false> packet;

extern int waveprop_compute(hls::stream<packet> &output_stream);
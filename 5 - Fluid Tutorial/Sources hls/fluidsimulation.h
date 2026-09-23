#pragma once

#include "ap_axi_sdata.h"
//#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_stream.h"
#include "hls_math.h"

// Simulation parameters
#define N 100          // Grid size
#define DT 0.1      // Time step
#define DIFF 0.0    // Diffusion rate
#define VISC 0.0001 // Viscosity
#define SIZE (N + 2) // Array dimension including boundaries

typedef ap_fixed<32, 16> data_type;
//typedef float data_type;

typedef  hls::axis<int, 0, 0, 0, (AXIS_ENABLE_KEEP | AXIS_ENABLE_LAST | AXIS_ENABLE_STRB), false> packet;

extern int fluidsimulation_compute(hls::stream<packet> &output_stream, int &frame);
#ifndef WAHWAH_H
#define WAHWAH_H

#include <hls_stream.h>
#include <ap_axi_sdata.h>

typedef ap_axis<32, 1, 1, 1> AXI_DATA;

float wah_wah_filter(
    float input,
    float time,
    float sample_rate,
    float min_freq,
    float max_freq,
    float lfo_freq
);

void wah_wah_filter_axi(
    hls::stream<AXI_DATA>& input_stream,
    hls::stream<AXI_DATA>& output_stream,
    int &sample_rate,
    float &min_freq,
    float &max_freq,
    float &lfo_freq
);

#endif

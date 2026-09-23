#include "wahwah.h"
#include <ap_int.h>
#include "hls_math.h"

static float previous_output = 0.0f;

float wah_wah_filter(
    float input,
    float time,
    float sample_rate,
    float min_freq,
    float max_freq,
    float lfo_freq
) {
    const float PI = 3.14159265358979f;

    const float lfo =
        0.5f * (1.0f + hls::sin(2.0f * PI * lfo_freq * time));

    const float cutoff_freq =
        min_freq + lfo * (max_freq - min_freq);

    const float rc = 1.0f / (2.0f * PI * cutoff_freq);
    const float alpha = rc / (rc + 1.0f / sample_rate);

    const float output =
        alpha * previous_output + (1.0f - alpha) * input;

    previous_output = output;
    return output;
}

void wah_wah_filter_axi(
    hls::stream<AXI_DATA>& input_stream,
    hls::stream<AXI_DATA>& output_stream,
    int &sample_rate,
    float &min_freq,
    float &max_freq,
    float &lfo_freq
) {
#pragma HLS INTERFACE axis port=input_stream
#pragma HLS INTERFACE axis port=output_stream
#pragma HLS INTERFACE s_axilite port=sample_rate bundle=control
#pragma HLS INTERFACE s_axilite port=min_freq bundle=control
#pragma HLS INTERFACE s_axilite port=max_freq bundle=control
#pragma HLS INTERFACE s_axilite port=lfo_freq bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    static float time = 0.0f;

process_stream:
    while (true) {
#pragma HLS PIPELINE II=1

        AXI_DATA input_data = input_stream.read();

        // TDATA est un vecteur de bits non signé.
        // On le réinterprète explicitement comme un entier PCM signé sur 32 bits.
        ap_int<32> signed_bits = input_data.data;
        float input_sample = static_cast<float>(signed_bits);

        float filtered_sample = wah_wah_filter(
            input_sample,
            time,
            static_cast<float>(sample_rate),
            min_freq,
            max_freq,
            lfo_freq
        );

        time += 1.0f / static_cast<float>(sample_rate);

        ap_int<32> output_sample = static_cast<ap_int<32> >(filtered_sample);

        AXI_DATA output_data;
        output_data.data = output_sample;
        output_data.keep = input_data.keep;
        output_data.strb = input_data.strb;
        output_data.user = input_data.user;
        output_data.last = input_data.last;
        output_data.id   = input_data.id;
        output_data.dest = input_data.dest;

        output_stream.write(output_data);

        if (input_data.last) {
            break;
        }
    }
}

#include "wahwah.h"

#include <ap_int.h>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

static std::vector<int32_t> make_test_signal(
    int sample_rate,
    int number_of_samples
) {
    const double PI = 3.14159265358979323846;
    std::vector<int32_t> signal(number_of_samples);

    // Somme de deux sinusoïdes : 300 Hz et 2 kHz.
    // Amplitude limitée pour rester dans int32_t.
    for (int n = 0; n < number_of_samples; ++n) {
        double t = static_cast<double>(n) / sample_rate;
        double value =
            12000.0 * std::sin(2.0 * PI * 300.0 * t) +
             6000.0 * std::sin(2.0 * PI * 2000.0 * t);

        signal[n] = static_cast<int32_t>(value);
    }

    return signal;
}

int main() {
    const int number_of_samples = 1024;

    int sample_rate = 48000;
    float min_freq = 500.0f;
    float max_freq = 3000.0f;
    float lfo_freq = 1.0f;

    hls::stream<AXI_DATA> input_stream("input_stream");
    hls::stream<AXI_DATA> output_stream("output_stream");

    std::vector<int32_t> input =
        make_test_signal(sample_rate, number_of_samples);

    // Injection des échantillons dans le flux AXI4-Stream.
    for (int i = 0; i < number_of_samples; ++i) {
        AXI_DATA packet;

        ap_int<32> signed_sample = input[i];
        packet.data = signed_sample;
        packet.keep = 0xF;
        packet.strb = 0xF;
        packet.user = 0;
        packet.id   = 0;
        packet.dest = 0;
        packet.last = (i == number_of_samples - 1) ? 1 : 0;

        input_stream.write(packet);
    }

    // Un appel traite le flux jusqu'au paquet portant TLAST.
    wah_wah_filter_axi(
        input_stream,
        output_stream,
        sample_rate,
        min_freq,
        max_freq,
        lfo_freq
    );

    int errors = 0;
    int received = 0;
    bool last_seen = false;

    while (!output_stream.empty()) {
        AXI_DATA packet = output_stream.read();
        ap_int<32> signed_output = packet.data;
        int32_t output_value = static_cast<int32_t>(signed_output);

        if (received < 10) {
            std::cout
                << "n=" << received
                << " input=" << input[received]
                << " output=" << output_value
                << " last=" << packet.last
                << std::endl;
        }

        if (packet.keep != 0xF || packet.strb != 0xF) {
            std::cerr << "Erreur KEEP/STRB à n=" << received << std::endl;
            ++errors;
        }

        if (packet.last) {
            last_seen = true;

            if (received != number_of_samples - 1) {
                std::cerr
                    << "TLAST reçu trop tôt à n=" << received
                    << std::endl;
                ++errors;
            }
        }

        ++received;
    }

    if (received != number_of_samples) {
        std::cerr
            << "Nombre de sorties incorrect : "
            << received << " au lieu de " << number_of_samples
            << std::endl;
        ++errors;
    }

    if (!last_seen) {
        std::cerr << "TLAST absent sur la sortie." << std::endl;
        ++errors;
    }

    if (!input_stream.empty()) {
        std::cerr << "Le flux d'entrée n'a pas été entièrement consommé."
                  << std::endl;
        ++errors;
    }

    if (errors == 0) {
        std::cout << "\nTEST PASSED" << std::endl;
        return 0;
    }

    std::cerr << "\nTEST FAILED - erreurs : " << errors << std::endl;
    return 1;
}

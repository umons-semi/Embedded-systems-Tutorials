#include "imagefiltering.h"

#define MAX_HEIGHT 256

int imagefiltering_compute(
    hls::stream<packet> &input,
    hls::stream<packet> &output,
    int &in_width,
    int &in_height,
    float kernel_data[3 * 3])
{
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS INTERFACE s_axilite port=in_width
#pragma HLS INTERFACE s_axilite port=in_height
#pragma HLS INTERFACE s_axilite port=kernel_data
#pragma HLS INTERFACE s_axilite port=return

    if (in_width <= 0 || in_width > MAX_WIDTH ||
        in_height <= 0 || in_height > MAX_HEIGHT)
    {
        return 0;
    }

    /*
     * Stockage complet de l'image en mémoire interne.
     * Cette version est volontairement simple et fiable pour valider
     * la C Simulation, la synthèse et le fonctionnement AXI Stream.
     */
    static data_type image[MAX_WIDTH * MAX_HEIGHT];
#pragma HLS BIND_STORAGE variable=image type=ram_2p impl=bram

    data_type kernel[3][3];
#pragma HLS ARRAY_PARTITION variable=kernel complete dim=0

load_kernel_y:
    for (int ky = 0; ky < 3; ++ky)
    {
#pragma HLS UNROLL
    load_kernel_x:
        for (int kx = 0; kx < 3; ++kx)
        {
#pragma HLS UNROLL
            kernel[ky][kx] = data_type(kernel_data[ky * 3 + kx]);
        }
    }

    const int pixel_count = in_width * in_height;

read_input:
    for (int i = 0; i < pixel_count; ++i)
    {
#pragma HLS PIPELINE II=1
        packet in_packet;
        input.read(in_packet);
        image[i] = data_type(in_packet.data);
    }

process_y:
    for (int y = 0; y < in_height; ++y)
    {
    process_x:
        for (int x = 0; x < in_width; ++x)
        {
#pragma HLS PIPELINE II=1

            data_type sum = 0;

        kernel_y:
            for (int ky = 0; ky < 3; ++ky)
            {
#pragma HLS UNROLL
            kernel_x:
                for (int kx = 0; kx < 3; ++kx)
                {
#pragma HLS UNROLL
                    const int px = x + kx - 1;
                    const int py = y + ky - 1;

                    data_type pixel = 0;

                    // Zero-padding sur les bords.
                    if (px >= 0 && px < in_width &&
                        py >= 0 && py < in_height)
                    {
                        pixel = image[py * in_width + px];
                    }

                    sum += pixel * kernel[ky][kx];
                }
            }

            const int index = y * in_width + x;

            packet out_packet;
            out_packet.data = int(sum);
            out_packet.keep = -1;
            out_packet.strb = -1;
            out_packet.last = (index == pixel_count - 1);

            output.write(out_packet);
        }
    }

    return 1;
}
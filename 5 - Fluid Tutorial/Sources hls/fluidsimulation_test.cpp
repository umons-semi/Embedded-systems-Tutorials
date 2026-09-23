#include <iostream>
#include <fstream>

#include "fluidsimulation.h"

int main()
{
    hls::stream<packet> s_out;

    std::cout << "Starting fluid simulation CSIM..." << std::endl;

    int frames = 100;

    for (int frame = 0; frame < frames; frame++)
    {
        std::cout << "Frame " << frame << std::endl;

        int frame_arg = frame;
        int ret = fluidsimulation_compute(s_out, frame_arg);

        std::cout << "Return value = " << ret << std::endl;

        int min_val = 2147483647;
        int max_val = -2147483648;

        std::ofstream img;

        if (frame == frames - 1)
        {
            img.open("fluidsimulation_output.pgm");
            img << "P2\n";
            img << SIZE << " " << SIZE << "\n";
            img << "255\n";
        }

        for (int y = 0; y < SIZE; y++)
        {
            for (int x = 0; x < SIZE; x++)
            {
                if (s_out.empty())
                {
                    std::cout << "ERROR: stream empty at "
                              << y << "," << x << std::endl;
                    return 1;
                }

                packet p = s_out.read();

                int pixel = p.data;

                if (pixel < min_val) min_val = pixel;
                if (pixel > max_val) max_val = pixel;

                int display_pixel = pixel;

                // Même logique que ton ancien testbench :
                // (out_packet.data + 255) / 2
                display_pixel = (display_pixel + 255) / 2;

                if (display_pixel < 0) display_pixel = 0;
                if (display_pixel > 255) display_pixel = 255;

                if (frame == frames - 1)
                {
                    img << display_pixel << " ";
                }

                if (y == SIZE - 1 && x == SIZE - 1)
                {
                    if (p.last != 1)
                    {
                        std::cout << "WARNING: TLAST missing on last pixel" << std::endl;
                    }
                }
            }

            if (frame == frames - 1)
            {
                img << "\n";
            }
        }

        if (frame == frames - 1)
        {
            img.close();
        }

        std::cout << "Min raw = " << min_val
                  << " | Max raw = " << max_val
                  << std::endl;
    }

    std::cout << "Saved fluidsimulation_output.pgm" << std::endl;
    std::cout << "CSIM finished successfully" << std::endl;

    return 0;
}
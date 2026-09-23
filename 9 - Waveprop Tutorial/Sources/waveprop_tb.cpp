#include <iostream>
#include <fstream>

#include "waveprop.h"

int main()
{
    hls::stream<packet> output_stream;

    std::cout << "Starting simulation..." << std::endl;

    int ret = waveprop_compute(output_stream);

    std::cout << "Return value = " << ret << std::endl;

    std::ofstream img("waveprop_output.pgm");

    if (!img.is_open())
    {
        std::cout << "Cannot create output file" << std::endl;
        return 1;
    }

    img << "P2\n";
    img << size << " " << size << "\n";
    img << "255\n";

    int min_val = 2147483647;
    int max_val = -2147483648;

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (output_stream.empty())
            {
                std::cout << "ERROR: stream empty at "
                          << y << "," << x << std::endl;
                return 1;
            }

            packet p = output_stream.read();

            int pixel = p.data;

            if (pixel < 0)
                pixel = 0;

            if (pixel > 255)
                pixel = 255;

            if (pixel < min_val)
                min_val = pixel;

            if (pixel > max_val)
                max_val = pixel;

            img << pixel << " ";
        }
        img << "\n";
    }

    img.close();

    std::cout << "Min value = " << min_val << std::endl;
    std::cout << "Max value = " << max_val << std::endl;

    std::cout << "Image saved as waveprop_output.pgm" << std::endl;

    return 0;
}
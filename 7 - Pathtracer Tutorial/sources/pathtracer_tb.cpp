#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "pathtracer.h"

int main()
{
    int width = 50;
    int height = 50;
    int samples_per_pixel = 2;

    hls::stream<packet> r_stream, g_stream, b_stream;

    pathtracer_compute(
        r_stream, g_stream, b_stream,
        width, height, samples_per_pixel
    );

    const int pixel_count = width * height;
    std::vector<unsigned char> image(pixel_count * 3);

    bool r_last = false, g_last = false, b_last = false;

    for (int i = 0; i < pixel_count; ++i)
    {
        if (r_stream.empty() || g_stream.empty() || b_stream.empty())
        {
            std::cerr << "ERREUR : flux vide au pixel " << i << std::endl;
            return 1;
        }

        packet rp, gp, bp;
        r_stream.read(rp);
        g_stream.read(gp);
        b_stream.read(bp);

        image[3*i + 0] = static_cast<unsigned char>(
            std::max(0, std::min(255, int(rp.data)))
        );
        image[3*i + 1] = static_cast<unsigned char>(
            std::max(0, std::min(255, int(gp.data)))
        );
        image[3*i + 2] = static_cast<unsigned char>(
            std::max(0, std::min(255, int(bp.data)))
        );

        if (rp.last) {
            if (i != pixel_count - 1) return 1;
            r_last = true;
        }
        if (gp.last) {
            if (i != pixel_count - 1) return 1;
            g_last = true;
        }
        if (bp.last) {
            if (i != pixel_count - 1) return 1;
            b_last = true;
        }
    }

    if (!r_last || !g_last || !b_last)
    {
        std::cerr << "ERREUR : TLAST absent." << std::endl;
        return 1;
    }

    FILE *f = fopen("pathtracer_output.ppm", "wb");
    if (!f)
    {
        std::cerr << "ERREUR : creation du fichier impossible." << std::endl;
        return 1;
    }

    fprintf(f, "P6\n%d %d\n255\n", width, height);

    for (int y = height - 1; y >= 0; --y)
        fwrite(&image[y * width * 3], 1, width * 3, f);

    fclose(f);

    std::cout << "C SIMULATION REUSSIE" << std::endl;
    std::cout << "Image : pathtracer_output.ppm" << std::endl;

    return 0;
}
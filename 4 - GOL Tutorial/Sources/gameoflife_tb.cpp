#include <iostream>
#include <stdio.h>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

extern "C"
{
    int gameoflife_compute(
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_out,
        int grid_width,
        int grid_height);
}

ap_uint<16> lfsr_random()
{
    static ap_uint<16> lfsr = 0xACE1u; // Initial seed value (non-zero)

    // Tap positions for a 16-bit LFSR with a maximal length sequence
    bool bit = lfsr[15] ^ lfsr[13] ^ lfsr[12] ^ lfsr[10];

    // Shift left by 1 and insert the new bit
    lfsr = (lfsr << 1) | bit;

    return lfsr;
}

float lfsr_uniform_random()
{
    ap_uint<16> r = lfsr_random();
    return r / 65536.0f;
}

static void initialize_grid(bool *grid, unsigned int width, unsigned int height)
{
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            float r = lfsr_uniform_random();
            grid[i * width + j] = (r < 0.2) ? 1 : 0;
        }
    }
}

void to_stream(const bool *data, int width, int height, hls::stream<ap_axis<32, 2, 5, 6>> &stream)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            ap_axis<32, 2, 5, 6> tmp;
            tmp.data = data[y * width + x];
            tmp.keep = -1;
            if (x == width - 1 && y == height - 1)
                tmp.last = true;
            else
                tmp.last = false;
            stream.write(tmp);
        }
    }
}

void from_stream(bool *data, int width, int height, hls::stream<ap_axis<32, 2, 5, 6>> &stream)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            ap_axis<32, 2, 5, 6> tmp;
            stream.read(tmp);
            data[y * width + x] = tmp.data;
        }
    }
}

int main()
{
    int width = 1024;
    int height = 1024;

    bool grid1[width * height];
    bool grid2[width * height];

    initialize_grid(grid1, width, height);

    bool *grid = grid1;
    bool *back_grid = grid2;

    hls::stream<ap_axis<32, 2, 5, 6>> stream_in;
    hls::stream<ap_axis<32, 2, 5, 6>> stream_out;

    for (int i = 0; i < 10; i++)
    {
        to_stream(grid, width, height, stream_in);
        gameoflife_compute(stream_in, stream_out, width, height);
        from_stream(back_grid, width, height, stream_out);
        // std::swap(grid, back_grid);
        bool *temp = grid;
        grid = back_grid;
        back_grid = temp;
    }

    unsigned char grid_out[width * height];
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            grid_out[i * width + j] = grid[i * width + j] ? 255 : 0;
        }
    }

    stbi_write_png("out.png", width, height, 1,
                   grid_out, width);

    return 0;
}
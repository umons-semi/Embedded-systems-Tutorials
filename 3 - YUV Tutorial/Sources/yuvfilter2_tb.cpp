#include <iostream>
#include <stdio.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

extern "C"
{
    void rgb2yuv_ip(
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_out);

    void scale_y_ip(
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_out,
        int scale_y);

    void yuv2rgb_ip(
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_out);
}

int main()
{
    const char *path = "C:/Users/LABO01/Tuto_YUV/Sources/input.jpg";
    int width;
    int height;
    int channels_file;

    unsigned char *rgb_in = stbi_load(path, &width, &height, &channels_file, /*req_channels=*/3);
    if (!rgb_in)
    {
        std::fprintf(stderr, "stb_image load error: %s\n", stbi_failure_reason());
        return 1;
    }

    hls::stream<ap_axis<32, 2, 5, 6>> rgb2yuv_stream_in;
    hls::stream<ap_axis<32, 2, 5, 6>> rgb2yuv_stream_out;
    hls::stream<ap_axis<32, 2, 5, 6>> scale_y_stream_in;
    hls::stream<ap_axis<32, 2, 5, 6>> scale_y_stream_out;
    hls::stream<ap_axis<32, 2, 5, 6>> yuv2rgb_stream_in;
    hls::stream<ap_axis<32, 2, 5, 6>> yuv2rgb_stream_out;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            ap_axis<32, 2, 5, 6> tmp;
            tmp.data.range(31, 24) = 0;
            tmp.data.range(23, 16) = rgb_in[y * width * 3 + x * 3 + 0];
            tmp.data.range(15, 8) = rgb_in[y * width * 3 + x * 3 + 1];
            tmp.data.range(7, 0) = rgb_in[y * width * 3 + x * 3 + 2];
            tmp.keep = 0xF;
            tmp.strb = 0xF;
            tmp.user = 0;
            tmp.id   = 0;
            tmp.dest = 0;

            tmp.last = (x == width - 1 && y == height - 1) ? 1 : 0;
            rgb2yuv_stream_in.write(tmp);
        }
    }
    
    std::fprintf(stdout, "STEP 2\n");

    rgb2yuv_ip(rgb2yuv_stream_in, rgb2yuv_stream_out);

    
    std::fprintf(stdout, "STEP 3\n");

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
        std::fprintf(stdout, "\t%d %d\n",y,x);
            ap_axis<32, 2, 5, 6> tmp;
            rgb2yuv_stream_out.read(tmp);
            scale_y_stream_in.write(tmp);
        }
    }

    std::fprintf(stdout, "STEP 4\n");

    scale_y_ip(scale_y_stream_in, scale_y_stream_out, 130);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            ap_axis<32, 2, 5, 6> tmp;
            scale_y_stream_out.read(tmp);
            yuv2rgb_stream_in.write(tmp);
        }
    }

    yuv2rgb_ip(yuv2rgb_stream_in, yuv2rgb_stream_out);

    unsigned char rgb_out[width * height * 3];

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            ap_axis<32, 2, 5, 6> tmp;
            yuv2rgb_stream_out.read(tmp);
            rgb_out[y * width * 3 + x * 3 + 0] = tmp.data.range(23, 16);
            rgb_out[y * width * 3 + x * 3 + 1] = tmp.data.range(15, 8);
            rgb_out[y * width * 3 + x * 3 + 2] = tmp.data.range(7, 0);
        }
    }

    stbi_write_png("out_test.png", width, height, 3,
                   rgb_out, width * 3);

    return 0;
}
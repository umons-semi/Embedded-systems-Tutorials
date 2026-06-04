#include "ap_axi_sdata.h"
#include "hls_stream.h"

static int clamp(int x, int min, int max)
{
   if (x < min)
      return min;
   if (x > max)
      return max;
   return x;
}


// Convert RGB image to Y'UV format
void rgb2yuv(ap_axis<32, 2, 5, 6> &data_in,
             ap_axis<32, 2, 5, 6> &data_out)
{
   int R = data_in.data.range(23, 16);
   int G = data_in.data.range(15, 8);
   int B = data_in.data.range(7, 0);

   int Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
   int U = (((-38) * R + (-74) * G + 112 * B + 128) >> 8) + 128;
   int V = ((122 * R + (-94) * G + (-18) * B + 128) >> 8) + 128;

   data_out.data.range(23, 16) = clamp(Y, 0, 255);
   data_out.data.range(15, 8) = clamp(U, 0, 255);
   data_out.data.range(7, 0) = clamp(V, 0, 255);
   data_out.keep = -1;
   data_out.strb = -1;
   data_out.last = data_in.last;
}

void scale_y(
    ap_axis<32, 2, 5, 6> &data_in,
    ap_axis<32, 2, 5, 6> &data_out,
    int Y_scale)
{
   int Y = data_in.data.range(23, 16);
   int U = data_in.data.range(15, 8);
   int V = data_in.data.range(7, 0);

   Y = clamp((Y * Y_scale) >> 7, 0, 255);

   data_out.data.range(23, 16) = Y;
   data_out.data.range(15, 8) = U;
   data_out.data.range(7, 0) = V;
   data_out.keep = -1;
   data_out.strb = -1;
   data_out.last = data_in.last;
}

// Convert YUV image to RGB format
void yuv2rgb(ap_axis<32, 2, 5, 6> &data_in,
             ap_axis<32, 2, 5, 6> &data_out)
{
   int Y = data_in.data.range(23, 16);
   int U = data_in.data.range(15, 8);
   int V = data_in.data.range(7, 0);

   int R = (298 * (Y - 16) + 409 * (V - 128) + 128) >> 8;
   int G = (298 * (Y - 16) + (-100) * (U - 128) + (-208) * (V - 128) + 128) >> 8;
   int B = (298 * (Y - 16) + 516 * (U - 128) + 128) >> 8;

   data_out.data.range(23, 16) = clamp(R, 0, 255);
   data_out.data.range(15, 8) = clamp(G, 0, 255);
   data_out.data.range(7, 0) = clamp(B, 0, 255);
   data_out.keep = -1;
   data_out.strb = -1;
   data_out.last = data_in.last;
}

extern "C"
{
    // Convert RGB image to Y'UV format
   void rgb2yuv_ip(
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_out)
   {
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE s_axilite port = return

      ap_axis<32, 2, 5, 6> data_in;
      data_in.last = false;
      while (!data_in.last)
      {
         stream_in.read(data_in);
         ap_axis<32, 2, 5, 6> data_yuv;
         rgb2yuv(data_in, data_yuv);
         stream_out.write(data_yuv);
      }
   }




   /*void rgb2yuv_ip(
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_out)
   {
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE s_axilite port = return

      ap_axis<32, 2, 5, 6> data_in;
      data_in.last = false;
      while (!data_in.last)
      {
         stream_in.read(data_in);
         ap_axis<32, 2, 5, 6> data_yuv;
         rgb2yuv(data_in, data_yuv);
         stream_out.write(data_yuv);
      }
   }*/

   void scale_y_ip(
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_out,
       int scale_Y)
   {
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE s_axilite port = scale_Y
#pragma HLS INTERFACE s_axilite port = return

      ap_axis<32, 2, 5, 6> data_in;
      data_in.last = false;
      while (!data_in.last)
      {
         stream_in.read(data_in);
         ap_axis<32, 2, 5, 6> data_scale;
         scale_y(data_in, data_scale, scale_Y);
         stream_out.write(data_scale);
      }
   }

   void yuv2rgb_ip(
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
       hls::stream<ap_axis<32, 2, 5, 6>> &stream_out)
   {
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE s_axilite port = return

      ap_axis<32, 2, 5, 6> data_in;
      data_in.last = false;
      while (!data_in.last)
      {
         stream_in.read(data_in);
         ap_axis<32, 2, 5, 6> data_out;
         yuv2rgb(data_in, data_out);
         stream_out.write(data_out);
      }
   }
}

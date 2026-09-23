#include "waveprop.h"

// Initialize wave amplitude matrices
static data_type u_past[size][size];    // Wave amplitude at time n-1
static data_type u_present[size][size]; // Wave amplitude at time n
static data_type u_future[size][size];  // Wave amplitude at time n+1
static int step = 0;

int waveprop_compute(hls::stream<packet> &output_stream)
{
#pragma HLS INTERFACE mode=s_axilite port=return
#pragma HLS INTERFACE mode=axis port=output_stream
    // Parameters
    float c = 343.0; // Speed of sound in air (m/s)
    float h = 1.0;   // Spatial step size (meters)

    // Stability condition (Courant–Friedrichs–Lewy condition)
    float safety_factor = 0.5;
    float delta_t = (h / c) * (1.0 / sqrt(2.0)) * safety_factor; // Time step size
    float s = c * delta_t / h;                                   // CFL number
    float s2 = s * s;

    // Initial condition: Gaussian pulse at the center
    float xc = (size - 1) * h / 2.0;
    float yc = (size - 1) * h / 2.0;
    float sigma = 5.0 * h;

    if (step == 0)
    {
    init_loop_i:
        for (int i = 0; i < size; ++i)
        {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100
            float x = i * h;
        init_loop_j:
            for (int j = 0; j < size; ++j)
            {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100
                float y = j * h;
                float value = hls::exp(-((x - xc) * (x - xc) + (y - yc) * (y - yc)) / (2.0 * sigma * sigma));
                u_present[i][j] = value;
                u_past[i][j] = value;
            }
        }
    }

update_loop_i:
    for (int i = 1; i < size - 1; ++i)
    {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100
    update_loop_j:
        for (int j = 1; j < size - 1; ++j)
        {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100

            if (j == 0 || j == size - 1 || i == 0 || i == size - 1)
            {
                u_future[i][j] = 0.0;
            }
            else
            {
                u_future[i][j] = data_type(2.0) * u_present[i][j] - u_past[i][j] +
                                 data_type(s2) * (u_present[i + 1][j] + u_present[i - 1][j] +
                                       u_present[i][j + 1] + u_present[i][j - 1] -
                                       data_type(4.0) * u_present[i][j]);
            }
        }
    }

    // Update the wave amplitude matrices for the next iteration
move_loop_i:
    for (int i = 0; i < size; ++i)
    {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100
    move_loop_j:
        for (int j = 0; j < size; ++j)
        {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100

            u_past[i][j] = u_present[i][j];
            u_present[i][j] = u_future[i][j];

            packet output;
            output.data = u_present[i][j]*255;
            output.keep = -1;
            output.strb = -1;
            // output.user = input_data.user;
            // output.last = input_data.last;
            // output.id = input_data.id;
            // output.dest = input_data.dest;

            if (i == size - 1 && j == size - 1)
                output.last = true;
            else
                output.last = false;

            output_stream.write(output);
        }
    }
    step++;

    return 1;
}

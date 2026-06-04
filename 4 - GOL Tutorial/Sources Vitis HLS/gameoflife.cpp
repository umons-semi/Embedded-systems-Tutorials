
#include "ap_axi_sdata.h"
#include "hls_stream.h"

template <typename T, int width>
class fifo_shiftreg_1
{
public:
    fifo_shiftreg_1()
    {
    }

    T shift(T new_data)
    {
        T last_data = data[width - 1];
    fifo_ff_loop:
        for (int i = width - 1; i > 0; i--)
        {
            data[i] = data[i - 1];
        }
        data[0] = new_data;
        return last_data;
    }

private:
    T data[width];
};

template <typename T, int max_width>
class fifo_shiftreg_2
{
public:
    fifo_shiftreg_2(int w)
    {
        width = w;
    }

    T shift(T new_data)
    {
        T last_data = data[width - 1];
    fifo_ff_loop:
        for (int i = max_width - 1; i > 0; i--)
        {
            data[i] = data[i - 1];
        }
        data[0] = new_data;
        return last_data;
    }

private:
    T data[max_width];
    int width;
};

template <typename T, int max_width>
class fifo_bram
{
public:
    fifo_bram(int w)
    {
        address = 0;
        width = w;
    }

    T shift(T new_data)
    {
        int write_address = address + width - 1;
        if (write_address > max_width - 1)
            write_address -= max_width;

        int read_address = address;

        T last_data = data[read_address];
        data[write_address] = new_data;

        address++;
        if (address >= max_width)
            address = 0;

        return last_data;
    }

private:
    T data[max_width];
    int address;
    int width;
};

extern "C"
{
    int gameoflife_compute(
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_in,
        hls::stream<ap_axis<32, 2, 5, 6>> &stream_out,
        int grid_width,
        int grid_height)
    {
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE s_axilite port = grid_width
#pragma HLS INTERFACE s_axilite port = grid_height
#pragma HLS INTERFACE s_axilite port = return

        int mat3[9];

        // fifo_shiftreg_1<int, 1024 - 1> line_1;
        // fifo_shiftreg_1<int, 1024 - 1> line_2;
        // fifo_shiftreg_1<int, 1024 - 1> line_3;

        // fifo_shiftreg_2<int, 1024> line_1(grid_width - 1);
        // fifo_shiftreg_2<int, 1024> line_2(grid_width - 1);
        // fifo_shiftreg_2<int, 1024> line_3(grid_width - 1);

        fifo_bram<int, 1024> line_1(grid_width - 1);
        fifo_bram<int, 1024> line_2(grid_width - 1);
        fifo_bram<int, 1024> line_3(grid_width - 1);

    gameoflife_y_loop:
        for (int y = -1; y <= grid_height + 3; y++)
        {
#pragma HLS loop_tripcount min = 1024 max = 1024 avg = 1024

        gameoflife_x_loop:
            for (int x = -1; x <= grid_width; x++)
            {
#pragma HLS loop_tripcount min = 1024 max = 1024 avg = 1024

                int data = 0;

                if (y >= 0 && x >= 0 && y < grid_height && x < grid_width)
                {
                    ap_axis<32, 2, 5, 6> package_in;
                    stream_in.read(package_in);
                    data = package_in.data;
                }

                int last_line_1 = line_1.shift(data);
                int last_line_2 = line_2.shift(mat3[2]);
                int last_line_3 = line_3.shift(mat3[5]);

                mat3[2] = mat3[1];
                mat3[1] = mat3[0];
                mat3[0] = last_line_1;

                mat3[5] = mat3[4];
                mat3[4] = mat3[3];
                mat3[3] = last_line_2;

                mat3[8] = mat3[7];
                mat3[7] = mat3[6];
                mat3[6] = last_line_3;

                // Compute the sum of the 8 neighbors
                int total = 0;
            compute_sum_loop:
                for (int i = 0; i < 9; i++)
                {
                    if (i == 4)
                        continue;
                    total += mat3[i];
                }

                int old_val = mat3[4];
                int new_val = old_val;
                // Apply Conway's Game of Life rules
                if (old_val == 1)
                {
                    if (total < 2 || total > 3)
                    {
                        new_val = 0;
                    }
                }
                else
                {
                    if (total == 3)
                    {
                        new_val = 1;
                    }
                }

                if (y >= 4 && x >= 1)
                {
                    ap_axis<32, 2, 5, 6> package_out;
                    package_out.data = new_val;
                    package_out.keep = -1;
                    package_out.strb = -1;
                    if (y == grid_height + 3 && x == grid_width)
                        package_out.last = 1;
                    else
                        package_out.last = 0;
                    stream_out.write(package_out);
                }
            }
        }

        return 0;
    }
}

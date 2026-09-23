#include "fluidsimulation.h"

static data_type u[SIZE][SIZE];
static data_type v[SIZE][SIZE];
static data_type u_prev[SIZE][SIZE];
static data_type v_prev[SIZE][SIZE];
static data_type dens[SIZE][SIZE];
static data_type dens_prev[SIZE][SIZE];
static data_type p[SIZE][SIZE];
static data_type div_[SIZE][SIZE];

void swap(data_type a[SIZE][SIZE], data_type b[SIZE][SIZE])
{
#pragma HLS INLINE off
    for (int y = 0; y < SIZE; y++)
    {
        for (int x = 0; x < SIZE; x++)
        {
            data_type temp = a[y][x];
            a[y][x] = b[y][x];
            b[y][x] = temp;
        }
    }
}

void set_bnd(int b, data_type x[SIZE][SIZE])
{
#pragma HLS INLINE off
set_bnd_loop:
    for (int i = 1; i <= N; i++)
    {
#pragma HLS PIPELINE off
        x[0][i] = (b == 1) ? data_type(-x[1][i]) : data_type(x[1][i]);
        x[N + 1][i] = (b == 1) ? data_type(-x[N][i]) : data_type(x[N][i]);
        x[i][0] = (b == 2) ? data_type(-x[i][1]) : data_type(x[i][1]);
        x[i][N + 1] = (b == 2) ? data_type(-x[i][N]) : data_type(x[i][N]);
    }

    x[0][0] = data_type(0.5) * (x[1][0] + x[0][1]);
    x[0][N + 1] = data_type(0.5) * (x[1][N + 1] + x[0][N]);
    x[N + 1][0] = data_type(0.5) * (x[N][0] + x[N + 1][1]);
    x[N + 1][N + 1] = data_type(0.5) * (x[N][N + 1] + x[N + 1][N]);
}

void add_source(data_type x[SIZE][SIZE], data_type s[SIZE][SIZE], float dt)
{
#pragma HLS INLINE off
add_source_i_loop:
    for (int i = 0; i < SIZE; i++)
    {
#pragma HLS PIPELINE off
    add_source_j_loop:
        for (int j = 0; j < SIZE; j++)
        {
#pragma HLS PIPELINE off
            x[i][j] += data_type(dt) * s[i][j];
        }
    }
}

void diffuse(int b, data_type x[SIZE][SIZE], data_type x0[SIZE][SIZE], float diff, float dt)
{
#pragma HLS INLINE off
    float a = dt * diff * N * N;
diffuse_k_loop:
    for (int k = 0; k < 20; k++)
    {
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN off
    diffuse_i_loop:
        for (int i = 1; i <= N; i++)
        {
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN off
        diffuse_j_loop:
            for (int j = 1; j <= N; j++)
            {
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN off
                x[i][j] = (x0[i][j] + data_type(a) * (x[i - 1][j] + x[i + 1][j] + x[i][j - 1] + x[i][j + 1])) / data_type(1 + 4 * a);
            }
        }
        set_bnd(b, x);
    }
}

void advect(int b, data_type d[SIZE][SIZE], data_type d0[SIZE][SIZE],
            data_type u[SIZE][SIZE], data_type v[SIZE][SIZE], float dt)
{
#pragma HLS INLINE off
    float dt0 = dt * N;
advect_i_loop:
    for (int i = 1; i <= N; i++)
    {
#pragma HLS PIPELINE off
    advect_j_loop:
        for (int j = 1; j <= N; j++)
        {
#pragma HLS PIPELINE off
            float x = i - data_type(dt0) * u[i][j];
            float y = j - data_type(dt0) * v[i][j];

            if (x < 0.5)
                x = 0.5;
            if (x > N + 0.5)
                x = N + 0.5;
            if (y < 0.5)
                y = 0.5;
            if (y > N + 0.5)
                y = N + 0.5;

            int i0 = (int)x;
            int i1 = i0 + 1;
            int j0 = (int)y;
            int j1 = j0 + 1;

            float s1 = x - i0;
            float s0 = 1 - s1;
            float t1 = y - j0;
            float t0 = 1 - t1;

            // clamp indices
            if (i0 < 0)
                i0 = 0;
            if (i0 > N + 1)
                i0 = N + 1;
            if (i1 < 0)
                i1 = 0;
            if (i1 > N + 1)
                i1 = N + 1;
            if (j0 < 0)
                j0 = 0;
            if (j0 > N + 1)
                j0 = N + 1;
            if (j1 < 0)
                j1 = 0;
            if (j1 > N + 1)
                j1 = N + 1;

            d[i][j] = data_type(s0) * (data_type(t0) * d0[i0][j0] + data_type(t1) * d0[i0][j1]) +
                      data_type(s1) * (data_type(t0) * d0[i1][j0] + data_type(t1) * d0[i1][j1]);
        }
    }
    set_bnd(b, d);
}

void project(data_type u[SIZE][SIZE], data_type v[SIZE][SIZE],
             data_type p[SIZE][SIZE], data_type div_[SIZE][SIZE])
{
#pragma HLS INLINE off
    project_i_loop:
    for (int i = 1; i <= N; i++)
    {
#pragma HLS UNROLL off
#pragma HLS PIPELINE off
        project_j_loop:
        for (int j = 1; j <= N; j++)
        {
#pragma HLS UNROLL off
#pragma HLS PIPELINE off
            div_[i][j] = data_type(-0.5) * ((u[i + 1][j] - u[i - 1][j]) + (v[i][j + 1] - v[i][j - 1])) / N;
            p[i][j] = 0;
        }
    }
    set_bnd(0, div_);
    set_bnd(0, p);

    project_k_loop:
    for (int k = 0; k < 50; k++)
    {
#pragma HLS PIPELINE off
#pragma HLS UNROLL off
        project_ki_loop:
        for (int i = 1; i <= N; i++)
        {
#pragma HLS PIPELINE off
#pragma HLS UNROLL off
            project_kj_loop:
            for (int j = 1; j <= N; j++)
            {
#pragma HLS PIPELINE off
#pragma HLS UNROLL off
                p[i][j] = (div_[i][j] + p[i - 1][j] + p[i + 1][j] + p[i][j - 1] + p[i][j + 1]) / data_type(4.0);
            }
        }
        set_bnd(0, p);
    }

    project_i2_loop:
    for (int i = 1; i <= N; i++)
    {
#pragma HLS PIPELINE off
#pragma HLS UNROLL off
        project_j2_loop:
        for (int j = 1; j <= N; j++)
        {
#pragma HLS PIPELINE off
#pragma HLS UNROLL off
            u[i][j] -= data_type(0.5) * N * (p[i + 1][j] - p[i - 1][j]);
            v[i][j] -= data_type(0.5) * N * (p[i][j + 1] - p[i][j - 1]);
        }
    }
    set_bnd(1, u);
    set_bnd(2, v);
}

void vel_step(data_type u[SIZE][SIZE], data_type v[SIZE][SIZE],
              data_type u0[SIZE][SIZE], data_type v0[SIZE][SIZE],
              float visc, float dt)
{
#pragma HLS INLINE off
    add_source(u, u0, dt);
    add_source(v, v0, dt);

    swap(u, u0);
    swap(v, v0);

    diffuse(1, u, u0, visc, dt);
    diffuse(2, v, v0, visc, dt);
    project(u, v, p, div_);

    swap(u, u0);
    swap(v, v0);

    advect(1, u, u0, u0, v0, dt);
    advect(2, v, v0, u0, v0, dt);
    project(u, v, p, div_);
}

void dens_step(data_type x[SIZE][SIZE], data_type x0[SIZE][SIZE],
               data_type u[SIZE][SIZE], data_type v[SIZE][SIZE],
               float diff, float dt)
{
#pragma HLS INLINE off
    add_source(x, x0, dt);

    swap(x, x0);

    diffuse(0, x, x0, diff, dt);

    swap(x, x0);

    advect(0, x, x0, u, v, dt);
}

// The compute function replicates the behavior of the Python compute function.
int fluidsimulation_compute(hls::stream<packet> &output_stream, int &frame)
{
#pragma HLS INTERFACE mode = s_axilite port = return
#pragma HLS INTERFACE mode = s_axilite port = frame
#pragma HLS INTERFACE mode = axis port = output_stream

    // Clear previous sources
    clear_i_loop:
    for (int i = 0; i < SIZE; i++)
    {
        clear_j_loop:
        for (int j = 0; j < SIZE; j++)
        {
            dens_prev[i][j] = 0.0;
            u_prev[i][j] = 0.0;
            v_prev[i][j] = 0.0;
        }
    }

    float t = frame * DT;
    int num_sources = 5;

    sources_loop:
    for (int s = 0; s < num_sources; s++)
    {
        float base_angle = 2.0 * M_PI * s / num_sources;
        float angle = base_angle + t * 0.1;
        int radius = N / 4;
        int cx = N / 2;
        int cy = N / 2;
        int x_pos = (int)(cx + radius * std::cos(angle));
        int y_pos = (int)(cy + radius * std::sin(angle));
        if (x_pos >= 1 && x_pos < N + 1 && y_pos >= 1 && y_pos < N + 1)
        {
            dens_prev[x_pos][y_pos] = 200.0;
            u_prev[x_pos][y_pos] = 50.0 * std::cos(angle);
            v_prev[x_pos][y_pos] = 50.0 * std::sin(angle);
        }
    }

    vel_step(u, v, u_prev, v_prev, VISC, DT);
    dens_step(dens, dens_prev, u, v, DIFF, DT);

write_i_loop:
    for (int i = 0; i < SIZE; ++i)
    {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100
    write_j_loop:
        for (int j = 0; j < SIZE; ++j)
        {
#pragma HLS LOOP_TRIPCOUNT max = 100 avg = 100 min = 100

            packet output;
            output.data = dens[i][j]*data_type(16.0);
            output.keep = -1;
            output.strb = -1;
            // output.user = input_data.user;
            // output.last = input_data.last;
            // output.id = input_data.id;
            // output.dest = input_data.dest;

            if (i == SIZE - 1 && j == SIZE - 1)
                output.last = true;
            else
                output.last = false;

            output_stream.write(output);
        }
    }

    return 0;
}
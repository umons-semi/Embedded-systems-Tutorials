#pragma once

#include "hls_math.h"

template <typename type>
struct mat3
{
    mat3()
    {
    }

    mat3(type d00, type d01, type d02, type d10, type d11, type d12, type d20, type d21, type d22)
    {
        data[0][0] = d00;
        data[0][1] = d01;
        data[0][2] = d02;
        data[1][0] = d10;
        data[1][1] = d11;
        data[1][2] = d12;
        data[2][0] = d20;
        data[2][1] = d21;
        data[2][2] = d22;
    }

    void zero()
    {
    mat3_zero_y_loop:
        for (int y = 0; y < 3; y++)
        {
        mat3_zero_x_loop:
            for (int x = 0; x < 3; x++)
            {
                data[y][x] = 0.0;
            }
        }
    }

    void identity()
    {
        zero();
        data[0][0] = 1.0;
        data[1][1] = 1.0;
        data[2][2] = 1.0;
    }

    mat3 transpose()
    {
        mat3<type> result;
        result(0, 0) = data[0][0];
        result(0, 1) = data[1][0];
        result(0, 2) = data[2][0];
        result(1, 0) = data[0][1];
        result(1, 1) = data[1][1];
        result(1, 2) = data[2][1];
        result(2, 0) = data[0][2];
        result(2, 1) = data[1][2];
        result(2, 2) = data[2][2];
        return result;
    }

    /*
        mat3 inverse()
        {
        double determinant =    +A(0,0)*(A(1,1)*A(2,2)-A(2,1)*A(1,2))
                            -A(0,1)*(A(1,0)*A(2,2)-A(1,2)*A(2,0))
                            +A(0,2)*(A(1,0)*A(2,1)-A(1,1)*A(2,0));
    double invdet = 1/determinant;
    result(0,0) =  (A(1,1)*A(2,2)-A(2,1)*A(1,2))*invdet;
    result(1,0) = -(A(0,1)*A(2,2)-A(0,2)*A(2,1))*invdet;
    result(2,0) =  (A(0,1)*A(1,2)-A(0,2)*A(1,1))*invdet;
    result(0,1) = -(A(1,0)*A(2,2)-A(1,2)*A(2,0))*invdet;
    result(1,1) =  (A(0,0)*A(2,2)-A(0,2)*A(2,0))*invdet;
    result(2,1) = -(A(0,0)*A(1,2)-A(1,0)*A(0,2))*invdet;
    result(0,2) =  (A(1,0)*A(2,1)-A(2,0)*A(1,1))*invdet;
    result(1,2) = -(A(0,0)*A(2,1)-A(2,0)*A(0,1))*invdet;
    result(2,2) =  (A(0,0)*A(1,1)-A(1,0)*A(0,1))*invdet;
        }
        */

    template <typename type2>
    mat3 operator/(type2 c)
    {
        mat3<type> result;

    mat3_div_y_loop:
        for (int y = 0; y < 3; y++)
        {
        mat3_div_x_loop:
            for (int x = 0; x < 3; x++)
            {
                result.data[y][x] = data[y][x] / c;
            }
        }

        return result;
    }

    template <typename type2>
    mat3 operator*(type2 c)
    {
        mat3<type> result;

    mat3_mult_y_loop:
        for (int y = 0; y < 3; y++)
        {
        mat3_mult_x_loop:
            for (int x = 0; x < 3; x++)
            {
                result.data[y][x] = data[y][x] * c;
            }
        }

        return result;
    }

    mat3 dot(mat3 c)
    {
        mat3<type> result;

    mat3_dot_y_loop:
        for (int y = 0; y < 3; y++)
        {
        mat3_dot_x_loop:
            for (int x = 0; x < 3; x++)
            {
                result.data[y][x] = data[y][0] * c(0, x) + data[y][1] * c(1, x) + data[y][2] * c(2, x);
            }
        }

        return result;
    }

    type mul_v1(mat3<type> c)
    {
        // #pragma HLS INLINE
        type result = 0.0;

    mat3_mul_v1_y_loop:
        for (int y = 0; y < 3; y++)
        {
        mat3_mul_v1_x_loop:
            for (int x = 0; x < 3; x++)
            {
                result += data[y][x] * c(y, x);
            }
        }

        return result;
    }

    type mul_v2(mat3<type> c)
    {
#pragma HLS INLINE

        type mul[3][3];
        // #pragma HLS ARRAY_PARTITION variable = mul complete dim = 0

    mat3_mul_v2_y_loop:
        for (int y = 0; y < 3; y++)
        {
#pragma HLS unroll
        mat3_mul_v2_x_loop:
            for (int x = 0; x < 3; x++)
            {
#pragma HLS unroll
                mul[y][x] = data[y][x] * c(y, x);
            }
        }

        type sum_lvl1_0 = mul[0][0] + mul[1][0];
        type sum_lvl1_1 = mul[0][1] + mul[1][1];
        type sum_lvl1_2 = mul[0][2] + mul[1][2];

        type sum_lvl2_0 = mul[2][0] + sum_lvl1_0;
        type sum_lvl2_1 = mul[2][1] + sum_lvl1_1;
        type sum_lvl2_2 = mul[2][2] + sum_lvl1_2;

        type sum_lvl3_0 = sum_lvl2_0 + sum_lvl2_1;

        type res = sum_lvl3_0 + sum_lvl2_2;

        return res;
    }

    type &operator()(int b, int c)
    {
        return data[b][c];
    }

    type operator()(int b, int c) const
    {
        return data[b][c];
    }

    type data[3][3];
};

template <typename type, int size>
class shift_register
{
public:
    shift_register()
    {
    }

    void shift_down(type val)
    {
#pragma HLS INLINE

    shift_down_loop:
        for (int i = 0; i < size - 1; i++)
        {
#pragma HLS UNROLL

            data[i] = data[i + 1];
        }

        data[size - 1] = val;
    }

    mat3<type> getMat3(int width)
    {
        mat3<type> mat;

    get_mat3_y_loop:
        for (int y = 0; y < 3; y++)
        {
        get_mat3_x_loop:
            for (int x = 0; x < 3; x++)
            {
                mat(y, x) = data[x + width * y];
            }
        }

        return mat;
    }

private:
    type data[size];
};

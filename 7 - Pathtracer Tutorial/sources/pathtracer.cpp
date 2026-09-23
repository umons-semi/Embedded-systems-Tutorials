#include "pathtracer.h"

ap_uint<16> lfsr_random()
{
#pragma HLS INLINE off
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

Vec3 random_in_hemisphere(const Vec3 &normal)
{
    Vec3 rand_dir = 1.0;
    
    rand_dir = Vec3(lfsr_uniform_random() * 2 - 1, lfsr_uniform_random() * 2 - 1, 1.0);
    /*
rand_loop:
    while (dot(rand_dir, rand_dir) >= 1.0)
    {
        #pragma HLS LOOP_TRIPCOUNT max = 10 avg = 10 min = 10

        rand_dir =
            Vec3(lfsr_uniform_random() * 2 - 1, lfsr_uniform_random() * 2 - 1,
                 lfsr_uniform_random() * 2 - 1);
    }
    */

    if (dot(rand_dir, normal) > 0.0)
        return normalize(rand_dir);
    else
        return -normalize(rand_dir);
}

Vec3 background(const Ray &ray)
{
    float t = 0.5 * (ray.direction.y + 1.0);
    return (1.0 - t) * Vec3(1.0, 1.0, 1.0) + t * Vec3(0.5, 0.7, 1.0);
}

Vec3 trace_ray(Ray ray, const Scene &scene, int max_depth = 5)
{
    Vec3 color(0.0, 0.0, 0.0);
    Vec3 attenuation(1.0, 1.0, 1.0);
trace_ray_loop:
    for (int i = 0; i < max_depth; ++i)
    {
#pragma HLS LOOP_TRIPCOUNT max = 5 avg = 5 min = 5
        float t;
        Sphere hit_object;
        if (!scene.intersect(ray, t, hit_object))
        {
            color += attenuation * background(ray);
            break;
        }
        else
        {
            Vec3 hit_point = ray.origin + t * ray.direction;
            Vec3 normal = normalize(hit_point - hit_object.center);
            Vec3 direction = random_in_hemisphere(normal);
            ray = Ray(hit_point + 1e-4 * normal, direction);
            attenuation = attenuation * hit_object.material.color;
        }
    }
    return color;
}

void pathtracer_compute(hls::stream<packet> &r_stream, hls::stream<packet> &g_stream,
            hls::stream<packet> &b_stream, int &width, int &height,
            int &samples_per_pixel)
{
#pragma HLS INTERFACE mode = s_axilite port = width
#pragma HLS INTERFACE mode = s_axilite port = height
#pragma HLS INTERFACE mode = s_axilite port = samples_per_pixel
#pragma HLS INTERFACE mode = s_axilite port = return
#pragma HLS INTERFACE mode = axis port = r_stream
#pragma HLS INTERFACE mode = axis port = g_stream
#pragma HLS INTERFACE mode = axis port = b_stream

    Scene scene;
    // Add a ground sphere
    Material ground_material(Vec3(0.8, 0.8, 0.0));
    scene.add(Sphere(Vec3(0, -100.5, -1), 100, ground_material));
    // Add a center sphere
    Material center_material(Vec3(0.7, 0.3, 0.3));
    scene.add(Sphere(Vec3(0, 0, -1), 0.5, center_material));
    // Add a right sphere
    Material right_material(Vec3(0.8, 0.6, 0.2));
    scene.add(Sphere(Vec3(1, 0, -1), 0.5, right_material));
    // Add a left sphere
    Material left_material(Vec3(0.1, 0.2, 0.5));
    scene.add(Sphere(Vec3(-1, 0, -1), 0.5, left_material));

    Vec3 camera_origin(0.0, 0.0, 0.0);
    float aspect_ratio = float(width) / height;
    float viewport_height = 2.0;
    float viewport_width = aspect_ratio * viewport_height;
    float focal_length = 1.0;

    Vec3 horizontal(viewport_width, 0, 0);
    Vec3 vertical(0, viewport_height, 0);
    Vec3 lower_left_corner =
        camera_origin - horizontal / 2 - vertical / 2 - Vec3(0, 0, focal_length);

    packet r_packet, g_packet, b_packet;
    r_packet.last = false;
    r_packet.keep = -1;
    r_packet.strb = -1;
    g_packet.last = false;
    g_packet.keep = -1;
    g_packet.strb = -1;
    b_packet.last = false;
    b_packet.keep = -1;
    b_packet.strb = -1;

height_loop:
    for (int j = 0; j < height; ++j)
    {
#pragma HLS LOOP_TRIPCOUNT max = 200 avg = 200 min = 200
    width_loop:
        for (int i = 0; i < width; ++i)
        {
#pragma HLS LOOP_TRIPCOUNT max = 200 avg = 200 min = 200
            Vec3 pixel_color(0.0, 0.0, 0.0);
        samples_loop:
            for (int s = 0; s < samples_per_pixel; ++s)
            {
#pragma HLS LOOP_TRIPCOUNT max = 10 avg = 10 min = 10

                float u = (i + lfsr_uniform_random()) / (width - 1);
                float v = (j + lfsr_uniform_random()) / (height - 1);
                // float u = (i + 0.5f) / (width - 1);
                // float v = (j + 0.5f) / (height - 1);
                Vec3 direction =
                    lower_left_corner + u * horizontal + v * vertical - camera_origin;
                Ray ray(camera_origin, direction);
                pixel_color += trace_ray(ray, scene);
            }
            pixel_color /= samples_per_pixel;
            // Gamma correction
            pixel_color = Vec3(hls::sqrt(pixel_color.x), hls::sqrt(pixel_color.y),
                               hls::sqrt(pixel_color.z));
            // Write the color to file
            int ir = static_cast<int>(255.999 *
                                      hls::min(hls::max(pixel_color.x, 0.0f), 1.0f));
            int ig = static_cast<int>(255.999 *
                                      hls::min(hls::max(pixel_color.y, 0.0f), 1.0f));
            int ib = static_cast<int>(255.999 *
                                      hls::min(hls::max(pixel_color.z, 0.0f), 1.0f));

            r_packet.data = ir;
            g_packet.data = ig;
            b_packet.data = ib;

            if (i == width - 1 && j == height - 1)
            {
                r_packet.last = true;
                g_packet.last = true;
                b_packet.last = true;
            }
            else
            {
                r_packet.last = false;
                g_packet.last = false;
                b_packet.last = false;
            }

            r_stream.write(r_packet);
            g_stream.write(g_packet);
            b_stream.write(b_packet);

            // r_buffer[i + j * width] = ir;
            // g_buffer[i + j * width] = ig;
            // b_buffer[i + j * width] = ib;
        }
    }
}

#pragma once

#include "hls_math.h"

#define MAX_OBJECTS 10

struct Vec3
{
    float x, y, z;

    // Constructors
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float a) : x(a), y(a), z(a) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Operator overloading
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 &operator+=(const Vec3 &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    Vec3 &operator*=(const float t)
    {
        x *= t;
        y *= t;
        z *= t;
        return *this;
    }
    Vec3 &operator/=(const float t) { return *this *= 1 / t; }
};

// Utility functions
inline Vec3 operator+(const Vec3 &u, const Vec3 &v)
{
    return Vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}
inline Vec3 operator-(const Vec3 &u, const Vec3 &v)
{
    return Vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}
inline Vec3 operator*(const Vec3 &u, const Vec3 &v)
{
    return Vec3(u.x * v.x, u.y * v.y, u.z * v.z);
}
inline Vec3 operator*(float t, const Vec3 &v)
{
    return Vec3(t * v.x, t * v.y, t * v.z);
}
inline Vec3 operator/(Vec3 v, float t) { return (1 / t) * v; }
inline float dot(const Vec3 &u, const Vec3 &v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z;
}
inline Vec3 normalize(Vec3 v) { return v / hls::sqrt(dot(v, v)); }

class Ray
{
public:
    Vec3 origin;
    Vec3 direction;

    Ray() {}
    Ray(const Vec3 &origin, const Vec3 &direction)
        : origin(origin), direction(normalize(direction)) {}
};

class Material
{
public:
    Vec3 color;

    Material() {}
    Material(const Vec3 &color) : color(color) {}
};

class Sphere
{
public:
    Vec3 center;
    float radius;
    Material material;
    bool used;

    Sphere() { used = false; }
    Sphere(const Vec3 &center, float radius, const Material &material)
        : center(center), radius(radius), material(material)
    {
        used = true;
    }

    bool intersect(const Ray &ray, float &t) const
    {
        Vec3 oc = ray.origin - center;
        float a = dot(ray.direction, ray.direction);
        float b = 2.0 * dot(oc, ray.direction);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0)
            return false;
        else
        {
            float sqrt_disc = std::sqrt(discriminant);
            float t1 = (-b - sqrt_disc) / (2.0 * a);
            float t2 = (-b + sqrt_disc) / (2.0 * a);
            if (t1 > 1e-4)
            {
                t = t1;
                return true;
            }
            if (t2 > 1e-4)
            {
                t = t2;
                return true;
            }
            return false;
        }
    }
};

class Scene
{
public:
    Sphere objects[MAX_OBJECTS];
    int lastObjectIndex;

    Scene() 
    { 
        lastObjectIndex = 0; 
    }

    void add(const Sphere &sphere)
    {
        if (lastObjectIndex >= MAX_OBJECTS)
            // cannot add any more objects
            return;
        objects[lastObjectIndex] = sphere;
        lastObjectIndex++;
    }

    bool intersect(const Ray &ray, float &t, Sphere &hit_object) const
    {
        bool hit_anything = false;
        float closest_t = 1e30;
        intersect_loop:
        for (const auto &object : objects)
        {
            #pragma HLS LOOP_TRIPCOUNT max=5 avg=5 min=5

            float temp_t;
            if (object.intersect(ray, temp_t))
            {
                if (temp_t < closest_t)
                {
                    closest_t = temp_t;
                    hit_object = object;
                    hit_anything = true;
                }
            }
        }
        if (hit_anything)
        {
            t = closest_t;
        }
        return hit_anything;
    }
};

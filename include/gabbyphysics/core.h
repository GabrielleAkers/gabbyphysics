#ifndef GABBYPHYSICS_CORE_H
#define GABBYPHYSICS_CORE_H

#include "precision.h"

namespace gabbyphysics
{
    class Vector3
    {
    public:
        real x;
        real y;
        real z;

    private:
        real pad;

    public:
        Vector3() : x(0), y(0), z(0) {}

        Vector3(const real x, const real y, const real z) : x(x), y(y), z(z) {}

        const static Vector3 GRAVITY;
        const static Vector3 NEGATIVE_GRAVITY;
        const static Vector3 UP;
        const static Vector3 RIGHT;
        const static Vector3 X;
        const static Vector3 Y;
        const static Vector3 Z;

        void operator+=(const Vector3 &v)
        {
            x += v.x;
            y += v.y;
            z += v.z;
        }

        void operator-=(const Vector3 &v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;
        }

        void operator*=(const real value)
        {
            x *= value;
            y *= value;
            z *= value;
        }

        // cross product
        void operator%=(const Vector3 &vector)
        {
            *this = vector_product(vector);
        }

        real operator*(const Vector3 &vector) const
        {
            return x * vector.x + y * vector.y + z * vector.z;
        }

        Vector3 operator+(const Vector3 &v) const
        {
            return Vector3(x + v.x, y + v.y, z + v.z);
        }

        Vector3 operator-(const Vector3 &v) const
        {
            return Vector3(x - v.x, y - v.y, z - v.z);
        }

        Vector3 operator*(const real value) const
        {
            return Vector3(x * value, y * value, z * value);
        }

        Vector3 operator%(const Vector3 &vector) const
        {
            return Vector3(y * vector.z - z * vector.y,
                           z * vector.x - x * vector.z,
                           x * vector.y - y * vector.x);
        }

        void component_product_update(const Vector3 &vector)
        {
            x *= vector.x;
            y *= vector.y;
            z *= vector.z;
        }

        // [a,b,c][x,y,z] = [ax,by,cz]
        Vector3 component_product(const Vector3 &vector) const
        {
            return Vector3(x * vector.x, y * vector.y, z * vector.z);
        }

        real dot_product(const Vector3 &vector) const
        {
            return x * vector.x + y * vector.y + z * vector.z;
        }

        Vector3 vector_product(const Vector3 &vector) const
        {
            return Vector3(y * vector.z - z * vector.y,
                           z * vector.x - x * vector.z,
                           x * vector.y - y * vector.x);
        }

        void add_scaled_vector(const Vector3 &vector, real scale)
        {
            x += vector.x * scale;
            y += vector.y * scale;
            z += vector.z * scale;
        }

        real magnitude() const
        {
            return real_sqrt(x * x + y * y + z * z);
        }

        real sqare_magnitude() const
        {
            return x * x + y * y + z * z;
        }

        void normalize()
        {
            real l = magnitude();
            if (l > 0)
            {
                (*this) *= ((real)1) / l;
            }
        }

        void invert()
        {
            x = -x;
            y = -y;
            z = -z;
        }

        void clear()
        {
            x = y = z = 0;
        }
    };
}

#endif // !GABBYPHYSICS_CORE_H

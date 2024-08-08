#ifndef WATER_H
#define WATER_H

#include "web.h"
#include "gabbyphysics/gabbyphysics.h"

using namespace std;

const static unsigned max_particles = 1024;

struct Color
{
    int r;
    int g;
    int b;
};

class Water : public gabbyphysics::Particle
{
private:
    bool active;
    Color color;

public:
    Water() : color(Color{0, 0, 255}), active(false) {}

    const bool is_active() const
    {
        return active;
    }

    void set_active(bool is_active)
    {
        active = is_active;
    }

    const Color get_color() const
    {
        return color;
    }

    void set_color(Color c)
    {
        color.r = c.r;
        color.g = c.g;
        color.b = c.b;
    }

    void set_color(int r, int g, int b)
    {
        color.r = r;
        color.g = g;
        color.b = b;
    }
};

#endif // !WATER_H

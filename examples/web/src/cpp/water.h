#ifndef WATER_H
#define WATER_H

#include "web.h"
#include "gabbyphysics/gabbyphysics.h"

class Water : public gabbyphysics::Particle
{
public:
    Water() {}
};

export extern "C" void browser_clear_canvas();
export extern "C" void browser_draw_particles(gabbyphysics::real x, gabbyphysics::real y);
export extern "C" void browser_log(const char *log);

#endif // !WATER_H

#ifndef WATER_H
#define WATER_H

#include "web.h"
#include "gabbyphysics/gabbyphysics.h"

using namespace std;

class Water : public gabbyphysics::Particle
{
private:
    bool active = false;

public:
    const bool is_active() const
    {
        return active;
    }

    void set_active(bool is_active)
    {
        active = is_active;
    }
};

#endif // !WATER_H

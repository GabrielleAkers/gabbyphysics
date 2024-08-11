#ifndef PARTICLESIM_H
#define PARTICLESIM_H

#include "web.h"
#include "gabbyphysics/gabbyphysics.h"

using namespace std;

class SimParticle : public gabbyphysics::Particle
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

#endif // !PARTICLESIM_H

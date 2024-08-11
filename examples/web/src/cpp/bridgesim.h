#include "gabbyphysics/gabbyphysics.h"

class BridgeSim
{
    gabbyphysics::ParticleWorld world;
    gabbyphysics::Particle *particle_array;
    gabbyphysics::GroundContacts ground_contact_generator;

    gabbyphysics::ParticleCable *cables;
    gabbyphysics::ParticleRod *rods;
    gabbyphysics::ParticleCableConstraint *cable_constraints;

    gabbyphysics::Vector3 ball_pos;
    gabbyphysics::Vector3 ball_display_pos;

    void update_ball();

public:
    BridgeSim();
    ~BridgeSim();

    void update(gabbyphysics::real duration);

    void display();

    void set_ball_pos(gabbyphysics::real x, gabbyphysics::real y);
};

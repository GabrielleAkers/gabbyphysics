#ifndef GABBYPHYSICS_PWORLD_H
#define GABBYPHYSICS_PWORLD_H

#include "vector"
#include "plinks.h"
#include "pfgen.h"

namespace gabbyphysics
{
    class ParticleWorld
    {
    public:
        typedef std::vector<gabbyphysics::Particle *> Particles;
        typedef std::vector<ParticleContactGenerator *> ContactGenerators;

    protected:
        Particles particles;
        bool calculate_iterations;
        ParticleForceRegistry registry;
        ParticleContactResolver resolver;
        ContactGenerators contact_generators;
        ParticleContact *contacts;
        unsigned max_contacts;

    public:
        // if no iterations provided then 2*max_contacts will be used
        ParticleWorld(unsigned max_contacts, unsigned iterations = 0);
        ~ParticleWorld();

        void start_frame();
        unsigned generate_contacts();
        void integrate(real duration);
        void run_physics(real duration);

        Particles &get_particles();
        ContactGenerators &get_contact_generators();
        ParticleForceRegistry &get_force_registry();
    };

    class GroundContacts : public gabbyphysics::ParticleContactGenerator
    {
        gabbyphysics::ParticleWorld::Particles *particles;
        gabbyphysics::real world_x;
        gabbyphysics::real world_y;

    public:
        void init(gabbyphysics::ParticleWorld::Particles *particles, gabbyphysics::real world_x, gabbyphysics::real world_y);

        virtual unsigned add_contact(gabbyphysics::ParticleContact *contact, unsigned limit) const;
    };
}

#endif // !GABBYPHYSICS_PWORLD_H

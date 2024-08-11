#ifndef GABBYPHYSICS_PCONTACTS_H
#define GABBYPHYSICS_PCONTACTS_H

#include "particle.h"

namespace gabbyphysics
{
    class ParticleContact
    {
    public:
        // second one can be null if only one object is involved (particle-scenery contact)
        Particle *particle[2];
        real restitution;
        // world coordinates
        Vector3 contact_normal;
        real penetration;

    public:
        void resolve(real duration);
        real calculate_separating_velocity() const;

    private:
        void resolve_velocity(real duration);
        void resolve_interpenetration(real duration);
    };

    class ParticleContactResolver
    {
    protected:
        unsigned iterations;
        unsigned iterations_used;

    public:
        ParticleContactResolver(unsigned iterations);

        void set_iterations(unsigned iterations);
        void resolve_contacts(ParticleContact *contact_array, unsigned num_contacts, real duration);
    };

    class ParticleContactGenerator
    {
    public:
        virtual unsigned add_contact(ParticleContact *contact, unsigned limit) const = 0;
    };
}

#endif // !GABBYPHYSICS_PCONTACTS_H

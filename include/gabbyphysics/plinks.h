#ifndef GABBYPHYSICS_PLINKS_H
#define GABBYPHYSICS_PLINKS_H

#include "pcontacts.h"

namespace gabbyphysics
{
    class ParticleLink : public ParticleContactGenerator
    {
    public:
        // pair of particles connected by the link
        Particle *particle[2];

    protected:
        real current_length() const;

    public:
        // contact should point to the first element of a contact array where limit is the
        // maximum number of contact in the array
        // returns the number of contacts that have been written
        virtual unsigned add_contact(ParticleContact *contact, unsigned limit) const = 0;
    };

    class ParticleCable : public ParticleLink
    {
    public:
        real max_length;
        // bounciness
        real restitution;

    public:
        virtual unsigned add_contact(ParticleContact *contact, unsigned limit) const;
    };

    class ParticleRod : public ParticleLink
    {
    public:
        real length;

    public:
        real current_length() const;
        virtual unsigned add_contact(ParticleContact *contact, unsigned limit) const;
    };

    // links a particle to immovable point
    class ParticleConstraint : public ParticleContactGenerator
    {
    public:
        Particle *particle;
        Vector3 anchor;

    protected:
        real current_length() const;

    public:
        virtual unsigned add_contact(ParticleContact *contact, unsigned limit) const = 0;
    };

    class ParticleCableConstraint : public ParticleConstraint
    {
    public:
        real max_length;
        real restitution;

    public:
        virtual unsigned add_contact(ParticleContact *contact, unsigned limit) const;
    };

    class ParticleRodConstraint : public ParticleConstraint
    {
    public:
        real length;

    public:
        virtual unsigned add_contact(ParticleContact *contact, unsigned limit) const;
    };
}

#endif // !GABBYPHYSICS_PLINKS_H

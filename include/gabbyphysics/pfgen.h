#ifndef GABBYPHYSICS_PFGEN_H
#define GABBYPHYSICS_PFGEN_H

#include "particle.h"
#include "precision.h"

#include "vector"

namespace gabbyphysics
{
    class ParticleForceGenerator
    {
    public:
        virtual void update_force(Particle *particle, real duration) = 0;
    };

    class ParticleForceRegistry
    {
    protected:
        struct ParticleForceRegistration
        {
            Particle *particle;
            ParticleForceGenerator *fg;
        };

        typedef std::vector<ParticleForceRegistration> Registry;
        Registry registrations;

    public:
        void add(Particle *particle, ParticleForceGenerator *fg);

        void remove(Particle *particle, ParticleForceGenerator *fg);

        void clear();

        void update_forces(real duration);
    };

    class ParticleGravity : public ParticleForceGenerator
    {
        Vector3 gravity;

    public:
        ParticleGravity(const Vector3 &gravity);

        virtual void update_force(Particle *particle, real duration);
    };

    class ParticleDrag : public ParticleForceGenerator
    {
        // velocity drag coeff
        real k1;
        // velocity^2 drag coeff
        real k2;

    public:
        ParticleDrag(real k1, real k2);

        virtual void update_force(Particle *particle, real duration);
    };

    class ParticleSpring : public ParticleForceGenerator
    {
        Particle *other;
        real spring_constant;
        real rest_length;

    public:
        ParticleSpring(Particle *other, real spring_constant, real rest_length);

        virtual void update_force(Particle *particle, real duration);
    };

    class ParticleAnchoredSpring : public ParticleForceGenerator
    {
        Vector3 *anchor;
        real spring_constant;
        real rest_length;

    public:
        ParticleAnchoredSpring(Vector3 *anchor, real spring_constant, real rest_length);

        virtual void update_force(Particle *particle, real duration);
    };

    class ParticleBungee : public ParticleForceGenerator
    {
        Particle *other;
        real spring_constant;
        real rest_length;

    public:
        ParticleBungee(Particle *other, real spring_constant, real rest_length);

        virtual void update_force(Particle *particle, real duration);
    };

    class ParticleBuoyancy : public ParticleForceGenerator
    {
        real max_depth;
        real volume;        // object volume
        real liquid_height; // height above y=0 parallel to XZ plane
        real liquid_density;

    public:
        ParticleBuoyancy(real max_depth, real volume, real liquid_height, real liquid_density = 1000.0f); // 1000.0 is density of water

        virtual void update_force(Particle *particle, real duration);
    };
}

#endif // !GABBYPHYSICS_PFGEN_H

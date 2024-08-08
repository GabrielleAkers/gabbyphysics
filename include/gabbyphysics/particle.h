#ifndef GABBYPHYSICS_PARTICLE_H
#define GABBYPHYSICS_PARTICLE_H

#include "core.h"

namespace gabbyphysics
{
    class Particle
    {
    protected:
        Vector3 position;
        Vector3 velocity;
        Vector3 acceleration;
        Vector3 force_accum;
        /**
         * [0, 1]: 0 means object stops without continuous force, 1 means no change in velocity
         * 0.995 is good to make an object 'look' like its not experiencing drag
         */
        real damping;
        // inverse_mass=0 implies infinite mass i.e. unmovable
        real inverse_mass;

    public:
        Particle() {}
        void set_position(const Vector3 &position);
        Vector3 get_position() const;
        void get_position(Vector3 *vec) const;
        void set_velocity(const Vector3 &velocity);
        void set_velocity(const real x, const real y, const real z);
        Vector3 get_velocity() const;
        void get_velocity(Vector3 *vec) const;
        void set_acceleration(const Vector3 &acceleration);
        Vector3 get_acceleration() const;
        void set_damping(const real damping);
        real get_damping() const;
        void set_mass(const real mass);
        real get_mass() const;
        void set_inverse_mass(const real inverse_mass);
        real get_inverse_mass() const;
        void integrate(real duration);
        void clear_accumulator();
        bool has_finite_mass() const;
        void add_force(const Vector3 &force);
    };
}

#endif // !GABBYPHYSICS_PARTICLE_H

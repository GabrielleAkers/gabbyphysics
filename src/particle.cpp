// #include "assert.h"
#include "gabbyphysics/particle.h"

using namespace gabbyphysics;

// moves the particle forward in time using newton's method
void Particle::integrate(real duration)
{
    // assert(duration > 0.0);
    if (duration == 0.0)
        return;

    position.add_scaled_vector(velocity, duration);

    Vector3 resulting_accel = acceleration;
    resulting_accel.add_scaled_vector(force_accum, inverse_mass);

    velocity.add_scaled_vector(resulting_accel, duration);

    velocity *= real_pow(damping, duration);
}

void Particle::set_position(const Vector3 &position)
{
    Particle::position = position;
}

Vector3 Particle::get_position() const
{
    return position;
}

void Particle::set_velocity(const Vector3 &velocity)
{
    Particle::velocity = velocity;
}

void Particle::set_velocity(const real x, const real y, const real z)
{
    velocity.x = x;
    velocity.y = y;
    velocity.z = z;
}

Vector3 Particle::get_velocity() const
{
    return velocity;
}

void Particle::set_acceleration(const Vector3 &acceleration)
{
    Particle::acceleration = acceleration;
}

Vector3 Particle::get_acceleration() const
{
    return acceleration;
}

void Particle::set_damping(const real damping)
{
    Particle::damping = damping;
}

real Particle::get_damping() const
{
    return damping;
}

void Particle::set_mass(const real mass)
{
    // assert(mass != 0);
    Particle::inverse_mass = ((real)1.0) / mass;
}

real Particle::get_mass() const
{
    if (inverse_mass == 0)
        return REAL_MAX;
    else
        return ((real)1.0) / inverse_mass;
}

void Particle::set_inverse_mass(const real inverse_mass)
{
    Particle::inverse_mass = inverse_mass;
}

real Particle::get_inverse_mass() const
{
    return inverse_mass;
}

void Particle::clear_accumulator()
{
    force_accum.clear();
}

void Particle::add_force(const Vector3 &force)
{
    force_accum += force;
}

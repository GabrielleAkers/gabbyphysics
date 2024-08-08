#include "gabbyphysics/pfgen.h"

using namespace gabbyphysics;

void ParticleForceRegistry::update_forces(real duration)
{
    Registry::iterator i = registrations.begin();
    for (; i != registrations.end(); i++)
    {
        i->fg->update_force(i->particle, duration);
    }
}

void ParticleForceRegistry::add(Particle *particle, ParticleForceGenerator *fg)
{
    ParticleForceRegistry::ParticleForceRegistration registration;
    registration.particle = particle;
    registration.fg = fg;
    registrations.push_back(registration);
}

ParticleGravity::ParticleGravity(const Vector3 &gravity) : gravity(gravity)
{
}

void ParticleGravity::update_force(Particle *particle, real duration)
{
    if (!particle->has_finite_mass())
        return;

    particle->add_force(gravity * particle->get_mass());
}

ParticleDrag::ParticleDrag(real k1, real k2) : k1(k1), k2(k2) {}

void ParticleDrag::update_force(Particle *particle, real duration)
{
    Vector3 force;
    particle->get_velocity(&force);

    real drag_coeff = force.magnitude();
    drag_coeff = k1 * drag_coeff + k2 * drag_coeff * drag_coeff;

    force.normalize();
    force *= -drag_coeff;
    particle->add_force(force);
}

ParticleSpring::ParticleSpring(Particle *other, real spring_constant, real rest_length)
    : other(other), spring_constant(spring_constant), rest_length(rest_length)
{
}

void ParticleSpring::update_force(Particle *particle, real duration)
{
    Vector3 force;
    particle->get_position(&force);
    force -= other->get_position();

    real magnitude = force.magnitude();
    magnitude = real_abs(magnitude - rest_length);
    magnitude *= spring_constant;

    force.normalize();
    force *= -magnitude;
    particle->add_force(force);
}

ParticleAnchoredSpring::ParticleAnchoredSpring(Vector3 *anchor, real spring_constant, real rest_length)
    : anchor(anchor), spring_constant(spring_constant), rest_length(rest_length)
{
}

void ParticleAnchoredSpring::update_force(Particle *particle, real duration)
{
    Vector3 force;
    particle->get_position(&force);
    force -= *anchor;

    real magnitude = force.magnitude();
    magnitude = real_abs(magnitude - rest_length);
    magnitude *= spring_constant;

    force.normalize();
    force *= -magnitude;
    particle->add_force(force);
}

ParticleBungee::ParticleBungee(Particle *other, real spring_constant, real rest_length)
    : other(other), spring_constant(spring_constant), rest_length(rest_length)
{
}

void ParticleBungee::update_force(Particle *particle, real duration)
{
    Vector3 force;
    particle->get_position(&force);
    force -= other->get_position();

    real magnitude = force.magnitude();
    if (magnitude <= rest_length)
        return;

    magnitude = spring_constant * (rest_length - magnitude);

    force.normalize();
    force *= -magnitude;
    particle->add_force(force);
}

ParticleBuoyancy::ParticleBuoyancy(real max_depth, real volume, real liquid_height, real liquid_density)
    : max_depth(max_depth), volume(volume), liquid_height(liquid_height), liquid_density(liquid_density)
{
}

void ParticleBuoyancy::update_force(Particle *particle, real duration)
{
    real depth = particle->get_position().y;
    if (depth >= liquid_height + max_depth)
        return;
    Vector3 force(0, 0, 0);

    // fully submerged
    if (depth <= liquid_height - max_depth)
    {
        force.y = liquid_density * volume;
        particle->add_force(force);
        return;
    }

    // partially submerged
    force.y = liquid_density * volume * (depth - max_depth - liquid_height) / 2 * max_depth;
    particle->add_force(force);
}

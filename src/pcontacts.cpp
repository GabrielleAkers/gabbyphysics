#include "gabbyphysics/pcontacts.h"

using namespace gabbyphysics;

void ParticleContact::resolve(real duration)
{
    resolve_velocity(duration);
    resolve_interpenetration(duration);
}

void ParticleContact::resolve_interpenetration(real duration)
{
    if (penetration <= 0)
        return;

    // correct is inversely proportional to mass so large objects get moved less than small ones when they collide
    real total_inverse_mass = particle[0]->get_inverse_mass();
    if (particle[1])
        total_inverse_mass += particle[1]->get_inverse_mass();

    if (total_inverse_mass <= 0)
        return;

    Vector3 move_per_inv_mass = contact_normal * (-penetration / total_inverse_mass);

    particle[0]->set_position(particle[0]->get_position() + move_per_inv_mass * particle[0]->get_inverse_mass());
    if (particle[1])
    {
        particle[1]->set_position(particle[1]->get_position() + move_per_inv_mass * particle[1]->get_inverse_mass());
    }
}

real ParticleContact::calculate_separating_velocity() const
{
    Vector3 relative_velocity = particle[0]->get_velocity();
    if (particle[1])
        relative_velocity -= particle[1]->get_velocity();
    return relative_velocity * contact_normal;
}

void ParticleContact::resolve_velocity(real duration)
{
    real separating_velocity = calculate_separating_velocity();

    if (separating_velocity > 0)
    {
        return;
    }

    real new_separating_velocity = -separating_velocity * restitution;

    // check velocity due to acceleration only to handle resting contact
    Vector3 accel_caused_velocity = particle[0]->get_acceleration();
    if (particle[1])
        accel_caused_velocity -= particle[1]->get_acceleration();
    real accel_caused_sep_velocity = accel_caused_velocity * contact_normal * duration;

    if (accel_caused_sep_velocity < 0)
    {
        new_separating_velocity += restitution * accel_caused_sep_velocity;

        if (new_separating_velocity < 0)
            new_separating_velocity = 0;
    }

    real delta_velocity = new_separating_velocity - separating_velocity;

    real total_inverse_mass = particle[0]->get_inverse_mass();
    if (particle[1])
        total_inverse_mass += particle[1]->get_inverse_mass();

    if (total_inverse_mass <= 0)
        return;

    real impulse = delta_velocity / total_inverse_mass;

    Vector3 impulse_per_invmass = contact_normal * impulse;

    particle[0]->set_velocity(particle[0]->get_velocity() + impulse_per_invmass * particle[0]->get_inverse_mass());
    if (particle[1])
    {
        particle[1]->set_velocity(particle[1]->get_velocity() + impulse_per_invmass * -particle[1]->get_inverse_mass());
    }
}

ParticleContactResolver::ParticleContactResolver(unsigned iterations) : iterations(iterations)
{
}

void ParticleContactResolver::resolve_contacts(ParticleContact *contact_array, unsigned num_contacts, real duration)
{
    iterations_used = 0;
    while (iterations_used < iterations)
    {
        // find largest closing velocity
        real max = 0;
        unsigned max_idx = num_contacts;
        for (unsigned i = 0; i < num_contacts; i++)
        {
            real sep_val = contact_array[i].calculate_separating_velocity();
            if (sep_val < max)
            {
                max = sep_val;
                max_idx = i;
            }
        }

        contact_array[max_idx].resolve(duration);
        iterations_used++;
    }
}

void ParticleContactResolver::set_iterations(unsigned iterations)
{
    ParticleContactResolver::iterations = iterations;
}

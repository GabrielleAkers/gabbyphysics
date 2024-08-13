#include "gabbyphysics/pworld.h"
#include "gabbyphysics/helper.h"

using namespace gabbyphysics;

ParticleWorld::ParticleWorld(unsigned max_contacts, unsigned iterations)
    : resolver(iterations), max_contacts(max_contacts)
{
    contacts = new ParticleContact[max_contacts];
    calculate_iterations = (iterations == 0);
}

ParticleWorld::~ParticleWorld()
{
    delete[] contacts;
}

void ParticleWorld::start_frame()
{
    for (Particles::iterator p = particles.begin();
         p != particles.end();
         p++)
    {
        (*p)->clear_accumulator();
    }
}

unsigned ParticleWorld::generate_contacts()
{
    unsigned limit = max_contacts;
    ParticleContact *next_contact = contacts;

    for (ContactGenerators::iterator g = contact_generators.begin();
         g != contact_generators.end();
         g++)
    {
        unsigned used = (*g)->add_contact(next_contact, limit);
        limit -= used;
        next_contact += used;

        if (limit <= 0)
            break;
    }

    return max_contacts - limit;
}

void ParticleWorld::integrate(real duration)
{
    for (Particles::iterator p = particles.begin();
         p != particles.end();
         p++)
    {
        (*p)->integrate(duration);
    }
}

void ParticleWorld::run_physics(real duration)
{
    registry.update_forces(duration);

    integrate(duration);

    unsigned used_contacts = generate_contacts();

    if (calculate_iterations)
    {
        resolver.set_iterations(used_contacts * 2);
    }
    resolver.resolve_contacts(contacts, used_contacts, duration);
}

ParticleWorld::Particles &ParticleWorld::get_particles()
{
    return particles;
}

ParticleWorld::ContactGenerators &ParticleWorld::get_contact_generators()
{
    return contact_generators;
}

ParticleForceRegistry &ParticleWorld::get_force_registry()
{
    return registry;
}

void GroundContacts::init(gabbyphysics::ParticleWorld::Particles *particles, gabbyphysics::real world_x, gabbyphysics::real world_y)
{
    GroundContacts::particles = particles;
    GroundContacts::world_x = world_x;
    GroundContacts::world_y = world_y;
}

unsigned GroundContacts::add_contact(gabbyphysics::ParticleContact *contact, unsigned limit) const
{
    unsigned count = 0;
    for (gabbyphysics::ParticleWorld::Particles::iterator p = particles->begin();
         p != particles->end();
         p++)
    {
        auto pp = (*p)->get_position();
        if (pp.y < 0.0f || pp.y > world_y)
        {
            contact->contact_normal = gabbyphysics::Vector3::UP;
            contact->particle[0] = *p;
            contact->particle[1] = NULL;
            contact->penetration = -pp.y;
            contact->restitution = 0.2f;
            contact++;
            count++;
        }

        if (pp.x < 0.0f || pp.x > world_x)
        {
            contact->contact_normal = gabbyphysics::Vector3::RIGHT;
            contact->particle[0] = *p;
            contact->particle[1] = NULL;
            contact->penetration = -pp.x;
            contact->restitution = 0.2f;
            contact++;
            count++;
        }

        if (count >= limit)
            return count;
    }
    return count;
}

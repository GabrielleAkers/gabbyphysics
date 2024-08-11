
#include "gabbyphysics/plinks.h"

using namespace gabbyphysics;

real ParticleLink::current_length() const
{
    Vector3 relative_pos = particle[0]->get_position() -
                           particle[1]->get_position();
    return relative_pos.magnitude();
}

// acts as a collision detector to generate a 'collision' when two particles connected by a cable move farther apart than the cable length
unsigned ParticleCable::add_contact(ParticleContact *contact, unsigned limit) const
{
    real length = current_length();

    if (length < max_length)
    {
        return 0;
    }

    contact->particle[0] = particle[0];
    contact->particle[1] = particle[1];

    Vector3 normal = particle[1]->get_position() - particle[0]->get_position();
    normal.normalize();
    contact->contact_normal = normal;

    contact->penetration = length - max_length;
    contact->restitution = restitution;

    return 1;
}

real ParticleRod::current_length() const
{
    return length;
}

unsigned ParticleRod::add_contact(ParticleContact *contact, unsigned limit) const
{
    real current_len = current_length();
    if (current_len == length)
    {
        return 0;
    }

    contact->particle[0] = particle[0];
    contact->particle[1] = particle[1];

    Vector3 normal = particle[1]->get_position() - particle[0]->get_position();
    normal.normalize();

    // extending vs depressing
    if (current_len > length)
    {
        contact->contact_normal = normal;
        contact->penetration = current_len - length;
    }
    else
    {
        contact->contact_normal = normal * -1;
        contact->penetration = length - current_len;
    }

    contact->restitution = 0;

    return 1;
}

real ParticleConstraint::current_length() const
{
    Vector3 relative_pos = particle->get_position() - anchor;
    return relative_pos.magnitude();
}

unsigned ParticleCableConstraint::add_contact(ParticleContact *contact, unsigned limit) const
{
    real length = current_length();

    if (length < max_length)
    {
        return 0;
    }

    contact->particle[0] = particle;
    contact->particle[1] = 0;

    Vector3 normal = anchor - particle->get_position();
    normal.normalize();
    contact->contact_normal = normal;

    contact->penetration = length - max_length;
    contact->restitution = restitution;

    return 1;
}

unsigned ParticleRodConstraint::add_contact(ParticleContact *contact, unsigned limit) const
{
    real current_len = current_length();

    if (current_len == length)
    {
        return 0;
    }

    contact->particle[0] = particle;
    contact->particle[1] = 0;

    Vector3 normal = anchor - particle->get_position();
    normal.normalize();

    if (current_len > length)
    {
        contact->contact_normal = normal;
        contact->penetration = current_len - length;
    }
    else
    {
        contact->contact_normal = normal * -1;
        contact->penetration = length - current_len;
    }

    contact->restitution = 0;

    return 1;
}

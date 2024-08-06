#include "water.h"
#include "string"
#include "list"

using namespace gabbyphysics;

const static unsigned max_particles = 1024;
real damping = 0.95;
real particle_radius = 5.0;
Vector3 gravity = Vector3::NEGATIVE_GRAVITY;

Water particles[max_particles];
unsigned next_particle = 0;

unsigned screenX = 0;
unsigned screenY = 0;

void create_particle(const Vector3 &position)
{
    Water *particle = particles + next_particle;

    particle->set_active(true);
    particle->set_position(position);
    particle->set_mass(1);
    particle->set_damping(damping);
    particle->set_acceleration(gravity);
    particle->clear_accumulator();

    next_particle = (next_particle + 1) % max_particles;

    if constexpr (dev_mode)
    {
        const auto pos = particle->get_position();
        browser_log(format_string("created particle at x=%f, y=%f", pos.x, pos.y).c_str());
    }
}

const real tiny_offset = 0.001;

extern "C"
{
    export void update_particles(const real duration)
    {
        if (duration < 0.0f)
            return;

        for (Water *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            const auto pp = p->get_position();
            const auto v = p->get_velocity();
            // bottom/top
            if ((pp.y >= screenY - particle_radius - tiny_offset) || (pp.y <= particle_radius + tiny_offset))
            {
                p->set_velocity(v.reflect(Vector3::UP));
            }
            // left/right
            if ((pp.x <= particle_radius + tiny_offset) || (pp.x >= screenX - particle_radius - tiny_offset))
            {
                p->set_velocity(v.reflect(Vector3::RIGHT));
            }
            p->integrate(duration);
        }
    }

    export void draw_particles()
    {
        browser_clear_canvas();
        for (Water *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            const Vector3 pos = p->get_position();
            browser_draw_particles(pos.x, pos.y);
        }
    }

    export void spawn_particle(const real x, const real y)
    {
        create_particle(Vector3(x, y, 0));
        browser_draw_particles(x, y);
    }

    export void reset_particles()
    {
        for (auto &p : particles)
        {
            p = Water();
        }
        browser_clear_canvas();
    }

    export void set_screen_width(const int x)
    {
        screenX = x;
        screenY = x;
    }

    export void set_damping(const real d)
    {
        if (damping < (real)0.0 || damping > (real)1.0)
            return;
        damping = d;
        for (Water *p = particles;
             p < particles + max_particles;
             p++)
        {
            p->set_damping(damping);
        }
    }

    export void set_particle_radius(const real r)
    {
        if (r <= (real)0.0)
            return;
        particle_radius = r;
        draw_particles();
    }

    export void set_gravity(const real x, const real y)
    {
        gravity.x = x;
        gravity.y = y;
        for (Water *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            p->set_acceleration(gravity);
        }
    }
}

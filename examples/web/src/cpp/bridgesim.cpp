#include "web.h"
#include "bridgesim.h"

using namespace gabbyphysics;

const static unsigned max_particles = 16;
const static unsigned max_contact_gens = 16;
unsigned next_particle = 0;
unsigned next_cable = 0;
unsigned next_rod = 0;
unsigned next_cable_constraint = 0;
unsigned next_rod_constraint = 0;

unsigned world_x;
unsigned world_y;

real damping = 0.9f;
Vector3 gravity = Vector3(gabbyphysics::Vector3::NEGATIVE_GRAVITY);

ParticleWorld world{max_particles * 10};
GroundContacts ground_contact_generator;
Particle particles[max_particles];
ParticleCable cable_array[max_contact_gens];
ParticleRod rod_array[max_contact_gens];
ParticleCableConstraint cable_constraint_array[max_contact_gens];
ParticleRodConstraint rod_constraint_array[max_contact_gens];

#define ROD_COUNT 6
#define CABLE_COUNT 10
#define SUPPORT_COUNT 12

#define BASE_MASS 1
#define EXTRA_MASS 10

#define PARTICLE_RADIUS 5

BridgeSim::BridgeSim() : world(max_particles * 10), cables(0), rods(0), cable_constraints(0), ball_pos(400, 400, 0)
{
    particle_array = new Particle[max_particles];

    for (unsigned i = 0; i < max_particles; i++)
    {
        world.get_particles().push_back(particle_array + i);
    }

    ground_contact_generator.init(&world.get_particles());
    world.get_contact_generators().push_back(&ground_contact_generator);

    for (unsigned i = 0; i < 12; i++)
    {
        unsigned x = (i % 12) / 2;
        particle_array[i].set_position(
            real(i / 2) * 500.0f - 200.0f,
            400,
            real(i % 2) * 2.0f - 1.0f);
        particle_array[i].set_velocity(0, 0, 0);
        particle_array[i].set_damping(damping);
        particle_array[i].set_acceleration(Vector3::NEGATIVE_GRAVITY);
        particle_array[i].clear_accumulator();
    }

    cables = new ParticleCable[CABLE_COUNT];
    for (unsigned i = 0; i < 10; i++)
    {
        cables[i].particle[0] = &particle_array[i];
        cables[i].particle[1] = &particle_array[i + 2];
        cables[i].max_length = 1.9f;
        cables[i].restitution = 0.3f;
        world.get_contact_generators().push_back(&cables[i]);
    }

    cable_constraints = new ParticleCableConstraint[SUPPORT_COUNT];
    for (unsigned i = 0; i < SUPPORT_COUNT; i++)
    {
        cable_constraints[i].particle = particle_array + i;
        cable_constraints[i].anchor = Vector3(
            real(i / 2) * 550.0f - 220.0f,
            600,
            real(i % 2) * 1.6f - 0.8f);
        if (i < 6)
            cable_constraints[i].max_length = real(i / 2) * 5.0f + 30.0f;
        else
            cable_constraints[i].max_length = 55.0f - real(i / 2) * 50.0f;
        cable_constraints[i].restitution = 0.5f;
        world.get_contact_generators().push_back(&cable_constraints[i]);
    }

    rods = new ParticleRod[ROD_COUNT];
    for (unsigned i = 0; i < 6; i++)
    {
        rods[i].particle[0] = &particle_array[i * 2];
        rods[i].particle[1] = &particle_array[i * 2 + 1];
        rods[i].length = 2;
        world.get_contact_generators().push_back(&rods[i]);
    }

    update_ball();
}

BridgeSim::~BridgeSim()
{
    if (cables)
        delete[] cables;
    if (rods)
        delete[] rods;
    if (cable_constraints)
        delete[] cable_constraints;
}

void BridgeSim::update_ball()
{
    for (unsigned i = 0; i < 12; i++)
    {
        particle_array[i].set_mass(BASE_MASS);
    }

    // Find the coordinates of the mass as an index and proportion
    int x = int(ball_pos.x);
    real xp = real_fmod(ball_pos.x, real(1.0f));
    if (x < 0)
    {
        x = 0;
        xp = 0;
    }
    if (x >= 5)
    {
        x = 5;
        xp = 0;
    }

    int z = int(ball_pos.z);
    real zp = real_fmod(ball_pos.z, real(1.0f));
    if (z < 0)
    {
        z = 0;
        zp = 0;
    }
    if (z >= 1)
    {
        z = 1;
        zp = 0;
    }

    // Calculate where to draw the mass
    ball_display_pos.clear();

    // Add the proportion to the correct masses
    particle_array[x * 2 + z].set_mass(BASE_MASS + EXTRA_MASS * (1 - xp) * (1 - zp));
    ball_display_pos.add_scaled_vector(
        particle_array[x * 2 + z].get_position(), (1 - xp) * (1 - zp));

    if (xp > 0)
    {
        particle_array[x * 2 + z + 2].set_mass(BASE_MASS + EXTRA_MASS * xp * (1 - zp));
        ball_display_pos.add_scaled_vector(
            particle_array[x * 2 + z + 2].get_position(), xp * (1 - zp));

        if (zp > 0)
        {
            particle_array[x * 2 + z + 3].set_mass(BASE_MASS + EXTRA_MASS * xp * zp);
            ball_display_pos.add_scaled_vector(
                particle_array[x * 2 + z + 3].get_position(), xp * zp);
        }
    }
    if (zp > 0)
    {
        particle_array[x * 2 + z + 1].set_mass(BASE_MASS + EXTRA_MASS * (1 - xp) * zp);
        ball_display_pos.add_scaled_vector(
            particle_array[x * 2 + z + 1].get_position(), (1 - xp) * zp);
    }
}

void BridgeSim::display()
{
    ParticleWorld::Particles &particles = world.get_particles();
    for (ParticleWorld::Particles::iterator p = particles.begin();
         p != particles.end();
         p++)
    {
        Particle *particle = *p;
        const Vector3 &pos = particle->get_position();
        browser_draw_point(pos.x, pos.y, PARTICLE_RADIUS, 255, 255, 255);
    }

    for (unsigned i = 0; i < ROD_COUNT; i++)
    {
        Particle **particles = rods[i].particle;
        const Vector3 &p0 = particles[0]->get_position();
        const Vector3 &p1 = particles[1]->get_position();
        browser_draw_line(p0.x, p0.y, p1.x, p1.y, 250, 0, 250);
    }

    for (unsigned i = 0; i < CABLE_COUNT; i++)
    {
        Particle **particles = cables[i].particle;
        const Vector3 &p0 = particles[0]->get_position();
        const Vector3 &p1 = particles[1]->get_position();
        browser_draw_line(p0.x, p0.y, p1.x, p1.y, 0, 250, 250);
    }

    for (unsigned i = 0; i < SUPPORT_COUNT; i++)
    {
        const Vector3 &p0 = cable_constraints[i].particle->get_position();
        const Vector3 &p1 = cable_constraints[i].anchor;
        browser_draw_line(p0.x, p0.y, p1.x, p1.y, 130, 130, 130);
    }

    browser_draw_point(ball_display_pos.x, ball_display_pos.y, 50, 255, 255, 255);
}

void BridgeSim::update(real duration)
{
    world.start_frame();

    if (duration <= 0.0f)
        return;

    // Run the simulation
    world.run_physics(duration);

    update_ball();

    browser_log("end update");
}

void BridgeSim::set_ball_pos(real x, real y)
{
    ball_pos = Vector3(x, y, 0);
}

BridgeSim *get_app()
{
    return new BridgeSim();
}

BridgeSim *app;

// this entrypoint is necessary because global object references arent held properly when compiling to wasm without it
int main()
{
    app = get_app();
    browser_log("main");
}

void init_sim(unsigned w, unsigned h)
{
    world_x = w;
    world_y = h;
    for (unsigned i = 0; i < max_particles; i++)
    {
        Particle *p = particles + i;
        p->set_acceleration(gravity);
        p->set_velocity(0, 0, 0);
        p->set_mass(1);
        p->clear_accumulator();
        world.get_particles().push_back(p);
    }

    for (unsigned i = 0; i < max_contact_gens; i++)
    {
        world.get_contact_generators().push_back(&cable_array[i]);
        world.get_contact_generators().push_back(&rod_array[i]);
        world.get_contact_generators().push_back(&cable_constraint_array[i]);
        world.get_contact_generators().push_back(&rod_constraint_array[i]);
    }

    ground_contact_generator.init(&world.get_particles());
    world.get_contact_generators().push_back(&ground_contact_generator);
}

void update(real duration)
{
    app->update(duration);
}

void display()
{
    browser_clear_canvas();
    app->display();
}

Particle *create_particle()
{
    Particle *p = particles + next_particle;
    p->set_velocity(0, 0, 0);
    p->set_damping(damping);
    p->set_acceleration(gravity);
    p->set_mass(1);
    p->clear_accumulator();
    world.get_particles().push_back(p);

    next_particle = (next_particle + 1) % max_particles;
    return p;
}

ParticleCable *create_cable()
{
    ParticleCable *c = cable_array + next_cable;
    next_cable = (next_cable + 1) % max_contact_gens;

    world.get_contact_generators().push_back(c);
    return c;
}

extern "C"
{
    export void init(unsigned w, unsigned h)
    {
        init_sim(w, h);
    }

    export void spawn_particle(const real x, const real y)
    {
        app->set_ball_pos(x, y);
    }

    export void create_cable(const real x1, const real y1, const real x2, const real y2)
    {
        Particle *p1 = create_particle();
        Particle *p2 = create_particle();
        p1->set_position(Vector3(x1, y1, 0));
        p2->set_position(Vector3(x2, y2, 0));
        browser_draw_point(x1, y1, PARTICLE_RADIUS, 250, 0, 250);
        browser_draw_point(x2, y2, PARTICLE_RADIUS, 250, 0, 250);

        ParticleCable *c = create_cable();
        c->particle[0] = p1;
        c->particle[1] = p2;
        c->max_length = (p2->get_position() - p2->get_position()).magnitude();
        c->restitution = 0.3f;

        browser_draw_line(x1, y1, x2, y2, 250, 0, 250);
    }

    export void set_screen_size(unsigned x, unsigned y)
    {
        world_x = x;
        world_y = y;
    }

    export void update_particles(const real duration)
    {
        update(duration);
    }

    export void draw_particles()
    {
        display();
    }
}

#include "watersim.h"
#include "web.h"

#include "cmath"
#include "stdlib.h"

using namespace gabbyphysics;

unsigned particle_radius = 5;
unsigned particle_spacing = 2;
unsigned particle_color[3]{0, 0, 250};

Vector3 default_gravity = Vector3::NEGATIVE_GRAVITY;
real default_damping = 0.95f;
real smoothing_radius = 15.0f;
real target_density = 1.0f;
real stiffness_coefficient = 10.0f;

WaterSim::WaterSim(unsigned num_particles, unsigned world_x, unsigned world_y, real (*kernel)(real radius, real distance))
    : num_particles(num_particles), world_x(world_x), world_y(world_y)
{
    particle_array = new Particle[num_particles];
    densities = new real[num_particles];
    spatial_lookup = new unsigned[num_particles];
    start_indices = new unsigned[num_particles];

    const int per_row = (int)sqrt(num_particles);
    const int per_col = (num_particles - 1) / per_row + 1;
    for (unsigned i = 0; i < num_particles; i++)
    {
        particle_array[i].set_position(
            world_x / 2 + (i % per_row - per_row / 2.0f + 0.5f) * 2 * particle_radius + particle_spacing,
            world_y / 2 + (i / per_row - per_col / 2.0f + 0.5f) * 2 * particle_radius + particle_spacing,
            0);
        particle_array[i]
            .set_velocity(0, 0, 0);
        particle_array[i].set_mass(1);
        particle_array[i].set_damping(default_damping);
        particle_array[i].set_acceleration(default_gravity);
        particle_array[i].clear_accumulator();
        browser_draw_point(particle_array[i].get_position().x, particle_array[i].get_position().y, particle_radius, particle_color[0], particle_color[1], particle_color[2]);
    }

    for (unsigned i = 0; i < num_particles; i++)
    {
        densities[i] = WaterSim::calculate_density(particle_array[i].get_position(), kernel);
    }
}

WaterSim::~WaterSim()
{
}

void WaterSim::set_gravity(Vector3 gravity)
{
    for (Particle *p = particle_array; p < particle_array + num_particles; p++)
    {
        p->set_acceleration(gravity);
    }
}

void WaterSim::set_damping(real damping)
{
    for (Particle *p = particle_array; p < particle_array + num_particles; p++)
    {
        p->set_damping(damping);
    }
}

typedef struct
{
    unsigned i;
    unsigned j;
} GridCoord;

GridCoord gridcoord_from_point(Vector3 point)
{
    return {
        (unsigned)floor(point.x / smoothing_radius),
        (unsigned)floor(point.y / smoothing_radius)};
}

unsigned hash_cell(unsigned i, unsigned j)
{
    return i * 92837111 + j + 689287499;
}

unsigned get_key_from_hash(unsigned hash, unsigned spatial_lookup_len)
{
    return hash % spatial_lookup_len;
}

int comparator(const void *p, const void *q)
{
    return (*(int *)p - *(int *)q);
}

void WaterSim::update_spatial_lookup(real radius)
{
    for (int i = 0; i < num_particles; i++)
    {
        auto coords = gridcoord_from_point(particle_array[i].get_position());
        auto cell_key = get_key_from_hash(hash_cell(coords.i, coords.j), num_particles);
        spatial_lookup[i] = cell_key;
        start_indices[i] = UINT32_MAX;
    }

    qsort(spatial_lookup, num_particles, sizeof(spatial_lookup[0]), comparator);

    for (int i = 0; i < num_particles; i++)
    {
        unsigned key = spatial_lookup[i];
        unsigned prev_key = i == 0 ? UINT32_MAX : spatial_lookup[i - 1];
        if (key != prev_key)
        {
            start_indices[key] = i;
        }
    }
}

void keep_in_box(Particle *p, unsigned world_x, unsigned world_y)
{
    const auto &pp = p->get_position();
    const auto &pv = p->get_velocity();
    if (pp.x < 0.0f)
    {
        p->set_position(particle_radius, pp.y, 0);
        p->set_velocity(-1 * pv.x, pv.y, 0);
    }
    if (pp.x > world_x)
    {
        p->set_position(world_x - particle_radius, pp.y, 0);
        p->set_velocity(-1 * pv.x, pv.y, 0);
    }

    if (pp.y < 0.0f)
    {
        p->set_position(pp.x, particle_radius, 0);
        p->set_velocity(pv.x, -1 * pv.y, 0);
    }
    if (pp.y > world_y)
    {
        p->set_position(pp.x, world_y - particle_radius, 0);
        p->set_velocity(pv.x, -1 * pv.y, 0);
    }
}

real WaterSim::calculate_density(Vector3 point, real (*kernel)(real radius, real distance))
{
    real density = 0;

    for (Particle *p = particle_array; p < particle_array + num_particles; p++)
    {
        real distance = (p->get_position() - point).magnitude();
        real influence = kernel(smoothing_radius, distance);
        density += p->get_mass() * influence;
    }

    return density;
}

real WaterSim::calculate_shared_pressure(real density_a, real density_b, real (*density_to_pressure)(real density))
{
    real pressure_a = density_to_pressure(density_a);
    real pressure_b = density_to_pressure(density_b);
    return (pressure_a + pressure_b) / 2;
}

Vector3 WaterSim::calculate_pressure_force(
    int particle_index,
    real (*kernel_derivative)(real radius, real distance),
    real (*density_to_pressure)(real density))
{
    Vector3 density_gradient = Vector3::ZERO;

    for (int other_index = 0; other_index < num_particles; other_index++)
    {
        if (particle_index == other_index)
            continue;

        Vector3 offset = particle_array[other_index].get_position() - particle_array[particle_index].get_position();
        real distance = offset.magnitude();
        Vector3 dir = distance == 0 ? Vector3::get_random() : offset * (real)(1.0 / distance);
        real slope = kernel_derivative(smoothing_radius, distance);
        real density = densities[other_index];
        real shared_pressure = calculate_shared_pressure(density, densities[particle_index], density_to_pressure);
        density_gradient += dir * (shared_pressure * slope * particle_array[other_index].get_mass() / density);
    }

    return density_gradient;
}

real smoothing_kernel(real radius, real distance)
{
    if (distance >= radius)
        return 0;
    real vol = (M_PI * real_pow(radius, 4)) / 6;
    return (radius - distance) * (radius - distance) / vol;
}

real smoothing_kernel_derivative(real radius, real distance)
{
    if (distance >= radius)
        return 0;
    real scale = 12 / (real_pow(radius, 4) * M_PI);
    return (distance - radius) * scale;
}

real convert_density_to_pressure(real density)
{
    real dp = density - target_density;
    return dp * stiffness_coefficient;
}

void WaterSim::spatial_based_pressure_step(Vector3 point, real radius, real duration)
{
    auto coords = gridcoord_from_point(point);
    real sqr_radius = radius * radius;

    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            unsigned key = get_key_from_hash(hash_cell(coords.i + i, coords.j + j), num_particles);
            int cell_start_idx = start_indices[key];

            for (int k = cell_start_idx; k < num_particles; k++)
            {
                if (spatial_lookup[k] != key)
                    break;

                real sqr_dist = (particle_array[k].get_position() - point).sqare_magnitude();

                if (sqr_dist <= sqr_radius)
                {
                    densities[k] = calculate_density(point, smoothing_kernel);

                    Vector3 pressure_force = calculate_pressure_force(k, smoothing_kernel_derivative, convert_density_to_pressure);
                    Vector3 pressure_acceleration = pressure_force * (real)(1.0 / densities[k]);
                    auto pv = particle_array[k].get_velocity();
                    pv.add_scaled_vector(pressure_acceleration, duration);
                    particle_array[k].set_velocity(pv);
                }
            }
        }
    }
}

void WaterSim::update(real duration)
{
    if (duration <= 0.0f)
        return;

    update_spatial_lookup(smoothing_radius);

    for (int i = 0; i < num_particles; i++)
    {
        auto pp = particle_array[i].get_position();
        auto pv = particle_array[i].get_velocity();

        spatial_based_pressure_step(pp, smoothing_radius, duration);
        // densities[i] = calculate_density(pp, smoothing_kernel);
        // Vector3 pressure_force = calculate_pressure_force(i, smoothing_kernel_derivative, convert_density_to_pressure);
        // Vector3 pressure_acceleration = pressure_force * (real)(1.0 / densities[i]);
        // pv.add_scaled_vector(pressure_acceleration, duration);
        // particle_array[i].set_velocity(pv);

        particle_array[i].integrate(duration);
        keep_in_box(&particle_array[i], world_x, world_y);
    }
}

void WaterSim::display()
{
    for (Particle *p = particle_array; p < particle_array + num_particles; p++)
    {
        const Vector3 &pp = p->get_position();

        browser_draw_point(pp.x, pp.y, particle_radius, particle_color[0], particle_color[1], particle_color[2]);
    }
}

WaterSim *get_sim(unsigned num_particles, unsigned world_x, unsigned world_y, real (*kernel)(real radius, real distance))
{
    return new WaterSim(num_particles, world_x, world_y, kernel);
}

WaterSim *sim;

int main()
{
    sim = get_sim(4000, 800, 800, smoothing_kernel);
}

extern "C"
{

    export void update_particles(const real duration)
    {
        sim->update(duration);
    }

    export void draw_particles()
    {
        browser_clear_canvas();
        sim->display();
    }

    export void set_gravity(const real x, const real y)
    {
        default_gravity = Vector3(x, y, 0);
        sim->set_gravity(default_gravity);
    }

    export void set_damping(real damping)
    {
        if (damping < (real)0.0 || damping > (real)1.0)
            return;

        default_damping = damping;
        sim->set_damping(default_damping);
    }

    export void set_particle_radius(real radius)
    {
        if (radius <= 0.0f)
            return;
        particle_radius = radius;
    }
}

#include "gabbyphysics/gabbyphysics.h"

class WaterSim
{
    gabbyphysics::Particle *particle_array;
    gabbyphysics::real *densities;

    unsigned *spatial_lookup;
    unsigned *start_indices;

    unsigned num_particles;
    unsigned world_x;
    unsigned world_y;

public:
    WaterSim(
        unsigned num_particles,
        unsigned world_x, unsigned world_y,
        gabbyphysics::real (*kernel)(gabbyphysics::real radius, gabbyphysics::real distance));
    ~WaterSim();

    void update(gabbyphysics::real duration);

    void display();

    void set_gravity(gabbyphysics::Vector3 gravity);

    void set_damping(gabbyphysics::real damping);

    void update_spatial_lookup(gabbyphysics::real radius);

    void spatial_based_pressure_step(gabbyphysics::Vector3 point, gabbyphysics::real radius, gabbyphysics::real duration);

    gabbyphysics::real calculate_density(
        gabbyphysics::Vector3 point,
        gabbyphysics::real (*kernel)(gabbyphysics::real radius, gabbyphysics::real distance));

    gabbyphysics::real calculate_shared_pressure(
        gabbyphysics::real density_a,
        gabbyphysics::real density_b,
        gabbyphysics::real (*density_to_pressure)(gabbyphysics::real density));

    gabbyphysics::Vector3 calculate_pressure_force(
        int particle_index,
        gabbyphysics::real (*kernel_derivative)(gabbyphysics::real radius, gabbyphysics::real distance),
        gabbyphysics::real (*density_to_pressure)(gabbyphysics::real density));
};

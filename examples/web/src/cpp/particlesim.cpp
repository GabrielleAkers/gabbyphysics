#include "particlesim.h"
#include "cell.h"

using namespace gabbyphysics;

unsigned screenX = 0;
unsigned screenY = 0;

const static unsigned max_particles = 1024;
real damping = 0.95;
real particle_radius = 5.0;
Vector3 gravity = Vector3::NEGATIVE_GRAVITY;

SimParticle particles[max_particles];
unsigned next_particle = 0;

const static unsigned num_cells = 40;
Cell grid[num_cells][num_cells];

real get_grid_h(const unsigned screen_y)
{
    return screen_y / num_cells;
}

struct GridCoord
{
    int i;
    int j;
};

const GridCoord get_grid_coords(const Vector3 &position)
{
    const real grid_h = get_grid_h(screenY);
    GridCoord coords;
    coords.i = floor(position.x / grid_h);
    coords.j = floor(position.y / grid_h);
    return coords;
}

void create_particle(const Vector3 &position)
{
    SimParticle *particle = particles + next_particle;

    particle->set_position(position);
    particle->set_mass(1);
    particle->set_damping(damping);
    particle->set_acceleration(gravity);
    particle->clear_accumulator();
    particle->set_active(true);

    next_particle = (next_particle + 1) % max_particles;

    if constexpr (dev_mode)
    {
        const auto pos = particle->get_position();
        browser_log(format_string("created particle at x=%f, y=%f", pos.x, pos.y).c_str());
    }
}

const real tiny_offset = 0.01;

extern "C"
{
    export void update_particles(const real duration)
    {
        if (duration < 0.0f)
            return;

        for (SimParticle *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            const auto pp = p->get_position();
            if (pp.y > screenY || pp.y < 0 || pp.x > screenX || pp.x < 0)
            {
                p->set_active(false);
                continue;
            }

            // check for solid collision TODO: make it good lol
            const auto coords_top = get_grid_coords(Vector3(pp.x, pp.y - particle_radius - tiny_offset, pp.z));
            const auto coords_bottom = get_grid_coords(Vector3(pp.x, pp.y + particle_radius + tiny_offset, pp.z));
            const auto coords_left = get_grid_coords(Vector3(pp.x - particle_radius - tiny_offset, pp.y, pp.z));
            const auto coords_right = get_grid_coords(Vector3(pp.x + particle_radius + tiny_offset, pp.y, pp.z));
            const auto cell_top = grid[coords_top.i][coords_top.j];
            const auto cell_bottom = grid[coords_bottom.i][coords_bottom.j];
            const auto cell_left = grid[coords_left.i][coords_left.j];
            const auto cell_right = grid[coords_right.i][coords_right.j];

            const auto v = p->get_velocity();
            if (cell_top.type == CellType(solid) || cell_bottom.type == CellType(solid))
            {
                p->set_velocity(v.reflect(Vector3::UP));
            }
            if (cell_left.type == CellType(solid) || cell_right.type == CellType(solid))
            {
                p->set_velocity(v.reflect(Vector3::RIGHT));
            }

            p->integrate(duration);
        }
    }

    export void draw_particles()
    {
        browser_clear_canvas();
        for (SimParticle *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            const Vector3 pos = p->get_position();
            browser_draw_point(pos.x, pos.y, 0, 0, 200);
        }
    }

    export void spawn_particle(const real x, const real y)
    {
        const auto coords = get_grid_coords(Vector3(x, y, 0));
        if (grid[coords.i][coords.j].type == CellType(solid))
            return;
        create_particle(Vector3(x, y, 0));
        browser_draw_point(x, y, 0, 0, 200);
    }

    export void reset_particles()
    {
        for (auto &p : particles)
        {
            p = SimParticle();
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
        for (SimParticle *p = particles;
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
        for (SimParticle *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            p->set_acceleration(gravity);
        }
    }

    export void init_grid()
    {
        const real grid_h = get_grid_h(screenY);

        for (int i = 0; i < num_cells; i++)
            for (int j = 0; j < num_cells; j++)
            {
                if (i == 0 || i == num_cells - 1)
                {
                    grid[i][j].type = CellType(solid);
                }
                else if (j == 0 || j == num_cells - 1)
                {
                    grid[i][j].type = CellType(solid);
                }
                else
                    grid[i][j].type = CellType(air);

                browser_draw_rect(i * grid_h, j * grid_h, grid[i][j].type, grid_h, grid_h);
            }
    }

    export void paint_wall(const real x, const real y)
    {
        const auto coords = get_grid_coords(Vector3(x, y, 0));
        const real grid_h = get_grid_h(screenY);
        grid[coords.i][coords.j].type = CellType(solid);
        browser_draw_rect(coords.i * grid_h, coords.j * grid_h, CellType(solid), grid_h, grid_h);
    }
}

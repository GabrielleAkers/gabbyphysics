#include "water.h"
#include "cell.h"
#include "web.h"

#include "cmath"
#include "tuple"

using namespace gabbyphysics;

unsigned screenX = 0;
unsigned screenY = 0;

real damping = 0.95;
real particle_radius = 5.0;
Vector3 gravity = Vector3::NEGATIVE_GRAVITY;

Water particles[max_particles];
unsigned num_particles = 0;
unsigned next_particle = 0;

const static unsigned num_cells = 40;
Cell grid[num_cells][num_cells];

int num_cell_particles[num_cells * num_cells];
int first_cell_particle[num_cells * num_cells + 1];
int cell_particle_ids[max_particles];

real density = 1000.0;
real particle_rest_density = 0.0;
real stiffness_coefficient = 1.0;
real overrelaxation = 1.5;

real target_framerate = 60.0;
real frametime = 1.0 / target_framerate;

real clamp(const real val, const real min, const real max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

real get_grid_h(const unsigned screen_y)
{
    return screen_y / num_cells;
}

// https://matthias-research.github.io/pages/tenMinutePhysics/18-flip.pdf for the method

struct GridCoord
{
    int i;
    int j;
};

GridCoord get_grid_coords(const Vector3 &position, const real grid_h, const real x_offset, const real y_offset)
{
    return GridCoord{(int)floor((position.x - x_offset) / grid_h), (int)floor((position.y - y_offset) / grid_h)};
}

struct CellDelta
{
    real dx;
    real dy;
};

CellDelta get_cell_delta(const Vector3 &position, const real grid_h, const real x_offset, const real y_offset)
{
    const auto coords = get_grid_coords(position, grid_h, x_offset, y_offset);
    return CellDelta{(position.x - x_offset) - coords.i * grid_h, (position.y - y_offset) - coords.j * grid_h};
}

CellDelta get_cell_delta(const Vector3 &position, const GridCoord &coords, const real grid_h, const real x_offset, const real y_offset)
{
    return CellDelta{(position.x - x_offset) - coords.i * grid_h, (position.y - y_offset) - coords.j * grid_h};
}

struct BilinearWeights
{
    real w1;
    real w2;
    real w3;
    real w4;
};

BilinearWeights get_bilinear_weights(const Vector3 &position, const real grid_h, const real x_offset, const real y_offset)
{
    const auto d = get_cell_delta(position, grid_h, x_offset, y_offset);
    return BilinearWeights{
        ((real)1.0 - d.dx / grid_h) * ((real)1.0 - d.dy / grid_h),
        (d.dx / grid_h) * ((real)1.0 - d.dy / grid_h),
        (d.dx / grid_h) * (d.dy / grid_h),
        ((real)1.0 - d.dx / grid_h) * (d.dy / grid_h)};
}

BilinearWeights get_bilinear_weights(const Vector3 &position, const GridCoord &coords, const real grid_h, const real x_offset, const real y_offset)
{
    const auto d = get_cell_delta(position, coords, grid_h, x_offset, y_offset);
    return BilinearWeights{
        ((real)1.0 - d.dx / grid_h) * ((real)1.0 - d.dy / grid_h),
        (d.dx / grid_h) * ((real)1.0 - d.dy / grid_h),
        (d.dx / grid_h) * (d.dy / grid_h),
        ((real)1.0 - d.dx / grid_h) * (d.dy / grid_h)};
}

void solve_incompressibility(const int num_iters = 100)
{
    for (int i = 0; i < num_cells; i++)
        for (int j = 0; j < num_cells; j++)
        {
            grid[i][j].p = (real)0.0;
            grid[i][j].prev_u = grid[i][j].u;
            grid[i][j].prev_v = grid[i][j].v;
        }

    const real grid_h = get_grid_h(screenY);
    const real cp = density * grid_h / frametime;

    for (int iter = 0; iter < num_iters; iter++)
    {
        for (int i = 1; i < num_cells - 1; i++)
            for (int j = 1; j < num_cells - 1; j++)
            {
                if (grid[i][j].type != CellType(fluid))
                    continue;

                // solve s
                const real left_s = grid[i - 1][j].s;
                const real right_s = grid[i + 1][j].s;
                const real top_s = grid[i][j - 1].s;
                const real bottom_s = grid[i][j + 1].s;
                const real s = left_s + right_s + top_s + bottom_s;
                if (s == (real)0) // surrounded by solid
                    continue;

                // solve divergence
                real d = grid[i + 1][j].u - grid[i][j].u + grid[i][j - 1].v - grid[i][j].v;

                if (particle_rest_density > (real)0.0)
                {
                    const real compression = grid[i][j].p - particle_rest_density;
                    if (compression > (real)0.0)
                    {
                        d -= stiffness_coefficient * compression;
                    }
                }

                real p = -d / s;
                p *= overrelaxation;
                grid[i][j].p += cp * p;

                grid[i][j].u -= left_s * p;
                grid[i + 1][j].u += right_s * p;
                grid[i][j].v -= bottom_s * p;
                grid[i][j - 1].v += top_s * p;
            }
    }
}

void push_particles_apart(const int num_iters = 2)
{
    for (int i = 0; i < num_cells * num_cells; i++)
    {
        num_cell_particles[i] = 0;
    }

    const real grid_h = get_grid_h(screenY);
    // count particles per cell
    for (Water *p = particles; p < particles + max_particles; p++)
    {
        if (!p->is_active())
            continue;
        const auto coords = get_grid_coords(p->get_position(), grid_h, 0.0, 0.0);
        num_cell_particles[coords.i * num_cells + coords.j] += 1;
    }

    unsigned first = 0;
    for (int i = 0; i < num_cells * num_cells; i++)
    {
        first += num_cell_particles[i];
        first_cell_particle[i] = first;
    }
    first_cell_particle[num_cells * num_cells] = first;

    // put particles in cells
    for (int i = 0; i < num_particles; i++)
    {
        Water *p = &particles[i];
        if (!p->is_active())
            continue;
        const auto coords = get_grid_coords(p->get_position(), grid_h, 0.0, 0.0);
        first_cell_particle[coords.i * num_cells + coords.j]--;
        cell_particle_ids[first_cell_particle[coords.i * num_cells + coords.j]] = i;
    }

    // push apart
    const real min_dist = (real)2.0 * particle_radius;
    const real min_dist2 = min_dist * min_dist;

    for (int iter = 0; iter < num_iters; iter++)
    {
        for (int i = 0; i < num_particles; i++)
        {
            Water *particle = &particles[i];
            if (!particle->is_active())
                continue;
            auto pp = particle->get_position();
            const auto coords = get_grid_coords(pp, grid_h, 0.0, 0.0);
            const int x0 = max(coords.i - 1, 0);
            const int y0 = max(coords.j - 1, 0);
            const int x1 = min(coords.i + 1, (int)num_cells - 1);
            const int y1 = min(coords.j + 1, (int)num_cells - 1);

            for (int xi = x0; xi <= x1; xi++)
            {
                for (int yi = y0; yi <= y1; yi++)
                {
                    const auto cell_n = xi * num_cells + yi;
                    const auto first = first_cell_particle[cell_n];
                    const auto last = first_cell_particle[cell_n + 1];
                    for (int j = first; j < last; j++)
                    {
                        const auto id = cell_particle_ids[j];
                        if (id == i)
                            continue;
                        Water *part = &particles[id];
                        auto qp = part->get_position();
                        auto dx = qp.x - pp.x;
                        auto dy = qp.y - pp.y;
                        const auto d2 = dx * dx + dy * dy;
                        if (d2 > min_dist2 || d2 == (real)0.0)
                            continue;
                        const auto d = real_sqrt(d2);
                        const auto s = (real)0.5 * (min_dist - d) / d;
                        dx *= s;
                        dy *= s;
                        particle->set_position(Vector3(pp.x - dx, pp.y - dy, pp.z));
                        part->set_position(Vector3(qp.x + dx, qp.y + dy, qp.z));
                    }
                }
            }
        }
    }
}

void update_particle_density()
{
    for (int i = 0; i < num_cells; i++)
        for (int j = 0; j < num_cells; j++)
        {
            grid[i][j].p = 0;
        }

    const auto grid_h = get_grid_h(screenY);
    for (Water *p = particles; p < particles + max_particles; p++)
    {
        if (!p->is_active())
            continue;
        const auto pp = p->get_position();

        const auto coords = get_grid_coords(pp, grid_h, grid_h / 2, grid_h / 2);
        const auto weights = get_bilinear_weights(pp, coords, grid_h, grid_h / 2, grid_h / 2);
        const auto i0 = coords.i;
        const auto i1 = min(i0 + 1, (int)num_cells - 2);
        const auto j0 = coords.j;
        const auto j1 = min(j0 + 1, (int)num_cells - 2);

        if (i0 < num_cells && j0 < num_cells)
            grid[i0][j0].p += weights.w1;
        if (i1 < num_cells && j0 < num_cells)
            grid[i1][j0].p += weights.w2;
        if (i1 < num_cells && j1 < num_cells)
            grid[i1][j1].p += weights.w3;
        if (i0 < num_cells && j1 < num_cells)
            grid[i0][j1].p += weights.w4;
    }

    if (particle_rest_density == (real)0.0)
    {
        real sum = 0.0;
        int num_fluid_cells = 0;
        for (int i = 0; i < num_cells; i++)
            for (int j = 0; j < num_cells; j++)
            {
                if (grid[i][j].type == CellType(fluid))
                {
                    sum += grid[i][j].p;
                    num_fluid_cells++;
                }
            }

        if (num_fluid_cells > 0)
            particle_rest_density = sum / num_fluid_cells;
    }
}

void transfer_velocities(bool to_grid, real flip_ratio = (real)0.0)
{
    const auto grid_h = get_grid_h(screenY);
    if (to_grid)
    {
        for (int i = 0; i < num_cells; i++)
            for (int j = 0; j < num_cells; j++)
            {
                grid[i][j].prev_u = grid[i][j].u;
                grid[i][j].prev_v = grid[i][j].v;
                grid[i][j].u = (real)0.0;
                grid[i][j].du = (real)0.0;
                grid[i][j].v = (real)0.0;
                grid[i][j].dv = (real)0.0;

                grid[i][j].type = grid[i][j].s == (real)0.0 ? CellType(solid) : CellType(air);
            }

        for (Water *p = particles; p < particles + max_particles; p++)
        {
            const auto coords = get_grid_coords(p->get_position(), grid_h, 0.0, 0.0);
            if (grid[coords.i][coords.j].type == CellType(air))
                grid[coords.i][coords.j].type = CellType(fluid);
        }
    }
    // // transfer u
    for (Water *p = particles; p < particles + max_particles; p++)
    {
        if (!p->is_active())
            continue;

        const auto pp = p->get_position();
        const auto coords = get_grid_coords(pp, grid_h, (real)0.0, grid_h / 2);
        const auto weights = get_bilinear_weights(pp, coords, grid_h, (real)0.0, grid_h / 2);
        const auto v = p->get_velocity();

        if (to_grid)
        {
            grid[coords.i][coords.j].u += v.x * weights.w1;
            grid[coords.i][coords.j].du += weights.w1;

            grid[coords.i + 1][coords.j].u += v.x * weights.w2;
            grid[coords.i + 1][coords.j].du += weights.w2;

            grid[coords.i + 1][coords.j + 1].u += v.x * weights.w3;
            grid[coords.i + 1][coords.j + 1].du += weights.w3;

            grid[coords.i][coords.j + 1].u += v.x * weights.w4;
            grid[coords.i][coords.j + 1].du += weights.w4;
        }
        else
        {
            const auto valid1 = grid[coords.i][coords.j].s;
            const auto valid2 = grid[coords.i + 1][coords.j].s;
            const auto valid3 = grid[coords.i + 1][coords.j + 1].s;
            const auto valid4 = grid[coords.i][coords.j + 1].s;

            const real w = valid1 * weights.w1 + valid2 * weights.w2 + valid3 * weights.w3 + valid4 * weights.w4;

            if (std::isnan(w))
            {
                debug_log("w is nan in transfer u");
                debug_log("pos %f, %f", pp.x, pp.y);
                debug_log("grid_h %f", grid_h);
                debug_log("grid_h/2 %f", grid_h / 2);
                debug_log("coords %i, %i", coords.i, coords.j);
                const auto delta = get_cell_delta(pp, coords, grid_h, (real)0.0, grid_h / 2);
                debug_log("cell deltas %f, %f", delta.dx, delta.dy);
                debug_log("weights %f, %f, %f, %f", weights.w1, weights.w2, weights.w3, weights.w4);
                debug_log("valid %f, %f, %f, %f", valid1, valid2, valid3, valid4);
            }
            if (w > (real)0.0 && !std::isnan(w))
            {
                const real pic_v = (valid1 * weights.w1 * grid[coords.i][coords.j].u +
                                    valid2 * weights.w2 * grid[coords.i + 1][coords.j].u +
                                    valid3 * weights.w3 * grid[coords.i + 1][coords.j + 1].u +
                                    valid4 * weights.w4 * grid[coords.i][coords.j + 1].u) /
                                   w;
                const real corr = (valid1 * weights.w1 * (grid[coords.i][coords.j].u - grid[coords.i][coords.j].prev_u) +
                                   valid2 * weights.w2 * (grid[coords.i + 1][coords.j].u - grid[coords.i + 1][coords.j].prev_u) +
                                   valid3 * weights.w3 * (grid[coords.i + 1][coords.j + 1].u - grid[coords.i + 1][coords.j + 1].prev_u) +
                                   valid4 * weights.w4 * (grid[coords.i][coords.j + 1].u - grid[coords.i][coords.j + 1].prev_u)) /
                                  w;
                const real flip_v = v.x + corr;

                real new_v = ((real)1.0 - flip_ratio) * pic_v + flip_ratio * flip_v;
                p->set_velocity(Vector3(new_v, v.y, v.z));
            }
        }
    }

    if (to_grid)
    {
        for (int i = 0; i < num_cells + 1; i++)
            for (int j = 0; j < num_cells + 1; j++)
            {
                if (grid[i][j].du > (real)0.0)
                    grid[i][j].u /= grid[i][j].du;

                if (grid[i][j].type == CellType(solid) || (i > 0 && grid[i - 1][j].type == CellType(solid)))
                    grid[i][j].u = grid[i][j].prev_u;
                if (grid[i][j].type == CellType(solid) || (j > 0 && grid[i][j - 1].type == CellType(solid)))
                    grid[i][j].v = grid[i][j].prev_v;
            }
    }
    // transfer v
    for (Water *p = particles; p < particles + max_particles; p++)
    {
        if (!p->is_active())
            continue;

        const auto pp = p->get_position();
        const auto coords = get_grid_coords(pp, grid_h, grid_h / 2.0, (real)0.0);
        const auto weights = get_bilinear_weights(pp, coords, grid_h, grid_h / 2.0, (real)0.0);
        const auto v = p->get_velocity();

        if (to_grid)
        {
            grid[coords.i][coords.j].v += v.y * weights.w1;
            grid[coords.i][coords.j].dv += weights.w1;

            grid[coords.i + 1][coords.j].v += v.y * weights.w2;
            grid[coords.i + 1][coords.j].dv += weights.w2;

            grid[coords.i + 1][coords.j + 1].v += v.y * weights.w3;
            grid[coords.i + 1][coords.j + 1].dv += weights.w3;

            grid[coords.i][coords.j + 1].v += v.y * weights.w4;
            grid[coords.i][coords.j + 1].dv += weights.w4;
        }
        else
        {
            const auto valid1 = grid[coords.i][coords.j].s;
            const auto valid2 = grid[coords.i + 1][coords.j].s;
            const auto valid3 = grid[coords.i + 1][coords.j + 1].s;
            const auto valid4 = grid[coords.i][coords.j + 1].s;

            const real w = valid1 * weights.w1 + valid2 * weights.w2 + valid3 * weights.w3 + valid4 * weights.w4;

            if (std::isnan(w))
            {
                debug_log("w is nan in transfer v");
            }
            // TODO: either w is NaN or pic_v/corr/flip_v is
            if (w > (real)0.0 && !std::isnan(w))
            {
                const real pic_v = (valid1 * weights.w1 * grid[coords.i][coords.j].v +
                                    valid2 * weights.w2 * grid[coords.i + 1][coords.j].v +
                                    valid3 * weights.w3 * grid[coords.i + 1][coords.j + 1].v +
                                    valid4 * weights.w4 * grid[coords.i][coords.j + 1].v) /
                                   w;
                const real corr = (valid1 * weights.w1 * (grid[coords.i][coords.j].v - grid[coords.i][coords.j].prev_v) +
                                   valid2 * weights.w2 * (grid[coords.i + 1][coords.j].v - grid[coords.i + 1][coords.j].prev_v) +
                                   valid3 * weights.w3 * (grid[coords.i + 1][coords.j + 1].v - grid[coords.i + 1][coords.j + 1].prev_v) +
                                   valid4 * weights.w4 * (grid[coords.i][coords.j + 1].v - grid[coords.i][coords.j + 1].prev_v)) /
                                  w;
                const real flip_v = v.y + corr;

                real new_v = ((real)1.0 - flip_ratio) * pic_v + flip_ratio * flip_v;
                p->set_velocity(Vector3(v.x, new_v, v.z));
            }
        }
    }
    if (to_grid)
    {
        for (int i = 0; i < num_cells; i++)
            for (int j = 0; j < num_cells; j++)
            {
                if (grid[i][j].dv > (real)0.0)
                    grid[i][j].v /= grid[i][j].dv;

                if (grid[i][j].type == CellType(solid) || (i > 0 && grid[i - 1][j].type == CellType(solid)))
                    grid[i][j].u = grid[i][j].prev_u;
                if (grid[i][j].type == CellType(solid) || (j > 0 && grid[i][j - 1].type == CellType(solid)))
                    grid[i][j].v = grid[i][j].prev_v;
            }
    }
}

const Water *create_particle(const Vector3 &position, bool debug = true)
{
    Water *particle = particles + next_particle;

    particle->set_position(position);
    particle->set_mass(1);
    particle->set_damping(damping);
    particle->set_acceleration(gravity);
    particle->clear_accumulator();
    particle->set_color(0, 0, 255);
    particle->set_active(true);

    next_particle = (next_particle + 1) % max_particles;
    num_particles++;
    num_particles = min(num_particles, max_particles);

    if (debug)
    {
        const auto pos = particle->get_position();
        debug_log("created particle at x=%f, y=%f", pos.x, pos.y);
        debug_log("num particles %i", num_particles);

        // offset grid sample points
        const real grid_h = get_grid_h(screenY);
        const real x_offset = (real)0.0;
        const real y_offset = grid_h / 2;
        const auto coords = get_grid_coords(pos, grid_h, x_offset, y_offset);
        const auto deltas = get_cell_delta(pos, coords, grid_h, x_offset, y_offset);
        const auto weights = get_bilinear_weights(pos, coords, grid_h, x_offset, y_offset);
        debug_log("coords %i, %i", coords.i, coords.j);
        debug_log("deltas %f, %f", deltas.dx, deltas.dy);
        debug_log("weights %f, %f, %f, %f", weights.w1, weights.w2, weights.w3, weights.w4);
    }
    return particle;
}

const real tiny_offset = 0.1;

extern "C"
{
    export void update_particles(const real duration)
    {
        if (duration < 0.0f)
            return;

        const real grid_h = get_grid_h(screenY);

        for (Water *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            const auto pp = p->get_position();
            if (pp.y > screenY || pp.y < 0 || pp.x > screenX || pp.x < 0)
            {
                p->set_active(false);
                num_particles--;
                continue;
            }

            // check for solid collision TODO: make it good lol
            const auto coords_top = get_grid_coords(Vector3(pp.x, pp.y - min(particle_radius, (real)2.0) - tiny_offset, pp.z), grid_h, 0.0, 0.0);
            const auto coords_bottom = get_grid_coords(Vector3(pp.x, pp.y + min(particle_radius, (real)2.0) + tiny_offset, pp.z), grid_h, 0.0, 0.0);
            const auto coords_left = get_grid_coords(Vector3(pp.x - min(particle_radius, (real)2.0) - tiny_offset, pp.y, pp.z), grid_h, 0.0, 0.0);
            const auto coords_right = get_grid_coords(Vector3(pp.x + min(particle_radius, (real)2.0) + tiny_offset, pp.y, pp.z), grid_h, 0.0, 0.0);
            const auto cell_top = grid[coords_top.i][coords_top.j];
            const auto cell_bottom = grid[coords_bottom.i][coords_bottom.j];
            const auto cell_left = grid[coords_left.i][coords_left.j];
            const auto cell_right = grid[coords_right.i][coords_right.j];

            auto v = p->get_velocity();
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

        push_particles_apart();
        transfer_velocities(true);
        update_particle_density();
        solve_incompressibility();
        transfer_velocities(false, (real)0.9);

        for (Water *p = particles;
             p < particles + max_particles;
             p++)
        {
            if (!p->is_active())
                continue;

            const auto pp = p->get_position();
            if (pp.y > screenY || pp.y < 0 || pp.x > screenX || pp.x < 0)
            {
                p->set_active(false);
                num_particles--;
                continue;
            }
            auto v = p->get_velocity();
            if (pp.y > screenY - particle_radius - tiny_offset || pp.y < particle_radius + tiny_offset || pp.x > screenX - particle_radius - tiny_offset || pp.x < particle_radius + tiny_offset)
            {
                v.invert();
                p->set_velocity(v);
            }
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
            const auto color = p->get_color();
            browser_draw_particles(pos.x, pos.y, color.r, color.g, color.b);
        }
    }

    export void spawn_particle(const real x, const real y)
    {
        const real grid_h = get_grid_h(screenY);
        const auto coords = get_grid_coords(Vector3(x, y, 0), grid_h, 0.0, 0.0);
        if (grid[coords.i][coords.j].type == CellType(solid))
            return;
        const Water *p = create_particle(Vector3(x, y, 0));
        const auto color = p->get_color();
        browser_draw_particles(x, y, color.r, color.g, color.b);
        // transfer_velocities(false, 0.9);
    }

    export void reset_particles()
    {
        for (auto &p : particles)
        {
            p = Water();
        }
        num_particles = 0;
        next_particle = 0;
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

                if (grid[i][j].type == CellType(solid))
                    grid[i][j].s = 0;
                else
                    grid[i][j].s = 1;

                grid[i][j].u = 0;
                grid[i][j].du = 0;
                grid[i][j].v = 0;
                grid[i][j].dv = 0;

                browser_draw_cell(i * grid_h, j * grid_h, grid[i][j].type, grid_h);
            }

        for (int i = 0; i < max_particles; i++)
        {
            const auto rangex = screenX - particle_radius - grid_h - (particle_radius + grid_h) + 1;
            const auto rangey = screenY - particle_radius - grid_h - (particle_radius + grid_h) + 1;
            const Water *p = create_particle(Vector3(std::rand() % (int)floor(rangex) + (particle_radius + grid_h),
                                                     std::rand() % (int)floor(rangey) + (particle_radius + grid_h),
                                                     0),
                                             false);
            const auto color = p->get_color();
            const auto pp = p->get_position();
            browser_draw_particles(pp.x, pp.y, color.r, color.g, color.b);
        }
    }

    export void paint_wall(const real x, const real y)
    {
        const real grid_h = get_grid_h(screenY);
        const auto coords = get_grid_coords(Vector3(x, y, 0), grid_h, 0.0, 0.0);
        grid[coords.i][coords.j].type = CellType(solid);
        grid[coords.i][coords.j].s = 0;
        browser_draw_cell(coords.i * grid_h, coords.j * grid_h, CellType(solid), grid_h);
    }
}

#ifndef WATER_H
#define WATER_H

#include "web.h"
#include "gabbyphysics/gabbyphysics.h"

using namespace std;

// browser provided functions
export extern "C"
{
    void browser_clear_canvas();
    void browser_draw_particles(gabbyphysics::real x, gabbyphysics::real y);
    void browser_log(const char *log);
}

struct GridCoord
{
    int i;
    int j;
};

struct ParticleCellDelta
{
    gabbyphysics::real delta_x;
    gabbyphysics::real delta_y;
};

struct ParticleCellWeights
{
    gabbyphysics::real w1;
    gabbyphysics::real w2;
    gabbyphysics::real w3;
    gabbyphysics::real w4;
};

class Water : public gabbyphysics::Particle
{
private:
    bool active = false;

public:
    const GridCoord get_grid_coords(const gabbyphysics::real grid_h) const
    {
        GridCoord coords;
        coords.i = floor(position.x / grid_h);
        coords.j = floor((position.y - (grid_h / 2)) / grid_h);
        return coords;
    }

    const ParticleCellDelta get_cell_deltas(const gabbyphysics::real grid_h) const
    {
        auto c = get_grid_coords(grid_h);
        ParticleCellDelta deltas;
        deltas.delta_x = position.x - (gabbyphysics::real)c.i * grid_h;
        deltas.delta_y = (position.y - (grid_h / 2)) - (gabbyphysics::real)c.j * grid_h;
        return deltas;
    }

    // relative amount each of the cell neighbors will contribute
    const ParticleCellWeights get_cell_weights(const unsigned grid_h) const
    {
        const auto deltas = get_cell_deltas(grid_h);
        // bilinear interpolation
        ParticleCellWeights weights;
        weights.w1 = (1 - deltas.delta_x / grid_h) * (1 - deltas.delta_y / grid_h);
        weights.w2 = (deltas.delta_x / grid_h) * (1 - deltas.delta_y / grid_h);
        weights.w3 = (deltas.delta_x / grid_h) * (deltas.delta_y / grid_h);
        weights.w4 = (1 - deltas.delta_x / grid_h) * (deltas.delta_y / grid_h);
        return weights;
    }

    const gabbyphysics::Vector3 get_q() const
    {
        return velocity;
    }

    void set_q(const gabbyphysics::Vector3 &q1, const gabbyphysics::Vector3 &q2, const gabbyphysics::Vector3 &q3, const gabbyphysics::Vector3 &q4, const ParticleCellWeights &weights)
    {
        const auto weighted_vel = q1 * weights.w1 + q2 * weights.w2 + q3 * weights.w3 + q4 * weights.w4;
        const auto sum_weights = weights.w1 + weights.w2 + weights.w3 + weights.w4;
        velocity = weighted_vel * ((gabbyphysics::real)1.0 / sum_weights);
    }

    const bool is_active() const
    {
        return active;
    }

    void set_active(bool is_active)
    {
        active = is_active;
    }
};

enum CellType
{
    wall = 0,
    air,
    water
};

struct Cell
{
    CellType type;
    gabbyphysics::Vector3 q;
    gabbyphysics::real r;
};

#endif // !WATER_H

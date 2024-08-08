#ifndef CELL_H
#define CELL_H

#include "gabbyphysics/gabbyphysics.h"
#include "water.h"

using namespace gabbyphysics;

enum CellType
{
    fluid = 0,
    air,
    solid
};

class Cell
{
public:
    CellType type;
    real u;  // left midpoint sample - velocity.x
    real du; // right midpoint sample - velocity.x
    real v;  // top midpoint sample - velocity.y
    real dv; // bottom midpoint sample - velocity.y
    real prev_u;
    real prev_v;
    real s; // 0 for solid objects 1 for air/fluid
    real p; // particle density

public:
    Cell() : u((real)0.0), du((real)0.0), v((real)0.0), dv((real)0.0), prev_u((real)0.0), prev_v((real)0.0), s((real)0.0), p((real)0.0) {}
};

#endif // !CELL_H

#ifndef CELL_H
#define CELL_H

#include "gabbyphysics/gabbyphysics.h"

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

public:
    Cell() {}
};

#endif // !CELL_H

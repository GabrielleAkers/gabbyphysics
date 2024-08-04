#include <web.h>

// extern "C" is so names dont get mangled
extern "C" export int add(int a, int b)
{
    return a + b;
}

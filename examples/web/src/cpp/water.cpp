#include "water.h"
#include "string"
#include "list"

using namespace gabbyphysics;

const static unsigned max_particles = 16;
real damping = 0.95;
real particle_radius = 10.0;

Water particles[max_particles];
unsigned next_particle = 0;

unsigned screenX = 0;
unsigned screenY = 0;

// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template <typename... Args>
std::string format_string(const std::string &format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

void create_particle(const Vector3 &position)
{
    Water *particle = particles + next_particle;

    particle->set_position(position);
    particle->set_mass(1);
    particle->set_damping(damping);
    particle->set_acceleration(Vector3::NEGATIVE_GRAVITY);
    particle->clear_accumulator();

    next_particle = (next_particle + 1) % max_particles;

    const auto pos = particle->get_position();
    browser_log(format_string("created particle at x=%f, y=%f", pos.x, pos.y).c_str());
}

extern "C"
{
    // https://github.com/WebAssembly/WASI/blob/main/legacy/application-abi.md and https://github.com/WebAssembly/wasi-libc/blob/main/libc-bottom-half/crt/crt1-reactor.c
    void __wasm_call_ctors(void);
    __attribute__((export_name("_initialize"))) void _initialize(void)
    {
        static volatile int initialized = 0;
        if (initialized != 0)
        {
            __builtin_trap();
        }
        initialized = 1;
        __wasm_call_ctors();
    }

    export void update_particles(const real duration)
    {
        if (duration < 0.0f)
            return;

        for (Water *p = particles;
             p < particles + max_particles;
             p++)
        {
            const auto pp = p->get_position();
            if (pp.y >= screenY - particle_radius)
            {
                auto v = p->get_velocity();
                v.invert();
                p->set_velocity(v);
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

    export void set_screen_size(const int x, const int y)
    {
        screenX = x;
        screenY = y;
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
}

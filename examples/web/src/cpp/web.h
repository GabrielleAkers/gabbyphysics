#ifndef WEB_H
#define WEB_H

#define export __attribute__((visibility("default")))

#include "gabbyphysics/gabbyphysics.h"

#ifdef DEV_MODE
constexpr bool dev_mode = true;
#else
constexpr bool dev_mode = false;
#endif

// browser provided functions
export extern "C"
{
    void browser_log(const char *log);
    void browser_clear_canvas();
    void browser_draw_point(gabbyphysics::real x, gabbyphysics::real y, gabbyphysics::real size, const int r, const int g, const int b);
    void browser_draw_rect(const int x, const int y, const int type, const int cell_w, const int cell_h);
    void browser_draw_line(gabbyphysics::real x1, gabbyphysics::real y1, gabbyphysics::real x2, gabbyphysics::real y2, const int r, const int g, const int b);
    void browser_draw_radial_gradient(
        gabbyphysics::real x1, gabbyphysics::real y1,
        const int inner_r, const int outer_r,
        const int r1, const int g1, const int b1,
        const int r2, const int g2, const int b2);
}

void debug_log(const char *log)
{
    if constexpr (dev_mode)
    {
        browser_log(log);
    }
}

template <typename... Args>
void debug_log(const std::string &format, Args... args)
{
    if constexpr (dev_mode)
    {
        browser_log(gabbyphysics::format_string(format, args...).c_str());
    }
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
}

#endif // !WEB_H

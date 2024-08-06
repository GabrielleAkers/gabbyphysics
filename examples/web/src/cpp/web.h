#ifndef WEB_H
#define WEB_H

#define export __attribute__((visibility("default")))

#ifdef DEV_MODE
constexpr bool dev_mode = true;
#else
constexpr bool dev_mode = false;
#endif

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

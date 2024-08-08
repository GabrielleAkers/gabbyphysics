# gabbyphysics

view examples here: https://gabrielleakers.github.io/gabbyphysics/examples/web/

based on the book [Game Physics Engine Development](http://www.r-5.org/files/books/computers/algo-list/realtime-3d/Ian_Millington-Game_Physics_Engine_Development-EN.pdf)

we compile to wasm using the wasi-sdk and use the browser as a gui

to get all this working i used [this stackexchange post](https://stackoverflow.com/questions/59587066/no-emscripten-how-to-compile-c-with-standard-library-to-webassembly), [this blog](https://michaelfranzl.github.io/clang-wasm-browser-starterpack/), and the [wasi-sdk docs](https://github.com/WebAssembly/wasi-sdk)

## build examples (particlesim)
```
make build IN=particlesim OUT=particlesim
```
or use the manual steps below
### fetch wasi-sdk
```
export WASI_VERSION=23
export WASI_VERSION_FULL=${WASI_VERSION}.0

wget "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}/wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz"

tar xvf wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz && rm wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz
```
### build wasm modules
```
export WASI_SDK_PATH=`pwd`/wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux

${WASI_SDK_PATH}/bin/clang++ \
  -nostartfiles \
  -fno-exceptions \
  -Wl,--no-entry \
  -Wl,--strip-all \
  -Wl,--export-dynamic \
  -Wl,--import-memory \
  -Wl,--allow-undefined \
  -fvisibility=hidden \
  -Oz \
  --sysroot ${WASI_SDK_PATH}/share/wasi-sysroot \
  -I include \
  -o examples/web/out/particlesim.wasm \
  src/*.cpp \
  examples/web/src/cpp/particlesim.cpp
```
## serve
```
cd examples/web
npm i
npm run dev
```

go to http://localhost:3000 to view

to view human readable form of the compiled and linked example wasm modules
```
export WABT_VERSION=1.0.36
wget "https://github.com/WebAssembly/wabt/releases/download/${WABT_VERSION}/wabt-${WABT_VERSION}-ubuntu-20.04.tar.gz"

tar xvf wabt-${WABT_VERSION}-ubuntu-20.04.tar.gz && rm wabt-${WABT_VERSION}-ubuntu-20.04.tar.gz

wabt-${WABT_VERSION}/bin/wasm2wat examples/web/out/main.wasm -o examples/web/out/main.wat
```
there is also an online version of wasm2wat [here](https://webassembly.github.io/wabt/demo/wasm2wat/)

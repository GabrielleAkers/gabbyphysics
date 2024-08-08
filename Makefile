WASI_VERSION=23
WASI_VERSION_FULL=${WASI_VERSION}.0
WASI_SDK_PATH=${shell pwd}/wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux

# pass DEV_MODE="" to disable
DEV_MODE ?= -DDEV_MODE

SRC := ${wildcard examples/web/src/cpp/*.cpp}
WASM := ${patsubst %.cpp,%.wasm,${patsubst examples/web/src/cpp/%, examples/web/out/%, ${SRC}}}

build : ${WASM}
	@echo built

examples/web/out/%.wasm : examples/web/src/cpp/%.cpp ${WASI_SDK_PATH}
	@echo building $@

	@${WASI_SDK_PATH}/bin/clang++ \
	${DEV_MODE} \
	-nostartfiles \
	-flto \
	-fvisibility=hidden \
	-Oz \
	-fno-exceptions \
	-Wl,--no-entry \
	-Wl,--strip-all \
	-Wl,--export-dynamic \
	-Wl,--import-memory \
	-Wl,--allow-undefined \
	--sysroot ${WASI_SDK_PATH}/share/wasi-sysroot \
	-I include \
	-o $@ \
	src/*.cpp \
	$<


${WASI_SDK_PATH}:
	wget "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}/wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz"
	tar xvf wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz && rm wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz

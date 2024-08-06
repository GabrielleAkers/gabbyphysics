WASI_VERSION=23
WASI_VERSION_FULL=${WASI_VERSION}.0
WASI_SDK_PATH=${shell pwd}/wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux

# pass DEV_MODE="" to disable
DEV_MODE ?= -DDEV_MODE

build : ${WASI_SDK_PATH}
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
	-o examples/web/out/main.wasm \
	src/*.cpp \
	examples/web/src/cpp/water.cpp
	
	@echo "built"

${WASI_SDK_PATH}:
	wget "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}/wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz"
	tar xvf wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz && rm wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz

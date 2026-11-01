BUILD_TYPE ?= Debug
CONAN_PROFILE ?= default
JOBS ?= 8
BUILD_DIR := build/$(BUILD_TYPE)

.PHONY: all configure build test sandbox clean

all: build

configure:
	conan install . --build=missing -pr:a "$(CONAN_PROFILE)" -s compiler.cppstd=23 -s build_type=$(BUILD_TYPE)
	cmake -S . -B "$(BUILD_DIR)" \
		-DCMAKE_TOOLCHAIN_FILE="$(CURDIR)/$(BUILD_DIR)/generators/conan_toolchain.cmake" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DKCORE_BUILD_TESTS=ON -DKCORE_BUILD_SANDBOX=ON

build: configure
	cmake --build "$(BUILD_DIR)" --parallel $(JOBS)

test: build
	ctest --test-dir "$(BUILD_DIR)" --output-on-failure

sandbox: build
	./$(BUILD_DIR)/sandbox/kcore-sandbox

clean:
	rm -rf build CMakeUserPresets.json

ROOT_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR = $(ROOT_DIR)/build
RELEASE_DIR = $(BUILD_DIR)/Release
DEBUG_DIR = $(BUILD_DIR)/Debug
CONAN_DIR = $(ROOT_DIR)/conan
CONAN_PROFILES = release debug
CMAKE_GENERATOR = Ninja
CMAKE_GENERATOR_PRODUCT = build.ninja
CMDSEP = ;

all: launch-benchmarks
.PHONY: all launch-benchmarks build-benchmarks config-release config-debug install_deps clean

install_deps $(ROOT_DIR)/ConanPresets.json:
	$(foreach profile, $(CONAN_PROFILES), \
		conan install $(ROOT_DIR)/conanfile.py \
			--profile=$(CONAN_DIR)/profiles/$(profile) \
			--conf=tools.cmake.cmaketoolchain:generator=$(CMAKE_GENERATOR) \
			--build=missing \
		$(CMDSEP) \
	)

config-release $(RELEASE_DIR)/$(CMAKE_GENERATOR_PRODUCT): $(ROOT_DIR)/ConanPresets.json
	cmake --preset release

config-debug $(DEBUG_DIR)/$(CMAKE_GENERATOR_PRODUCT): $(ROOT_DIR)/ConanPresets.json
	cmake --preset debug

build-benchmarks $(RELEASE_DIR)/benchmarks/benchmarks: $(RELEASE_DIR)/$(CMAKE_GENERATOR_PRODUCT) $(ROOT_DIR)/ConanPresets.json
	rm -f $(RELEASE_DIR)/benchmarks/benchmarks
	cmake --build --preset release --target benchmarks

launch-benchmarks: $(RELEASE_DIR)/benchmarks/benchmarks
	$(RELEASE_DIR)/benchmarks/benchmarks

clean:
	rm -f $(ROOT_DIR)/ConanPresets.json
	rm -rf $(BUILD_DIR)

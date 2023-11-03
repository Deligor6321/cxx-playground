ROOT_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR = $(ROOT_DIR)/build
RELEASE_DIR = $(BUILD_DIR)/Release
CONAN_DIR = $(ROOT_DIR)/conan
CONAN_PROFILES = release debug
CMAKE_GENERATOR = Ninja
CMAKE_GENERATOR_PRODUCT = build.ninja
CMDSEP = ;

all: launch-release
.PHONY: all launch-release build-release config-release install_deps clean

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

build-release $(RELEASE_DIR)/bench: $(RELEASE_DIR)/$(CMAKE_GENERATOR_PRODUCT) $(ROOT_DIR)/ConanPresets.json
	rm -f $(RELEASE_DIR)/bench
	cmake --build --preset release

launch-release: $(RELEASE_DIR)/bench
	$(RELEASE_DIR)/bench

clean:
	rm -f $(ROOT_DIR)/ConanPresets.json
	rm -rf $(BUILD_DIR)

ROOT_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR = $(ROOT_DIR)/build
RELEASE_DIR = $(BUILD_DIR)/Release
DEBUG_DIR = $(BUILD_DIR)/Debug
CONAN_DIR = $(ROOT_DIR)/conan
CONAN_PROFILES = release debug
CMAKE_GENERATOR = Ninja
CMAKE_GENERATOR_PRODUCT = build.ninja
COMPILE_COMMANDS = compile_commands.json
CONAN_PRESETS = ConanPresets.json
CMDSEP = ;

all: launch-benchmarks launch-tests
.PHONY: all launch-benchmarks build-benchmarks config-release config-debug init compile_commands install_deps clean

install_deps $(ROOT_DIR)/$(CONAN_PRESETS):
	$(foreach profile, $(CONAN_PROFILES), \
		conan install $(ROOT_DIR)/conanfile.py \
			--profile=$(CONAN_DIR)/profiles/$(profile) \
			--conf=tools.cmake.cmaketoolchain:generator=$(CMAKE_GENERATOR) \
			--build=missing \
		$(CMDSEP) \
	)

init: $(ROOT_DIR)/$(CONAN_PRESETS) $(ROOT_DIR)/$(COMPILE_COMMANDS)

compile_commands $(ROOT_DIR)/$(COMPILE_COMMANDS) : $(RELEASE_DIR)/$(COMPILE_COMMANDS)
	cp $(RELEASE_DIR)/$(COMPILE_COMMANDS) $(ROOT_DIR)/$(COMPILE_COMMANDS)

config-release $(RELEASE_DIR)/$(CMAKE_GENERATOR_PRODUCT) $(RELEASE_DIR)/$(COMPILE_COMMANDS) : $(ROOT_DIR)/$(CONAN_PRESETS)
	cmake --preset release

config-debug $(DEBUG_DIR)/$(CMAKE_GENERATOR_PRODUCT) $(DEBUG_DIR)/$(COMPILE_COMMANDS) : $(ROOT_DIR)/$(CONAN_PRESETS)
	cmake --preset debug

build-benchmarks $(RELEASE_DIR)/benchmarks/benchmarks: $(RELEASE_DIR)/$(CMAKE_GENERATOR_PRODUCT) $(ROOT_DIR)/$(CONAN_PRESETS)
	rm -f $(RELEASE_DIR)/benchmarks/benchmarks
	cmake --build --preset release --target benchmarks

launch-benchmarks: $(RELEASE_DIR)/benchmarks/benchmarks
	$(RELEASE_DIR)/benchmarks/benchmarks

build-tests $(RELEASE_DIR)/tests/tests: $(RELEASE_DIR)/$(CMAKE_GENERATOR_PRODUCT) $(ROOT_DIR)/$(CONAN_PRESETS)
	rm -f $(RELEASE_DIR)/tests/tests
	cmake --build --preset release --target tests

launch-tests: $(RELEASE_DIR)/tests/tests
	$(RELEASE_DIR)/tests/tests

clean:
	rm -f $(ROOT_DIR)/$(CONAN_PRESETS)
	rm -f $(ROOT_DIR)/$(COMPILE_COMMANDS)
	rm -rf $(BUILD_DIR)

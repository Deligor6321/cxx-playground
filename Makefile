# Directories
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)/build
CONAN_DIR := $(ROOT_DIR)/conan

# Build settings
BUILD_TYPES := release debug
DEFAULT_BUILD_TYPE := release
BUILD_TARGETS := benchmarks tests
CMAKE_GENERATOR := Ninja
SOURCE_DIRS := include/dlgr tests/src benchmarks/src
SOURCE_EXTENSIONS := cc h

# Helper variables
CMAKE_GENERATOR_PRODUCT := build.ninja
COMPILE_COMMANDS := compile_commands.json
CONANFILE := $(ROOT_DIR)/conanfile.py
CONAN_CMAKE_PRESETS_FILE := $(ROOT_DIR)/CMakeUserPresets.json
CONAN_INSTALL_PRODUCT := generators/conan_toolchain.cmake
CMD_SEP := ;
CMAKE_FILES := $(shell find $(ROOT_DIR) -type f -name CMakeLists.txt)
SRC_FILES :=$(shell \
	$(foreach dir, $(SOURCE_DIRS), \
		$(foreach ext, $(SOURCE_EXTENSIONS), \
			find $(ROOT_DIR)/$(dir) -type f -name "*.$(ext)"$(CMD_SEP) )))

# Build type configs
BUILD_DIR_release := $(BUILD_DIR)/Release
BUILD_DIR_debug := $(BUILD_DIR)/Debug
CMAKE_CONFIG_PRESET_release := conan-release
CMAKE_CONFIG_PRESET_debug := conan-debug
CMAKE_BUILD_PRESET_release := conan-release
CMAKE_BUILD_PRESET_debug := conan-debug
CMAKE_TEST_PRESET_release := conan-release
CMAKE_TEST_PRESET_debug := conan-debug
CONAN_PROFILE_release := $(CONAN_DIR)/profiles/release
CONAN_PROFILE_debug := $(CONAN_DIR)/profiles/debug

.PHONY : all init compile-commands clean install-deps config build test \
	$(foreach _build_target, $(BUILD_TARGETS), launch-$(_build_target)) \
	$(foreach _build_target, $(BUILD_TARGETS), build-$(_build_target)) \
	$(foreach _build_type, $(BUILD_TYPES), install-deps-$(_build_type)) \
	$(foreach _build_type, $(BUILD_TYPES), config-$(_build_type)) \
	$(foreach _build_type, $(BUILD_TYPES), build-$(_build_type)) \
	$(foreach _build_type, $(BUILD_TYPES), test-$(_build_type)) \
	$(foreach _build_type, $(BUILD_TYPES), $(foreach _build_target, $(BUILD_TARGETS), build-$(_build_type)-$(_build_target))) \
	$(foreach _build_type, $(BUILD_TYPES), $(foreach _build_target, $(BUILD_TARGETS), launch-$(_build_type)-$(_build_target)))

all : init $(foreach _build_target, $(BUILD_TARGETS), launch-$(_build_target))

init : $(CONAN_CMAKE_PRESETS_FILE) $(ROOT_DIR)/$(COMPILE_COMMANDS)

compile-commands $(ROOT_DIR)/$(COMPILE_COMMANDS) : $(BUILD_DIR_release)/$(COMPILE_COMMANDS)
	cp $(BUILD_DIR_release)/$(COMPILE_COMMANDS) $(ROOT_DIR)/$(COMPILE_COMMANDS)

define INSTALL_DEPS_RULE
install-deps-$(1) $(BUILD_DIR_$(1))/$(CONAN_INSTALL_PRODUCT) : $(CONANFILE) $(CONAN_PROFILE_$(1))
	conan install $(CONANFILE)  \
		--profile=$(CONAN_PROFILE_$(1)) \
		--conf=tools.cmake.cmaketoolchain:generator=$(CMAKE_GENERATOR) \
		--build=missing
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call INSTALL_DEPS_RULE,$(_build_type))))

$(CONAN_CMAKE_PRESETS_FILE) : $(foreach _build_type, $(BUILD_TYPES), $(BUILD_DIR_$(_build_type))/$(CONAN_INSTALL_PRODUCT))
	touch $(CONAN_CMAKE_PRESETS_FILE)

install-deps : $(CONAN_CMAKE_PRESETS_FILE) $(foreach _build_type, $(BUILD_TYPES), install-deps-$(_build_type))

define CONFIG_RULE
config-$(1) $(BUILD_DIR_$(1))/$(CMAKE_GENERATOR_PRODUCT) $(BUILD_DIR_$(1))/$(COMPILE_COMMANDS) : $(CONAN_CMAKE_PRESETS_FILE) $(CMAKE_FILES)
	cmake --preset $(CMAKE_CONFIG_PRESET_$(1)) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call CONFIG_RULE,$(_build_type))))

config : config-$(DEFAULT_BUILD_TYPE)

define BUILD_TARGET_RULE
build-$(1)-$(2) $(BUILD_DIR_$(1))/$(2)/$(2) : $(BUILD_DIR_$(1))/$(CMAKE_GENERATOR_PRODUCT) $(CONAN_CMAKE_PRESETS_FILE) $(SRC_FILES)
	rm $(BUILD_DIR_$(1))/$(2)/$(2)
	cmake --build --preset $(CMAKE_BUILD_PRESET_$(1)) --target $(2)
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(foreach _build_target, $(BUILD_TARGETS), \
		$(eval $(call BUILD_TARGET_RULE,$(_build_type),$(_build_target)))))

define BUILD_TARGET_DEFAULT_RULE
build-$(1) : build-$(DEFAULT_BUILD_TYPE)-$(1)
endef
$(foreach _build_target, $(BUILD_TARGETS), \
	$(eval $(call BUILD_TARGET_DEFAULT_RULE,$(_build_target))))

define BUILD_RULE
build-$(1) : $(foreach _build_target, $(BUILD_TARGETS), build-$(1)-$(_build_target))
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call BUILD_RULE,$(_build_type))))

build : build-$(DEFAULT_BUILD_TYPE)

define LAUNCH_TARGET_RULE
launch-$(1)-$(2) : $(BUILD_DIR_$(1))/$(2)/$(2)
	$(BUILD_DIR_$(1))/$(2)/$(2)
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(foreach _build_target, $(BUILD_TARGETS), \
		$(eval $(call LAUNCH_TARGET_RULE,$(_build_type),$(_build_target)))))

define LAUNCH_TARGET_DEFAULT_RULE
launch-$(1) : launch-$(DEFAULT_BUILD_TYPE)-$(1)
endef
$(foreach _build_target, $(BUILD_TARGETS), \
	$(eval $(call LAUNCH_TARGET_DEFAULT_RULE,$(_build_target))))

define TEST_RULE
test-$(1) : $(CONAN_CMAKE_PRESETS_FILE) $(foreach _build_target, $(BUILD_TARGETS), $(BUILD_DIR_$(1))/$(_build_target)/$(_build_target))
	ctest --preset $(CMAKE_TEST_PRESET_$(1))
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call TEST_RULE,$(_build_type))))

test : test-$(DEFAULT_BUILD_TYPE)

clean:
	rm -f $(CONAN_CMAKE_PRESETS_FILE)
	rm -f $(ROOT_DIR)/$(COMPILE_COMMANDS)
	rm -rf $(BUILD_DIR)

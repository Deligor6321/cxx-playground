# Directories
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT_DIR)/build
CONAN_DIR := $(ROOT_DIR)/conan

# Build settings
BUILD_TYPES := release debug
DEFAULT_BUILD_TYPE := release
SANITIZERS := ASan UBSan TSan
TEST_TARGETS := $(foreach _san, $(SANITIZERS), utests-$(_san))
BUILD_TARGETS := benchmarks $(TEST_TARGETS)
TEST_FAST_TARGET := $(firstword $(TEST_TARGETS))
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
CMD_AND := &&
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
TARGET_SUBDIR_benchmarks := benchmarks
define TARGET_SUBDIR_TESTS_DEF
TARGET_SUBDIR_$(1) := tests
endef
$(foreach _test_target, $(TEST_TARGETS), \
	$(eval $(call TARGET_SUBDIR_TESTS_DEF,$(_test_target))))

.PHONY : help init compile-commands clean install-deps config build test \
	iwyu cppcheck clang-format clang-tidy validate \
	$(foreach _build_target, $(BUILD_TARGETS), launch-$(_build_target)) \
	$(foreach _build_target, $(BUILD_TARGETS), build-$(_build_target)) \
	$(foreach _build_type, $(BUILD_TYPES), config-$(_build_type)) \
	$(foreach _build_type, $(BUILD_TYPES), build-$(_build_type)) \
	$(foreach _build_type, $(BUILD_TYPES), test-$(_build_type)) \
	$(foreach _build_type, $(BUILD_TYPES), $(foreach _build_target, $(BUILD_TARGETS), build-$(_build_type)-$(_build_target))) \
	$(foreach _build_type, $(BUILD_TYPES), $(foreach _build_target, $(BUILD_TARGETS), launch-$(_build_type)-$(_build_target)))

help :
	@echo "Available rules:"
	@echo "  help                    show this message"
	@echo "  init                    init the repo"
	@echo "  compile-commands        genearte compile commands in the repo's root"
	@echo "  clean                   remove generated files"
	@echo "  install-deps            install project dependencies"
	@echo "  config                  configure project with default build type"
	@echo "  build                   build all targets with default build type"
	@echo "  test                    run all tests with default build type"
	@echo "  launch-{target}         run specific target with default build type"
	@echo "  build-{target}          build specific target with default build type"
	@echo "  config-{type}           configure project with specific build type"
	@echo "  build-{type}            build all targets with specific build type"
	@echo "  test-{type}             run all tests with specific build type"
	@echo "  launch-{type}-{target}  run specific target with specific build type"
	@echo "  build-{type}-{target}   build specific target with specific build type"
	@echo ""
	@echo "Build types: $(foreach _build_type,$(BUILD_TYPES),$(_build_type))"
	@echo "Default build type: $(DEFAULT_BUILD_TYPE)"
	@echo ""
	@echo "Build targets: $(foreach _build_target,$(BUILD_TARGETS),$(_build_target))"

init : $(CONAN_CMAKE_PRESETS_FILE) $(ROOT_DIR)/$(COMPILE_COMMANDS)

compile-commands $(ROOT_DIR)/$(COMPILE_COMMANDS) : $(BUILD_DIR_release)/$(COMPILE_COMMANDS)
	cp $(BUILD_DIR_release)/$(COMPILE_COMMANDS) $(ROOT_DIR)/$(COMPILE_COMMANDS)

define INSTALL_DEPS_RULE
$(BUILD_DIR_$(1))/$(CONAN_INSTALL_PRODUCT) : $(CONANFILE) $(CONAN_PROFILE_$(1))
	conan install $(CONANFILE)  \
		--profile=$(CONAN_PROFILE_$(1)) \
		--conf=tools.cmake.cmaketoolchain:generator=$(CMAKE_GENERATOR) \
		--build=missing
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call INSTALL_DEPS_RULE,$(_build_type))))

install-deps $(CONAN_CMAKE_PRESETS_FILE) : $(foreach _build_type, $(BUILD_TYPES), $(BUILD_DIR_$(_build_type))/$(CONAN_INSTALL_PRODUCT))
	touch $(CONAN_CMAKE_PRESETS_FILE)

define CONFIG_RULE
config-$(1) $(BUILD_DIR_$(1))/$(CMAKE_GENERATOR_PRODUCT) $(BUILD_DIR_$(1))/$(COMPILE_COMMANDS) : $(CONAN_CMAKE_PRESETS_FILE) $(CMAKE_FILES)
	cmake --preset $(CMAKE_CONFIG_PRESET_$(1))
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call CONFIG_RULE,$(_build_type))))

config : config-$(DEFAULT_BUILD_TYPE)

define BUILD_TARGET_RULE
build-$(1)-$(2) $(BUILD_DIR_$(1))/$(TARGET_SUBDIR_$(2))/$(2) : $(BUILD_DIR_$(1))/$(CMAKE_GENERATOR_PRODUCT) $(CONAN_CMAKE_PRESETS_FILE) $(SRC_FILES)
	rm -f $(BUILD_DIR_$(1))/$(TARGET_SUBDIR_$(2))/$(2)
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
launch-$(1)-$(2) : $(BUILD_DIR_$(1))/$(TARGET_SUBDIR_$(2))/$(2)
	$(BUILD_DIR_$(1))/$(TARGET_SUBDIR_$(2))/$(2)
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
test-$(1) : $(CONAN_CMAKE_PRESETS_FILE) $(foreach _build_target, $(TEST_TARGETS), $(BUILD_DIR_$(1))/$(_build_target)/$(_build_target))
	ctest --preset $(CMAKE_TEST_PRESET_$(1)) --output-on-failure
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call TEST_RULE,$(_build_type))))

test : test-$(DEFAULT_BUILD_TYPE)

define TEST_FAST_RULE
test-fast-$(1) : $(CONAN_CMAKE_PRESETS_FILE) $(BUILD_DIR_$(1))/$(TEST_FAST_TARGET)/$(TEST_FAST_TARGET)
	ctest --preset $(CMAKE_TEST_PRESET_$(1)) -R ''^$(TEST_FAST_TARGET)$$' --output-on-failure
endef
$(foreach _build_type, $(BUILD_TYPES), \
	$(eval $(call TEST_FAST_RULE,$(_build_type))))

test-fast : test-fast-$(DEFAULT_BUILD_TYPE)

iwyu : $(ROOT_DIR)/$(COMPILE_COMMANDS)
	iwyu_tool.py -p $(ROOT_DIR) -- -Xiwyu --mapping_file=$(ROOT_DIR)/tools/iwyu/libcxx.imp

cppcheck :
	cppcheck -v --error-exitcode=1 --enable=all --language=c++ \
		--suppress=unmatchedSuppression --suppress=missingIncludeSystem --suppress=unusedFunction \
		--inline-suppr --suppressions-list=cppcheck-suppressions.list \
		$(SRC_FILES)

clang-format :
	clang-format -n --Werror $(SRC_FILES)

clang-tidy : $(ROOT_DIR)/$(COMPILE_COMMANDS)
	clang-tidy $(SRC_FILES)

# TODO(improve): Add cpplint, iwyu
validate : cppcheck clang-format clang-tidy

clean:
	rm -f $(CONAN_CMAKE_PRESETS_FILE)
	rm -f $(ROOT_DIR)/$(COMPILE_COMMANDS)
	rm -rf $(BUILD_DIR)

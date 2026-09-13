# ---------------------------------------------------------------------------------------------------------------------
#
# Copyright (C) Microsoft Corporation.  
# Copyright (C) 2025 IAMAI CONSULTING CORP
#
# MIT License. All rights reserved.
#
# Module Name:
#
#   build_macos.mk
#
# Abstract:
#
#   Main makefile for macOS builds to drive CMake/UBT commands.
#
# ---------------------------------------------------------------------------------------------------------------------

REDIRECT_OUTPUT = > /dev/null 2>&1

default:
	@echo "======================================================================="
	@echo "No target specified. Please run './build_macos.sh [target]' using the following targets:"
	@echo
	@echo " all = Build + Test + Package everything"
	@echo " rebuild_all = Clean + Build + Test + Package everything"
	@echo " all_no_test = Build + Package everything"
	@echo " clean = Clean sim libs + Blocks build files"
	@echo
	@echo " simlibs_debug = Build + Package sim libs for Debug"
	@echo " simlibs_release = Build + Package sim libs for Release"
	@echo " test_simlibs_debug = Test sim libs for Debug"
	@echo " test_simlibs_release = Test sim libs for Release"
	@echo
	@echo " blocks_debuggame = Build Plugin + Blocks for DebugGame (uses Debug sim libs)"
	@echo " blocks_development = Build Plugin + Blocks for Development (uses Release sim libs)"
	@echo " blocks_shipping = Build Plugin + Blocks for Shipping (uses Release sim libs)"
	@echo
	@echo " package_simlibs = Package sim libs for Debug + Release"
	@echo " package_plugin = Package UE Plugin for Debug + Release"
	@echo " package_blocks_debuggame = Package stand-alone Blocks environment executable for DebugGame"
	@echo " package_blocks_development = Package stand-alone Blocks environment executable for Development"
	@echo " package_blocks_shipping = Package stand-alone Blocks environment executable for Shipping"
	@echo

.PHONY: all
all: simlibs_debug test_simlibs_debug simlibs_release test_simlibs_release package_simlibs package_plugin package_blocks_debuggame package_blocks_development package_blocks_shipping

.PHONY: rebuild_all
rebuild_all: clean simlibs_debug test_simlibs_debug simlibs_release test_simlibs_release package_simlibs package_plugin package_blocks_debuggame package_blocks_development package_blocks_shipping

.PHONY: all_no_test
all_no_test: simlibs_debug simlibs_release package_simlibs package_plugin package_blocks_debuggame package_blocks_development package_blocks_shipping

# ---------------------------------------------------------------------------------------------------------------------
#
# CMAKE integration.
#
# ---------------------------------------------------------------------------------------------------------------------

CMAKE_BUILD_DIR = build/macos64
CMAKE_CMD = cmake -G "Ninja"

CMAKE_DBG_BUILD_CMD = cmake --build $(CMAKE_BUILD_DIR)/Debug
CMAKE_REL_BUILD_CMD = cmake --build $(CMAKE_BUILD_DIR)/Release

.PHONY: config_simlibs_debug
config_simlibs_debug:
	@echo "======================================================================="
	@echo "Configuring the ProjectAirSimLibs project for macOS64-Debug..."
	mkdir -p $(CMAKE_BUILD_DIR)/Debug $(REDIRECT_OUTPUT)
	cd $(CMAKE_BUILD_DIR)/Debug && $(CMAKE_CMD) -DCMAKE_BUILD_TYPE=Debug ../../..

.PHONY: simlibs_debug
simlibs_debug: config_simlibs_debug
	@echo "======================================================================="
	@echo "Building the ProjectAirSimLibs project for macOS64-Debug..."
	$(CMAKE_DBG_BUILD_CMD)

.PHONY: config_simlibs_release
config_simlibs_release:
	@echo "======================================================================="
	@echo "Configuring the ProjectAirSimLibs project for macOS64-Release..."
	mkdir -p $(CMAKE_BUILD_DIR)/Release $(REDIRECT_OUTPUT)
	cd $(CMAKE_BUILD_DIR)/Release && $(CMAKE_CMD) -DCMAKE_BUILD_TYPE=Release ../../..

.PHONY: simlibs_release
simlibs_release: config_simlibs_release
	@echo "======================================================================="
	@echo "Building the ProjectAirSimLibs project for macOS64-Release..."
	$(CMAKE_REL_BUILD_CMD)

.PHONY: package_simlibs
package_simlibs: simlibs_debug simlibs_release
	@echo "======================================================================="
	@echo "Packaging sim libs for use in custom projects..."
	mkdir -p $(CURDIR)/packages/projectairsim_simlibs/
	rsync -a $(CURDIR)/unreal/Blocks/Plugins/ProjectAirSim/SimLibs/ \
		$(CURDIR)/packages/projectairsim_simlibs/
	@echo "Packaging completed to: $(CURDIR)/packages/projectairsim_simlibs"

.PHONY: clean
clean:
	@echo "======================================================================="
	@echo "Cleaning build artifacts..."
	rm -rf $(CMAKE_BUILD_DIR) $(REDIRECT_OUTPUT)
	rm -rf $(CURDIR)/packages/projectairsim_simlibs/ $(REDIRECT_OUTPUT)
	@echo "Clean completed."

# ---------------------------------------------------------------------------------------------------------------------
#
# Testing.
#
# ---------------------------------------------------------------------------------------------------------------------

.PHONY: test_simlibs_debug
test_simlibs_debug: simlibs_debug
	@echo "======================================================================="
	@echo "Testing sim libs for Debug..."
	cd $(CMAKE_BUILD_DIR)/Debug && ctest --output-on-failure

.PHONY: test_simlibs_release
test_simlibs_release: simlibs_release
	@echo "======================================================================="
	@echo "Testing sim libs for Release..."
	cd $(CMAKE_BUILD_DIR)/Release && ctest --output-on-failure

# ---------------------------------------------------------------------------------------------------------------------
#
# Unreal Engine integration (if UE_ROOT is set).
#
# ---------------------------------------------------------------------------------------------------------------------

ifdef UE_ROOT
.PHONY: blocks_debuggame
blocks_debuggame: simlibs_debug
	@echo "======================================================================="
	@echo "Building Plugin + Blocks for DebugGame (uses Debug sim libs)..."
	$(UE_ROOT)/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh -project="$(CURDIR)/unreal/Blocks/Blocks.uproject" -game -engine
	$(UE_ROOT)/Engine/Build/BatchFiles/Mac/Build.sh Blocks Mac DebugGame $(CURDIR)/unreal/Blocks/Blocks.uproject

.PHONY: blocks_development
blocks_development: simlibs_release
	@echo "======================================================================="
	@echo "Building Plugin + Blocks for Development (uses Release sim libs)..."
	$(UE_ROOT)/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh -project="$(CURDIR)/unreal/Blocks/Blocks.uproject" -game -engine
	$(UE_ROOT)/Engine/Build/BatchFiles/Mac/Build.sh Blocks Mac Development $(CURDIR)/unreal/Blocks/Blocks.uproject

.PHONY: blocks_shipping
blocks_shipping: simlibs_release
	@echo "======================================================================="
	@echo "Building Plugin + Blocks for Shipping (uses Release sim libs)..."
	$(UE_ROOT)/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh -project="$(CURDIR)/unreal/Blocks/Blocks.uproject" -game -engine
	$(UE_ROOT)/Engine/Build/BatchFiles/Mac/Build.sh Blocks Mac Shipping $(CURDIR)/unreal/Blocks/Blocks.uproject

.PHONY: package_plugin
package_plugin: blocks_debuggame blocks_development blocks_shipping
	@echo "======================================================================="
	@echo "Packaging UE Plugin for Debug + Release..."
	mkdir -p $(CURDIR)/packages/projectairsim_ue_plugin/
	rsync -a --exclude 'Binaries' --exclude 'Intermediate' $(CURDIR)/unreal/Blocks/Plugins/ \
		$(CURDIR)/packages/projectairsim_ue_plugin/Plugins/
	@echo "Plugin packaging completed to: $(CURDIR)/packages/projectairsim_ue_plugin"

.PHONY: package_blocks_debuggame
package_blocks_debuggame: simlibs_debug simlibs_release
	@echo "======================================================================="
	@echo "Building/cooking/packaging UE stand-alone game for DebugGame (with Debug sim libs) variant..."
ifndef UE_ROOT
	@echo
	@echo "ERROR: UE_ROOT environment variable is not set. It must be set to the target \
	Unreal engine's root folder path, ex. /Users/username/UnrealEngine"
else
	@echo "UE_ROOT env variable set to: $(UE_ROOT)"
	$(UE_ROOT)/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
		-project="$(CURDIR)/unreal/Blocks/Blocks.uproject" \
		-nop4 -nocompile -build -cook -compressed -pak -allmaps -stage \
		-archive -archivedirectory="$(CURDIR)/packages/Blocks/DebugGame" \
		-clientconfig=DebugGame -clean -utf8output -prereqs
	@echo "Blocks DebugGame packaging completed to: $(CURDIR)/packages/Blocks/DebugGame"
endif

.PHONY: package_blocks_development
package_blocks_development: blocks_development
	@echo "======================================================================="
	@echo "Packaging stand-alone Blocks environment executable for Development..."
ifndef UE_ROOT
	@echo
	@echo "ERROR: UE_ROOT environment variable is not set. It must be set to the target \
	Unreal engine's root folder path, ex. /Users/username/UnrealEngine"
else
	@echo "UE_ROOT env variable set to: $(UE_ROOT)"
	$(UE_ROOT)/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
		-project="$(CURDIR)/unreal/Blocks/Blocks.uproject" \
		-nop4 -nocompileeditor --installed -build -cook -compressed -pak -allmaps -stage \
		-archive -archivedirectory="$(CURDIR)/packages/Blocks/Development" \
		-platform=Mac -CookCultures=en \
		-clientconfig=Development -serverconfig=Development -utf8output -prereqs \
		-createreleaseversion= -iterativecooking -stage -package -stagingdirectory=$(CURDIR)/staging/Blocks/Development -cmdline="BlocksMap -Messaging"
	@echo "Blocks Development packaging completed to: $(CURDIR)/packages/Blocks/Development"
endif

.PHONY: package_blocks_shipping
package_blocks_shipping: blocks_shipping
	@echo "======================================================================="
	@echo "Packaging stand-alone Blocks environment executable for Shipping..."
ifndef UE_ROOT
	@echo
	@echo "ERROR: UE_ROOT environment variable is not set. It must be set to the target \
	Unreal engine's root folder path, ex. /Users/username/UnrealEngine"
else
	@echo "UE_ROOT env variable set to: $(UE_ROOT)"
	$(UE_ROOT)/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
		-project="$(CURDIR)/unreal/Blocks/Blocks.uproject" \
		-nop4 -nocompile -build -cook -compressed -pak -allmaps -stage \
		-archive -archivedirectory="$(CURDIR)/packages/Blocks/Shipping" \
		-clientconfig=Shipping -serverconfig=Shipping -utf8output -installed -prereqs \
		-unrealexe=$(UE_ROOT)/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor \
		-platform=Mac -map=BlocksMap+BlocksMap -CookCultures=en -makebinaryconfig \
		-createreleaseversion= -iterativecooking -package -stagingdirectory=$(CURDIR)/staging/Blocks/Shipping/ \
		-cmdline="BlocksMap -Messaging"

	@echo "Blocks Shipping packaging completed to: $(CURDIR)/packages/Blocks/Shipping"
endif
else
.PHONY: blocks_debuggame blocks_development blocks_shipping package_plugin package_blocks_debuggame package_blocks_development package_blocks_shipping
blocks_debuggame blocks_development blocks_shipping package_plugin package_blocks_debuggame package_blocks_development package_blocks_shipping:
	@echo "======================================================================="
	@echo "UE_ROOT environment variable is not set. Skipping Unreal Engine builds."
	@echo "To build Unreal Engine components, set UE_ROOT to your Unreal Engine installation path."
	@echo "Example: export UE_ROOT=/Users/username/UnrealEngine"
endif

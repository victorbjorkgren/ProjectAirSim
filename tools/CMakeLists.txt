# ---------------------------------------------------------------------------------------------------------------------
#
# Copyright (C) Microsoft Corporation.  
# Copyright (C) 2025 IAMAI CONSULTING CORP
#
# MIT License. All rights reserved.
#
# Module Name:
#
#   CMakeLists.txt
#
# Abstract:
#
#   CMake build file.
#
# ---------------------------------------------------------------------------------------------------------------------

# Add lvmon library (Windows only - not compatible with macOS/Linux)
if(WIN32)
    add_subdirectory(lvmon)
else()
    message(STATUS "Skipping lvmon tool - not compatible with ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Using lvmon stub header for ${CMAKE_SYSTEM_NAME}")
endif()

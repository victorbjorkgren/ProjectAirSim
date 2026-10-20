# ---------------------------------------------------------------------------------------------------------------------
#
# Copyright (C) Microsoft Corporation.
# Copyright (C) 2025 IAMAI CONSULTING CORP
#
# MIT License. All rights reserved.
#
# Module Name:
#
#   unreal-linux-toolchain.cmake
#
# Abstract:
#
#   CMake Linux toolchain file for building the sim libs the way Unreal Engine 5.x builds a Linux game:
#   cross toolchain "v26_clang-20.1.8-rockylinux8" (UE 5.8), clang with the libc++ that ships inside
#   the toolchain (include/c++/v1, lib64/libc++.a, libc++abi.a), glibc 2.28 sysroot.
#
#   The toolchain is Epic's public download (not part of this repository):
#     https://cdn.unrealengine.com/CrossToolchain_Linux/v26_clang-20.1.8-rockylinux8.exe
#   (on a UE install it is Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/... or the LINUX_MULTIARCH_ROOT folder).
#
# Environment:
#
#   LINUX_MULTIARCH_ROOT  (required) folder that contains x86_64-unknown-linux-gnu/ and aarch64-unknown-linux-gnueabi/
#   PAS_LINUX_ARCH        x86_64 (default) or aarch64
#   PAS_CLANG_DIR         folder with a host clang/clang++ (default: <toolchain>/bin; set it when the toolchain's own
#                         clang cannot run on the build host, e.g. an upstream LLVM 20.1.8 on a Linux host)
#
# ---------------------------------------------------------------------------------------------------------------------

if(NOT DEFINED ENV{LINUX_MULTIARCH_ROOT})
    message(FATAL_ERROR "LINUX_MULTIARCH_ROOT must point at the UE Linux cross toolchain (e.g. .../v26_clang-20.1.8-rockylinux8)")
endif()

set(PAS_LINUX_ARCH "$ENV{PAS_LINUX_ARCH}")
if(NOT PAS_LINUX_ARCH)
    set(PAS_LINUX_ARCH x86_64)
endif()
if(PAS_LINUX_ARCH STREQUAL "x86_64")
    set(PAS_TRIPLE x86_64-unknown-linux-gnu)
elseif(PAS_LINUX_ARCH STREQUAL "aarch64")
    set(PAS_TRIPLE aarch64-unknown-linux-gnueabi)  # UE's name for LinuxArm64
else()
    message(FATAL_ERROR "PAS_LINUX_ARCH must be x86_64 or aarch64, got '${PAS_LINUX_ARCH}'")
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR ${PAS_LINUX_ARCH})

file(TO_CMAKE_PATH "$ENV{LINUX_MULTIARCH_ROOT}" PAS_TC_ROOT)
set(PAS_TC_BASE "${PAS_TC_ROOT}/${PAS_TRIPLE}")

if(DEFINED ENV{PAS_CLANG_DIR})
    file(TO_CMAKE_PATH "$ENV{PAS_CLANG_DIR}" PAS_CLANG_DIR)
else()
    set(PAS_CLANG_DIR "${PAS_TC_BASE}/bin")
endif()
if(CMAKE_HOST_WIN32)
    set(PAS_EXE_SUFFIX ".exe")
else()
    set(PAS_EXE_SUFFIX "")
endif()
set(CMAKE_C_COMPILER   "${PAS_CLANG_DIR}/clang${PAS_EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${PAS_CLANG_DIR}/clang++${PAS_EXE_SUFFIX}")
set(CMAKE_AR     "${PAS_CLANG_DIR}/llvm-ar${PAS_EXE_SUFFIX}" CACHE FILEPATH "")
set(CMAKE_RANLIB "${PAS_CLANG_DIR}/llvm-ranlib${PAS_EXE_SUFFIX}" CACHE FILEPATH "")

set(CMAKE_C_COMPILER_TARGET   ${PAS_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${PAS_TRIPLE})
set(CMAKE_SYSROOT "${PAS_TC_BASE}")

# Same flags UnrealBuildTool uses for Linux: bundled libc++ headers instead of the compiler's, PIC, unwind tables.
# They are appended to CMAKE_<LANG>_FLAGS (not set via *_FLAGS_INIT) because sub-projects such as assimp pass their own
# -DCMAKE_C_FLAGS/-DCMAKE_CXX_FLAGS, which would replace the _INIT value.
# (-fuse-ld=lld rides in the compile flags: CMake passes them on the link line too, and sub-projects that set
# CMAKE_EXE_LINKER_FLAGS explicitly would otherwise drop it.)
set(PAS_COMMON_FLAGS "-fuse-ld=lld -Qunused-arguments -fPIC -fno-math-errno -funwind-tables -ffunction-sections -fdata-sections -fbinutils-version=2.36")
string(APPEND CMAKE_C_FLAGS   " ${PAS_COMMON_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS " ${PAS_COMMON_FLAGS} -nostdinc++ -isystem ${PAS_TC_BASE}/include -isystem ${PAS_TC_BASE}/include/c++/v1")

# Link like UE: the toolchain's static libc++/libc++abi, no compiler default C++ runtime.
set(CMAKE_CXX_STANDARD_LIBRARIES "-nodefaultlibs ${PAS_TC_BASE}/lib64/libc++.a ${PAS_TC_BASE}/lib64/libc++abi.a -lm -lc -lpthread -lgcc_s -lgcc" CACHE STRING "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

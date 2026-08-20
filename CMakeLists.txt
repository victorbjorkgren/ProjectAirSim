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

cmake_minimum_required(VERSION 3.15.0)

# Before making projects, set CMake policy to use MSVC_RUNTIME_LIBRARY property
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")  # Have to match UE (uses /MD non-debug CRT for all configs)

include(FetchContent)
include(ExternalProject)

if (UNIX)
    if(DEFINED ENV{UE_ROOT} AND NOT APPLE)
        # Build/link against UE's packaged Linux toolchain instead of system-installed toolchain
        # Note: This must be set BEFORE `project()` is called
        # Only use Linux toolchain on actual Linux systems, not on macOS
        message("Building/linking against Unreal packaged toolchain from UE_ROOT=$ENV{UE_ROOT}")
        set(CMAKE_TOOLCHAIN_FILE "unreal-linux-toolchain.cmake")
        link_directories("$ENV{UE_ROOT}/Engine/Source/ThirdParty/Unix/LibCxx/lib/Unix/x86_64-unknown-linux-gnu")
    else()
        message("Building/linking against system-installed toolchain")
        if(APPLE)
            # macOS: use system clang or Homebrew LLVM
            if(EXISTS "/opt/homebrew/opt/llvm/bin/clang")
                set(CMAKE_C_COMPILER /opt/homebrew/opt/llvm/bin/clang)
                set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++)
            elseif(EXISTS "/usr/local/opt/llvm/bin/clang")
                set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang)
                set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
            else()
                # Use system clang
                set(CMAKE_C_COMPILER clang)
                set(CMAKE_CXX_COMPILER clang++)
            endif()
            # Set macOS deployment target to match Unreal Engine's requirement
            set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS deployment version" FORCE)
            message("Setting macOS deployment target to: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
        else()
            # Linux: use clang-13
            set(CMAKE_C_COMPILER clang-13)
            set(CMAKE_CXX_COMPILER clang++-13)
        endif()
    endif()
endif()

# Set up project
project("ProjectAirSimLibs")

# Set up paths for packaging the sim libs build outputs as custom post-command steps
set(SIMLIBS_PACKAGE_DIR ${CMAKE_SOURCE_DIR}/packages/projectairsim_simlibs)
set(SIMLIBS_SHARED_LIB_DIR ${SIMLIBS_PACKAGE_DIR}/shared_libs)
set(SIMLIBS_TEST_DIR ${CMAKE_BINARY_DIR}/unit_tests)
set(UE_PLUGIN_SIMLIBS_DIR ${CMAKE_SOURCE_DIR}/unreal/Blocks/Plugins/ProjectAirSim/SimLibs)
set(UNITY_WRAPPER_DLL_DIR ${CMAKE_SOURCE_DIR}/unity/BlocksUnity/Assets/Plugins)
set(MATLAB_PHYSICS_DIR ${CMAKE_SOURCE_DIR}/physics/matlab_sfunc)
set(MATLAB_CONTROL_DIR ${CMAKE_SOURCE_DIR}/vehicle_apis/multirotor_api/matlab_sfunc)
# set(UE_PLUGIN_CESIUM_NATIVE_DIR ${CMAKE_SOURCE_DIR}/unreal/Blocks/Plugins/ProjectAirSim/SimLibs)
# set(UE_CESIUM_PLUGIN_DIR ${CMAKE_SOURCE_DIR}/unreal/Blocks/Plugins/CesiumForUnreal/)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    # Note: /EHsc is for MSVC to enable unwinding stack during C++ exception handling
    add_compile_options(/EHsc)
    # set(CESIUM_CXX_FLAGS /EHsc)

    # Disable manifest generation for executables to avoid build failures due to Windows write
    # permission issues (possibly from file locks by antivirus or other OS file protection)
    add_link_options(/MANIFEST:NO)
elseif(UNIX)
    add_compile_definitions(__CLANG__)
    add_compile_options(-stdlib=libc++ -ferror-limit=10)
    
    if(APPLE)
        # macOS: -lanl is not available, only need -pthread
        add_link_options(-stdlib=libc++ -pthread)
    else()
        # Linux: -lanl (for getaddrinfo) and -pthread are required because of nng
        add_link_options(-stdlib=libc++ -lanl -pthread)
    endif()

    # set(CESIUM_CXX_FLAGS "-std=c++17 -stdlib=libc++")
    # set(CESIUM_LINKER_FLAGS "-stdlib=libc++ -lc++abi")
    # set(CESIUM_LINUX_TOOLCHAIN ${UE_CESIUM_PLUGIN_DIR}/extern/unreal-linux-toolchain.cmake")
    
    # Add lvmon stub header for macOS/Linux builds
    if(APPLE OR UNIX)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tools/lvmon/Lib/include)
        message(STATUS "Added lvmon stub header include path for ${CMAKE_SYSTEM_NAME}")
    endif()
endif()


# Set up dependency: nlohmann JSON
# Directly download single include file json.hpp
FetchContent_Declare(
    nlohmann-json
    URL                  https://raw.githubusercontent.com/nlohmann/json/v3.11.2/single_include/nlohmann/json.hpp
    URL_HASH             MD5=b1c1ce77d46b72b72f38051d8384c7b8  # from PowerShell: CertUtil -hashfile json.hpp MD5
    DOWNLOAD_NO_EXTRACT  True
    DOWNLOAD_NO_PROGRESS True
    DOWNLOAD_DIR         ${CMAKE_BINARY_DIR}/_deps/nlohmann-install
)
FetchContent_GetProperties(nlohmann-json)
if(NOT nlohmann-json_POPULATED)
    message("Checking if [nlohmann-json] dependency needs to be fetched...")
    FetchContent_Populate(nlohmann-json)
    set(JSON_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/nlohmann-install)
    message("JSON_INCLUDE_DIR set to ${JSON_INCLUDE_DIR}")

    message("Packaging [JSON (nlohmann)] header to ${UE_PLUGIN_SIMLIBS_DIR}/json/include")
    file(COPY ${JSON_INCLUDE_DIR}/ DESTINATION ${UE_PLUGIN_SIMLIBS_DIR}/json/include)
endif()

# Set up dependency: NNG
message("Setting up [NNG] dependency as an external project...")
# CMake external projects don't adopt the parent's CMAKE_MSVC_RUNTIME_LIBRARY setting,
# so to force /MD non-debug CRT to match UE, build Debug config as Relwithdebinfo and
# build Release as Release.
set(NNG_BUILD_TYPE $<IF:$<CONFIG:Debug>,Relwithdebinfo,Release>)
if(WIN32)
    set(NNG_CMAKE_LINKER_MANIFEST_FLAG "/MANIFEST:NO")
else()
    set(NNG_CMAKE_LINKER_MANIFEST_FLAG "")
endif()
ExternalProject_Add(nng-external
    GIT_REPOSITORY          https://github.com/nanomsg/nng.git
    GIT_TAG                 v1.11
    GIT_CONFIG              "advice.detachedHead=false"
    PREFIX                  ${CMAKE_BINARY_DIR}/_deps/nng
    # Use Update step override to delete unused /etc/ folder of NNG development support files that list a lot of Go reference packages
    UPDATE_COMMAND          ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/_deps/nng/src/nng-external/etc"
    BUILD_COMMAND           ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${NNG_BUILD_TYPE}
    INSTALL_COMMAND         ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${NNG_BUILD_TYPE} --target install
    TEST_COMMAND            ""  # disable test step
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_EXE_LINKER_FLAGS=${NNG_CMAKE_LINKER_MANIFEST_FLAG}
        -DCMAKE_POSITION_INDEPENDENT_CODE=True
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/_deps/nng-install
        -DCMAKE_INSTALL_MESSAGE=LAZY  # don't print out all files copied
        -DCMAKE_BUILD_TYPE:STRING=${NNG_BUILD_TYPE}  # for single-config generators (Ninja)
        -DNNG_HAVE_GETRANDOM=False  # llvm-ar fails to find symbol `getrandom` (a syscall on Linux); Requires glibc >=2.25
        -DNNG_TESTS=False  # don't build NNG's unit tests
        $<$<BOOL:${APPLE}>:-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}>  # Pass macOS deployment target on Apple platforms
    EXCLUDE_FROM_ALL        True
)
set(NNG_LIB_DIR ${CMAKE_BINARY_DIR}/_deps/nng-install/lib)
set(NNG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/nng-install/include)
message("NNG_LIB_DIR set to ${NNG_LIB_DIR}")
message("NNG_INCLUDE_DIR set to ${NNG_INCLUDE_DIR}")
ExternalProject_Add_Step(nng-external post-install
    COMMAND ${CMAKE_COMMAND} -E echo "Packaging [nng] installed outputs to ${UE_PLUGIN_SIMLIBS_DIR}/nng/$<IF:$<CONFIG:Release>,Release,Debug>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${NNG_LIB_DIR}" "${UE_PLUGIN_SIMLIBS_DIR}/nng/$<IF:$<CONFIG:Release>,Release,Debug>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${NNG_INCLUDE_DIR}" "${UE_PLUGIN_SIMLIBS_DIR}/nng/include"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${UE_PLUGIN_SIMLIBS_DIR}/nng/$<IF:$<CONFIG:Release>,Release,Debug>/cmake"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${UE_PLUGIN_SIMLIBS_DIR}/nng/$<IF:$<CONFIG:Release>,Release,Debug>/pkgconfig"

    # Also copy NNG install lib to Matlab physics and control S-functions folder for building in Matlab:
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${NNG_LIB_DIR}" "${MATLAB_PHYSICS_DIR}/_deps/nng-install/lib"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${NNG_INCLUDE_DIR}" "${MATLAB_PHYSICS_DIR}/_deps/nng-install/include"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${NNG_LIB_DIR}" "${MATLAB_CONTROL_DIR}/_deps/nng-install/lib"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${NNG_INCLUDE_DIR}" "${MATLAB_CONTROL_DIR}/_deps/nng-install/include"
    DEPENDEES install
)

# Set up dependency: assimp
if(APPLE)
    # On macOS, temporarily disable assimp due to zlib compatibility issues with macOS 15.5
    message("WARNING: [assimp] dependency disabled on macOS due to zlib compatibility issues")
    message("This will limit 3D model import capabilities but allows the build to proceed")
    set(ASSIMP_LIB_DIR "")
    set(ASSIMP_INCLUDE_DIR "")
    set(ASSIMP_LIBRARIES "")
else()
    # On non-macOS platforms, build from source as before
    message("Setting up [assimp] dependency as an external project...")
    set(ASSIMP_BUILD_TYPE $<IF:$<CONFIG:Debug>,Relwithdebinfo,Release>)
    if(WIN32)
        set(ASSIMP_LIBRARY_SUFFIX_OVERRIDE "-DLIBRARY_SUFFIX:STRING=-mt")
    else()
        set(ASSIMP_LIBRARY_SUFFIX_OVERRIDE "")
    endif()
    ExternalProject_Add(assimp-external
        GIT_REPOSITORY          https://github.com/assimp/assimp.git
        GIT_TAG                 v5.2.5
        GIT_CONFIG              "advice.detachedHead=false"
        PREFIX                  ${CMAKE_BINARY_DIR}/_deps/assimp
        UPDATE_COMMAND          ""  # disable update step
        BUILD_COMMAND           ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${ASSIMP_BUILD_TYPE}
        INSTALL_COMMAND         ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${ASSIMP_BUILD_TYPE} --target install
        TEST_COMMAND            ""  # disable test step
        CMAKE_ARGS
            -DCMAKE_SYSROOT=${CMAKE_SYSROOT}
            -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM}
            -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY}
            -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE}
            -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_POSITION_INDEPENDENT_CODE=True
            -DBUILD_SHARED_LIBS=OFF
            -DASSIMP_BUILD_ZLIB=ON  # force build of assimp's packaged zlib in case it detects it existing in a python environment
            -DASSIMP_BUILD_TESTS=OFF
            -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
            ${ASSIMP_LIBRARY_SUFFIX_OVERRIDE}
            -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/_deps/assimp-install
            -DCMAKE_INSTALL_MESSAGE=LAZY  # don't print out all files copied
            -DCMAKE_BUILD_TYPE:STRING=${ASSIMP_BUILD_TYPE}  # for single-config generators (Ninja)
            -DCMAKE_C_FLAGS="-Wno-deprecated-declarations -Wno-macro-redefined -D_POSIX_C_SOURCE=200809L"
            -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations -D_POSIX_C_SOURCE=200809L"
    )
    set(ASSIMP_LIB_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-install/lib)
    set(ASSIMP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-install/include)
    message("ASSIMP_LIB_DIR set to ${ASSIMP_LIB_DIR}")
    message("ASSIMP_INCLUDE_DIR set to ${ASSIMP_INCLUDE_DIR}")
    ExternalProject_Add_Step(assimp-external post-install
        COMMAND ${CMAKE_COMMAND} -E echo "Packaging [assimp] installed outputs to ${UE_PLUGIN_SIMLIBS_DIR}/assimp/$<IF:$<CONFIG:Release>,Release,Debug>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSIMP_LIB_DIR}" "${UE_PLUGIN_SIMLIBS_DIR}/assimp/$<IF:$<CONFIG:Release>,Release,Debug>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSIMP_INCLUDE_DIR}" "${UE_PLUGIN_SIMLIBS_DIR}/assimp/include"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${UE_PLUGIN_SIMLIBS_DIR}/assimp/$<IF:$<CONFIG:Release>,Release,Debug>/cmake"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${UE_PLUGIN_SIMLIBS_DIR}/assimp/$<IF:$<CONFIG:Release>,Release,Debug>/pkgconfig"
        DEPENDEES install
    )
endif()

# Set up dependency: msgpack
# "When you use msgpack on C++, you can just add msgpack-c/include to your include path"
FetchContent_Declare(
    msgpack
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG        cpp-6.1.0
)
FetchContent_GetProperties(msgpack)
if(NOT msgpack_POPULATED)
    message("Checking if [msgpack] dependency needs to be fetched...")
    FetchContent_Populate(msgpack)
    add_definitions(-DMSGPACK_NO_BOOST)
    set(MSGPACK_INCLUDE_DIR ${msgpack_SOURCE_DIR}/include)
    message("MSGPACK_INCLUDE_DIR set to ${MSGPACK_INCLUDE_DIR}")
    # Note: msgpack is only used internally by sim libs, so no need to package it with other sim libs

    # Copy msgpack headers to Matlab physics and control S-function folders for building in Matlab:
    message("Packaging [msgpack] headers to ${MATLAB_PHYSICS_DIR}/msgpack")
    file(COPY ${msgpack_SOURCE_DIR}/include/ DESTINATION ${MATLAB_PHYSICS_DIR}/_deps/msgpack/include)

    message("Packaging [msgpack] headers to ${MATLAB_CONTROL_DIR}/msgpack")
    file(COPY ${msgpack_SOURCE_DIR}/include/ DESTINATION ${MATLAB_CONTROL_DIR}/_deps/msgpack/include)
endif()

# Set up dependency: Eigen3
# Using latest stable release as of from 08/18/21
FetchContent_Declare(
    Eigen3
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
)
FetchContent_GetProperties(Eigen3)
string(TOLOWER "Eigen3" lc_eigen)
if(NOT ${lc_eigen}_POPULATED)
    message("Checking if [Eigen3] dependency needs to be fetched...")
    FetchContent_Populate(Eigen3)
    set(EIGEN_INCLUDE_DIR ${${lc_eigen}_SOURCE_DIR})
    message("EIGEN_INCLUDE_DIR set to ${EIGEN_INCLUDE_DIR}")

    message("Packaging [Eigen3] headers to ${UE_PLUGIN_SIMLIBS_DIR}/eigen")
    file(COPY ${EIGEN_INCLUDE_DIR}/ DESTINATION ${UE_PLUGIN_SIMLIBS_DIR}/eigen/include)
endif()

# Set up dependency: Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.1
)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    message("Checking if [GTest] dependency needs to be fetched...")
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

include(CTest) # this automatically also sets enable_testing()

# Set up dependency: tinygltf
# "When you use tinygltf on C++, you can just add to your include path since its header only library"
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.6.3
)
FetchContent_GetProperties(tinygltf)
if(NOT tinygltf_POPULATED)
    message("Checking if [tinygltf] dependency needs to be fetched...")
    FetchContent_Populate(tinygltf)
    set(TINYGLTF_INCLUDE_DIR ${tinygltf_SOURCE_DIR})
    file(REMOVE_RECURSE ${TINYGLTF_INCLUDE_DIR}/examples)
    message("TINYGLTF_INCLUDE_DIR set to ${TINYGLTF_INCLUDE_DIR}")
    # Note: tinygltf is only used internally by sim libs, so no need to package it with other sim libs
endif()

if(UNIX)
    # Set up dependency: openssl
    message("Setting up [openssl] dependency as an external project...")
    # CMake external projects don't adopt the parent's CMAKE_MSVC_RUNTIME_LIBRARY setting,
    # so to force /MD non-debug CRT to match UE, build Debug config as Relwithdebinfo and
    # build Release as Release.
    set(OPENSSL_BUILD_TYPE $<IF:$<CONFIG:Debug>,debug,release>)
    if(APPLE)
        # On macOS, pass deployment target via environment variable
        set(OPENSSL_MACOS_FLAGS "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    else()
        set(OPENSSL_MACOS_FLAGS "")
    endif()
    ExternalProject_Add(openssl-external
        GIT_REPOSITORY          https://github.com/openssl/openssl.git
        GIT_TAG                 openssl-3.1.1
        GIT_CONFIG              "advice.detachedHead=false"
        PREFIX                  ${CMAKE_BINARY_DIR}/_deps/openssl
        UPDATE_COMMAND          ""  # disable update step
        CONFIGURE_COMMAND       ${CMAKE_COMMAND} -E env ${OPENSSL_MACOS_FLAGS} <SOURCE_DIR>/Configure --${OPENSSL_BUILD_TYPE} --prefix=${CMAKE_BINARY_DIR}/_deps/openssl-install no-tests
        BUILD_COMMAND           ${CMAKE_COMMAND} -E env ${OPENSSL_MACOS_FLAGS} make
        INSTALL_COMMAND         make install
        TEST_COMMAND            ""  # disable test step
    )
    if(APPLE)
    set(OPENSSL_LIB_DIR ${CMAKE_BINARY_DIR}/_deps/openssl-install/lib)
else()
    set(OPENSSL_LIB_DIR ${CMAKE_BINARY_DIR}/_deps/openssl-install/lib64)
endif()
    set(OPENSSL_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/openssl-install/include)
    message("OPENSSL_LIB_DIR set to ${OPENSSL_LIB_DIR}")
    message("OPENSSL_INCLUDE_DIR set to ${OPENSSL_INCLUDE_DIR}")
    ExternalProject_Add_Step(openssl-external post-install
        COMMAND ${CMAKE_COMMAND} -E echo "Packaging [openssl] installed outputs to $${UE_PLUGIN_SIMLIBS_DIR}/openssl/$<IF:$<CONFIG:Release>,Release,Debug>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${OPENSSL_LIB_DIR}" "${UE_PLUGIN_SIMLIBS_DIR}/openssl/$<IF:$<CONFIG:Release>,Release,Debug>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${OPENSSL_INCLUDE_DIR}" "${UE_PLUGIN_SIMLIBS_DIR}/openssl/include"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${UE_PLUGIN_SIMLIBS_DIR}/openssl/$<IF:$<CONFIG:Release>,Release,Debug>/cmake"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${UE_PLUGIN_SIMLIBS_DIR}/openssl/$<IF:$<CONFIG:Release>,Release,Debug>/pkgconfig"
        DEPENDEES install
    )
endif()

if(APPLE)
    # ONNX Runtime is disabled for macOS due to API compatibility issues
    message("ONNX Runtime is disabled for macOS due to API compatibility issues")
    add_definitions(-DDISABLE_ONNX_RUNTIME)
    
    # CUDA and TensorRT are not available on macOS
    # add_definitions(-DUSE_CUDA)
    # add_definitions(-DUSE_TENSORRT)
elseif(UNIX)
    # For Linux, try to use system package or fall back to download
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ONNXRUNTIME QUIET onnxruntime)
        if(ONNXRUNTIME_FOUND)
            set(ONNXRUNTIME_ROOTDIR ${ONNXRUNTIME_PREFIX})
            set(ONNX_INCLUDE_DIR ${ONNXRUNTIME_INCLUDE_DIRS})
            set(ONNX_LIB_DIR ${ONNXRUNTIME_LIBRARY_DIRS})
            message("Using system-installed ONNX Runtime")
        else()
            # Fall back to downloading if system package not found
            set(onnx_url https://github.com/microsoft/onnxruntime/releases/download/v1.16.0/onnxruntime-linux-x64-gpu-1.16.0.tgz)
            set(onnx_hash MD5=27adfa51648d2713608f35c3c6957a4f)
            message("System ONNX Runtime not found, downloading...")
        endif()
    else()
        set(onnx_url https://github.com/microsoft/onnxruntime/releases/download/v1.16.0/onnxruntime-linux-x64-gpu-1.16.0.tgz)
        set(onnx_hash MD5=27adfa51648d2713608f35c3c6957a4f)
        message("PkgConfig not found, downloading ONNX Runtime...")
    endif()
    
    if(DEFINED onnx_url)
        FetchContent_Declare(
            onnxruntime
            URL                  ${onnx_url}
            URL_HASH             ${onnx_hash}
            DOWNLOAD_NO_PROGRESS True
            DOWNLOAD_DIR         ${CMAKE_BINARY_DIR}/_deps/onnxruntime-download
        )
        FetchContent_GetProperties(onnxruntime)

        if(NOT onnxruntime_POPULATED)
            message("Checking if [onnxruntime] dependency needs to be fetched...")
            FetchContent_Populate(onnxruntime)
            set(ONNXRUNTIME_ROOTDIR ${onnxruntime_SOURCE_DIR})
            set(ONNX_INCLUDE_DIR ${onnxruntime_SOURCE_DIR}/include)
            set(ONNX_LIB_DIR  ${onnxruntime_SOURCE_DIR}/lib/)
            message("ONNX_INCLUDE_DIR set to ${ONNX_INCLUDE_DIR}")
        endif()
    endif()
    
    if(ONNX_INCLUDE_DIR AND ONNX_LIB_DIR)
        message("Packaging [onnxruntime] to ${UE_PLUGIN_SIMLIBS_DIR}/shared_libs")
        file(COPY "${ONNX_INCLUDE_DIR}" DESTINATION "${UE_PLUGIN_SIMLIBS_DIR}/shared_libs/onnxruntime")
        file(COPY "${ONNX_LIB_DIR}" DESTINATION "${UE_PLUGIN_SIMLIBS_DIR}/shared_libs" FILES_MATCHING PATTERN "*onn*.dll" PATTERN "*onn*.lib" PATTERN "*.so*")

        message("Packaging [onnxruntime] to ${UNITY_WRAPPER_DLL_DIR}")
        file(COPY "${ONNX_LIB_DIR}/" DESTINATION "${UNITY_WRAPPER_DLL_DIR}/" FILES_MATCHING PATTERN "*onn*.dll" PATTERN "*onn*.lib" PATTERN "*.so*")

        add_definitions(-DUSE_CUDA)
        add_definitions(-DUSE_TENSORRT)
    endif()
else()
    # Windows - try to use system package or fall back to download
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ONNXRUNTIME QUIET onnxruntime)
        if(ONNXRUNTIME_FOUND)
            set(ONNXRUNTIME_ROOTDIR ${ONNXRUNTIME_PREFIX})
            set(ONNX_INCLUDE_DIR ${ONNXRUNTIME_INCLUDE_DIRS})
            set(ONNX_LIB_DIR ${ONNXRUNTIME_LIBRARY_DIRS})
            message("Using system-installed ONNX Runtime")
        else()
            # Fall back to downloading if system package not found
            set(onnx_url https://github.com/microsoft/onnxruntime/releases/download/v1.16.0/onnxruntime-win-x64-gpu-1.16.0.zip)
            set(onnx_hash MD5=dd6d6ba7aaaa58dd9ef8528dfcb822d2)
            message("System ONNX Runtime not found, downloading...")
        endif()
    else()
        set(onnx_url https://github.com/microsoft/onnxruntime/releases/download/v1.16.0/onnxruntime-win-x64-gpu-1.16.0.zip)
        set(onnx_hash MD5=dd6d6ba7aaaa58dd9ef8528dfcb822d2)
        message("PkgConfig not found, downloading ONNX Runtime...")
    endif()
    
    if(DEFINED onnx_url)
        FetchContent_Declare(
            onnxruntime
            URL                  ${onnx_url}
            URL_HASH             ${onnx_hash}
            DOWNLOAD_NO_PROGRESS True
            DOWNLOAD_DIR         ${CMAKE_BINARY_DIR}/_deps/onnxruntime-download
        )
        FetchContent_GetProperties(onnxruntime)

        if(NOT onnxruntime_POPULATED)
            message("Checking if [onnxruntime] dependency needs to be fetched...")
            FetchContent_Populate(onnxruntime)
            set(ONNXRUNTIME_ROOTDIR ${onnxruntime_SOURCE_DIR})
            set(ONNX_INCLUDE_DIR ${onnxruntime_SOURCE_DIR}/include)
            set(ONNX_LIB_DIR  ${onnxruntime_SOURCE_DIR}/lib/)
            message("ONNX_INCLUDE_DIR set to ${ONNX_INCLUDE_DIR}")
        endif()
    endif()
    
    if(ONNX_INCLUDE_DIR AND ONNX_LIB_DIR)
        message("Packaging [onnxruntime] to ${UE_PLUGIN_SIMLIBS_DIR}/shared_libs")
        file(COPY "${ONNX_INCLUDE_DIR}" DESTINATION "${UE_PLUGIN_SIMLIBS_DIR}/shared_libs/onnxruntime")
        file(COPY "${ONNX_LIB_DIR}" DESTINATION "${UE_PLUGIN_SIMLIBS_DIR}/shared_libs" FILES_MATCHING PATTERN "*onn*.dll" PATTERN "*onn*.lib" PATTERN "*.so*")

        message("Packaging [onnxruntime] to ${UNITY_WRAPPER_DLL_DIR}")
        file(COPY "${ONNX_LIB_DIR}/" DESTINATION "${UNITY_WRAPPER_DLL_DIR}/" FILES_MATCHING PATTERN "*onn*.dll" PATTERN "*onn*.lib" PATTERN "*.so*")

        add_definitions(-DUSE_CUDA)
        add_definitions(-DUSE_TENSORRT)
    endif()
endif()

# if(WIN32)
#     # Set up dependency: cesium-native
#     message("Setting up [cesium-native] dependency as an external project...")
#     set (CESIUM_BUILD_TYPE $<IF:$<CONFIG:Debug>,RelWithDebInfo,Release>)
#     ExternalProject_Add(cesium-native
#         GIT_REPOSITORY          https://github.com/CesiumGS/cesium-native.git
#         GIT_TAG                 v0.3.0
#         GIT_CONFIG              "advice.detachedHead=false"
#         GIT_SUBMODULES_RECURSE  True
#         PREFIX                  ${CMAKE_BINARY_DIR}/_deps/cesium-native
#         UPDATE_COMMAND          ""  # disable update step
#         BUILD_COMMAND           ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${CESIUM_BUILD_TYPE}
#         INSTALL_COMMAND         ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${CESIUM_BUILD_TYPE} --target install
#         TEST_COMMAND            ""  # disable test step
#         TEST_EXCLUDE_FROM_MAIN  True
#         CMAKE_ARGS
#             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
#             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#             -DCMAKE_POSITION_INDEPENDENT_CODE=True
#             -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/_deps/cesium-native-install
#             -DCMAKE_INSTALL_MESSAGE=LAZY  # don't print out all files copied
#             -DCMAKE_BUILD_TYPE:STRING=${CESIUM_BUILD_TYPE}  # for single-config generators (Ninja)
#             -DCMAKE_CXX_FLAGS=${CESIUM_CXX_FLAGS}
#             -DCMAKE_EXE_LINKER_FLAGS=${CESIUM_LINKER_FLAGS}
#     )
#     set(CESIUM_LIB_DIR ${CMAKE_BINARY_DIR}/_deps/cesium-native-install/lib)
#     set(CESIUM_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/cesium-native-install/include)
#     set(CESIUM_PLUGIN_THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/unreal/Blocks/Plugins/CesiumForUnreal/Source/ThirdParty)
#     message("CESIUM_LIB_DIR set to ${CESIUM_LIB_DIR}")
#     message("CESIUM_INCLUDE_DIR set to ${CESIUM_INCLUDE_DIR}")

#     ExternalProject_Add_Step(cesium-native post-install
#         COMMAND ${CMAKE_COMMAND} -E echo "Packaging [cesium-native] installed outputs to ${SIMLIBS_PACKAGE_DIR}/cesium-native/$<IF:$<CONFIG:Release>,Release,Debug>"
#         COMMAND ${CMAKE_COMMAND} -E copy_directory "${CESIUM_LIB_DIR}" "${SIMLIBS_PACKAGE_DIR}/cesium-native/$<IF:$<CONFIG:Release>,Release,Debug>"
#         COMMAND ${CMAKE_COMMAND} -E copy_directory "${CESIUM_INCLUDE_DIR}" "${SIMLIBS_PACKAGE_DIR}/cesium-native/include"
#         COMMAND ${CMAKE_COMMAND} -E remove_directory "${SIMLIBS_PACKAGE_DIR}/cesium-native/$<IF:$<CONFIG:Release>,Release,Debug>/cmake"
#         COMMAND ${CMAKE_COMMAND} -E remove_directory "${SIMLIBS_PACKAGE_DIR}/cesium-native/$<IF:$<CONFIG:Release>,Release,Debug>/pkgconfig"
#         COMMAND ${CMAKE_COMMAND} -E echo "Packaging [cesium-native] installed outputs to ${CESIUM_PLUGIN_THIRDPARTY_DIR}"
#         COMMAND ${CMAKE_COMMAND} -E copy_directory "${SIMLIBS_PACKAGE_DIR}/cesium-native/$<IF:$<CONFIG:Release>,Release,Debug>" "${CESIUM_PLUGIN_THIRDPARTY_DIR}/lib/$<IF:$<CONFIG:Release>,Release,Debug>"
#         COMMAND ${CMAKE_COMMAND} -E copy_directory "${SIMLIBS_PACKAGE_DIR}/cesium-native/include" "${CESIUM_PLUGIN_THIRDPARTY_DIR}/include"
#         COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/cesium-native/src/cesium-native/extern/stb/stb_image_resize.h" "${CESIUM_PLUGIN_THIRDPARTY_DIR}/include"
#         DEPENDEES install
#     )
# endif()

# Add sub-projects
add_subdirectory(core_sim)
add_subdirectory(simserver)
add_subdirectory(samples)
add_subdirectory(physics)
add_subdirectory(mavlinkcom)
add_subdirectory(rendering)
add_subdirectory(tools)
add_subdirectory(unity)
add_subdirectory(vehicle_apis)

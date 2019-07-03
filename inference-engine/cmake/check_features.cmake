# Copyright (C) 2018-2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

include("features")
include("mode")
include("itt")

#64 bits platform
if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    message(STATUS "Detected 64 bit architecture")
    SET(ARCH_64 ON)
    SET(ARCH_32 OFF)
else()
    message(STATUS "Detected 32 bit architecture")
    SET(ARCH_64 OFF)
    SET(ARCH_32 ON)
endif()

if (ARCH_64)
else()
    if (UNIX OR APPLE)
        SET(ENABLE_CLDNN OFF)
    endif()
    SET(ENABLE_MKL_DNN OFF)
endif()

#apple specific
if (APPLE)
    set(ENABLE_GNA OFF)
    set(ENABLE_CLDNN OFF)
    SET(ENABLE_MYRIAD OFF)
endif()


#minGW specific - under wine no support for downloading file and applying them using git
if (WIN32)
    if (MINGW)
        SET(ENABLE_CLDNN OFF) # dont have mingw dll for linking
        set(ENABLE_SAMPLES OFF)
    endif()
endif()

# Linux specific - not all OS'es are supported
if (LINUX)
    include("linux_name")
    get_linux_name(LINUX_OS_NAME)
    if (LINUX_OS_NAME)
        if (NOT(
                ${LINUX_OS_NAME} STREQUAL "Ubuntu 14.04" OR
                ${LINUX_OS_NAME} STREQUAL "Ubuntu 16.04" OR
                ${LINUX_OS_NAME} STREQUAL "CentOS 7"))
        endif()
    else ()
        message(WARNING "Cannot detect Linux OS via reading /etc/*-release:\n ${release_data}")
    endif ()
endif ()

if (NOT ENABLE_MKL_DNN)
    set(ENABLE_MKL OFF)
endif()

if (NOT ENABLE_VPU)
    set(ENABLE_MYRIAD OFF)
endif()

if (NOT ENABLE_MYRIAD)
    set(ENABLE_VPU OFF)
endif()

#next section set defines to be accesible in c++/c code for certain feature
if (ENABLE_PROFILING_RAW)
    add_definitions(-DENABLE_PROFILING_RAW=1)
endif()

if (ENABLE_CLDNN)
    add_definitions(-DENABLE_CLDNN=1)
endif()

if (ENABLE_MYRIAD)
    add_definitions(-DENABLE_MYRIAD=1)
endif()

if (ENABLE_MYX_PCIE AND ENABLE_MYRIAD)
    add_definitions(-DENABLE_MYX_PCIE=1)
endif()

if (ENABLE_MYRIAD_NO_BOOT AND ENABLE_MYRIAD )
    add_definitions(-DENABLE_MYRIAD_NO_BOOT=1)
endif()

if (ENABLE_MYX_PCIE AND ENABLE_MYRIAD_NO_BOOT)
    message(FATAL_ERROR "ENABLE_MYX_PCIE and ENABLE_MYRIAD_NO_BOOT can't be enabled at the same time")
endif()

if (ENABLE_MKL_DNN)
    add_definitions(-DENABLE_MKL_DNN=1)
endif()

if (ENABLE_GNA)
    add_definitions(-DENABLE_GNA)
endif()

if (ENABLE_SAMPLES)
    set (ENABLE_SAMPLES_CORE ON)
endif()

if (DEVELOPMENT_PLUGIN_MODE)
    message (STATUS "Enabled development plugin mode")

    set (ENABLE_MKL_DNN OFF)
    set (ENABLE_TESTS OFF)

    message (STATUS "Initialising submodules")
    execute_process (COMMAND git submodule update --init ${IE_MAIN_SOURCE_DIR}/thirdparty/pugixml
                     RESULT_VARIABLE git_res)

    if (NOT ${git_res})
        message (STATUS "Initialising submodules - done")
    endif()
endif()

if (VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE  ON)
endif()

print_enabled_features()

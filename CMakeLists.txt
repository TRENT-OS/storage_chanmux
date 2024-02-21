#
# ChanMux Storage driver
# 
# Copyright (C) 2020-2024, HENSOLDT Cyber GmbH 
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
#

cmake_minimum_required(VERSION 3.17)

# This file is included by the SDK's CMakeLists.txt, so we are running in its
# scope here and not in our own scope. As a consequence, any relative paths
# given here aren't relative to the directory this file is located in, but
# relative to whoever includes us.

#-------------------------------------------------------------------------------
# the raw lib for the proxy NVM channel
project(proxy_nvm C)

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME}
    INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/proxy_nvm/ProxyNVM.c"
)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/proxy_nvm/include"
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        lib_utils
        lib_mem
        chanmux_client
)

#-------------------------------------------------------------------------------
# the raw driver without the CAmkES wrapper
project(ChanMuxNvmDriver C)

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME}
    INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/ChanMuxNvmDriver/ChanMuxNvmDriver.c"
)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/ChanMuxNvmDriver/include"
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        proxy_nvm
)


#-------------------------------------------------------------------------------
# Declare ChanMux Storage Driver CAmkES Component
#
# Parameters:
#
#   <name>
#     required, component instance name
#
function(Storage_ChanMux_DeclareCAmkESComponent
    name
)

    DeclareCAmkESComponent(
        ${name}
        SOURCES
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/Storage_ChanMux.c
        C_FLAGS
            -Wall -Werror
        LIBS
            os_core_api
            lib_debug
            ChanMuxNvmDriver
    )

endfunction()

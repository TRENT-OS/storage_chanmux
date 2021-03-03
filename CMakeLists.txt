#
# ChanMux Storage driver
#
# Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.17)

# This file is included by the SDK's CMakeLists.txt, so we are running in its
# scope here and not in our own scope. As a consequence, any relative paths
# given here aren't relative to the directory this file is located in, but
# relative to whoever includes us.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ProxyNvm")

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
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ChanMuxNvmDriver.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/Storage_ChanMux.c
        C_FLAGS
            -Wall -Werror
        LIBS
            os_core_api
            lib_debug
            proxy_nvm
    )

endfunction()

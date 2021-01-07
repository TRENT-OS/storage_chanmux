#
# ChanMux Storage driver
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.17)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/ProxyNvm)

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
            os_libs
            proxy_nvm
    )

endfunction()

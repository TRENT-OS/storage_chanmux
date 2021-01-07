#
# ProxyNvm object
#
# Copyright (C) 2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.17)

# enable new behavior introduced in CMake 3.13, where relative paths will be
# converted into absolute paths in target_sources(INTERFACE ....)
cmake_policy(SET CMP0076 NEW)

#-------------------------------------------------------------------------------
project(proxy_nvm C)

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME}
    INTERFACE
        "ProxyNVM.c"
)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        "."
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        os_core_api
        os_libs
        chanmux_client
)

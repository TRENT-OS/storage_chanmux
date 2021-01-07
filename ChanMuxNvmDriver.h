/* Copyright (C) 2020, Hensoldt Cyber GmbH */
/**
 * @file
 * @brief   ChanMX NVM driver
 */

#pragma once

#include "LibMem/Nvm.h"
#include "ChanMux/ChanMuxClient.h"
#include "ProxyNVM.h"

#include <limits.h>

typedef struct {
    ProxyNVM        proxyNVM;
    char            proxyBuffer[PAGE_SIZE];

    ChanMuxClient   chanMuxClient;
} ChanMuxNvmDriver;


bool
ChanMuxNvmDriver_ctor(
    ChanMuxNvmDriver*             self,
    const ChanMuxClientConfig_t*  config);


void
ChanMuxNvmDriver_dtor(
    ChanMuxNvmDriver*  self);


Nvm*
ChanMuxNvmDriver_get_nvm(
    ChanMuxNvmDriver*  self);

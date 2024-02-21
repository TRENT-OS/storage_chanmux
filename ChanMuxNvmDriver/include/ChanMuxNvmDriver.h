/* 
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH 
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

/**
 * @file
 * @brief   ChanMX NVM driver
 */

#pragma once

#include "ChanMux/ChanMuxClient.h"
#include "ProxyNVM.h"

#include <limits.h> // needed to get PAGE_SIZE

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

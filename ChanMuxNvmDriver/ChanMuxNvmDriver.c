/*
 * ChanMX NVM driver
 * 
 * Copyright (C) 2020-2024, HENSOLDT Cyber GmbH 
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#include "ChanMuxNvmDriver.h"

#include "ChanMux/ChanMuxClient.h"
#include "ProxyNVM.h"

#include <stdint.h>
#include <stdbool.h>


//------------------------------------------------------------------------------
bool
ChanMuxNvmDriver_ctor(
    ChanMuxNvmDriver*             self,
    const ChanMuxClientConfig_t*  config)
{
    // initialise ChanMux client
    if (!ChanMuxClient_ctor(&(self->chanMuxClient), config))
    {
        Debug_LOG_ERROR("ChanMuxClient_ctor() failed");
        return false;
    }

    // initialise the Proxy-NVM driver library.
    if (!ProxyNVM_ctor(
            &(self->proxyNVM),
            &(self->chanMuxClient),
            self->proxyBuffer,
            sizeof(self->proxyBuffer)))
    {
        Debug_LOG_ERROR("ProxyNVM_ctor() failed");
        return false;
    }

    return true;
}


//------------------------------------------------------------------------------
void
ChanMuxNvmDriver_dtor(
    ChanMuxNvmDriver*  self)
{
    ProxyNVM_dtor( ProxyNVM_TO_NVM( &(self->proxyNVM) ) );
    ChanMuxClient_dtor( &(self->chanMuxClient) );
}


//------------------------------------------------------------------------------
Nvm*
ChanMuxNvmDriver_get_nvm(
    ChanMuxNvmDriver*  self)
{
    return ProxyNVM_TO_NVM( &(self->proxyNVM) );
}

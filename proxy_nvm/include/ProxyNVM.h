/* Copyright (C) 2020, HENSOLDT Cyber GmbH */
/**
 * @addtogroup OS
 * @{
 *
 * @file
 *
 * @brief a implementation of the LibMem/Nvm.h interface using the proxy
 *  NVM. The linux proxy application provides facilities (like NVM or
 *  network sockets) that the current state of the OS for one reason or another
 *  cannot provide natively. The OS-linux communication happens on a channel
 *  like a serial port.
 *
 */
#pragma once

/* Includes ------------------------------------------------------------------*/

#include "lib_mem/Nvm.h"
#include "ChanMux/ChanMuxClient.h"


/* Exported macro ------------------------------------------------------------*/

#define ProxyNVM_TO_NVM(self) (&(self)->parent)

#define COMMAND_GET_SIZE    0x00
#define COMMAND_WRITE       0x01
#define COMMAND_READ        0x02


/* Exported types ------------------------------------------------------------*/

typedef struct ProxyNVM ProxyNVM;

struct ProxyNVM
{
    Nvm parent;
    ChanMuxClient* chanmux;
    char* msgBuf;
    size_t msgBufSize;
};


/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
 * @brief constructor.
 *
 * @return true if success
 *
 */
bool
ProxyNVM_ctor(ProxyNVM* self, ChanMuxClient* chanmux, char* msgBufer,
              size_t msgBufersize);
/**
 * @brief static implementation of virtual method NVM_write().
 *
 */
size_t
ProxyNVM_write(Nvm* nvm, size_t addr, void const* buffer, size_t length);
/**
 * @brief static implementation of virtual method NVM_read()
 *
 */
size_t
ProxyNVM_read(Nvm* nvm, size_t addr, void* buffer, size_t length);
/**
 * @brief static implementation of the erase method that is required
 * when working with flash
 *
 */
size_t
ProxyNVM_erase(Nvm* nvm, size_t addr, size_t length);
/**
 * @brief static implementation of virtual method NVM_getSize()
 *
 */
size_t
ProxyNVM_getSize(Nvm* nvm);

void
ProxyNVM_dtor(Nvm* nvm);

///@}

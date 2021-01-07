/*
 *  Copyright (C) 2018, Hensoldt Cyber GmbH
 */
/* Includes ------------------------------------------------------------------*/
#include "ProxyNVM.h"
#include <string.h>
#include <stdio.h>

#include "LibUtil/BitConverter.h"

/*---------------PROTOCOL--------------------*/
/*
Commands:
    0 -> getSize
    1 -> write
    2 -> read

Retval:
    0 -> OK
    -1 -> GENERIC_ERROR
    ...

-----------------GetSize------------------------
Request
    [Command=0]
Response
    [Command=0][Retval][SIZE_0|SIZE_1|SIZE_2|SIZE_3]


Example: Get the capacity of the NVM
Request
[0]
Response
[0][0][0|0|0|128]

------------------Write---------------------------
Request
    [Command=1][ADDR_0|ADDR_1|ADDR_2|ADDR_3][LENGTH_0|LENGTH_1|LENGTH_2|LENGTH_3][...|...]
Response
    [Command=1][Retval][WRITTEN_0|WRITTEN_1|WRITEN_2|WRITTEN_3]


Example: Write 2 bytes 0xAA and 0x55 from address 0x02
Request
    [1][0x00000002][0|0|0|2][0xAA][0x55]
Response
    [1][0][0|0|0|2]

-------------------Read----------------------------
Request
    [Command=2][ADDR_0|ADDR_1|ADDR_2|ADDR_3][LENGTH_0|LENGTH_1|LENGTH_2|LENGTH_3]
Response
    [Command=2][Retval][READ_0|READ_1|READ_2|READ_3][...|...]


Example: Read 2 bytes from address 0x02
Request
    [1][0x00000002][0|0|0|2]
Response
    [2][0][0|0|0|2][0xAA][0x55]
*/
/*---------------PROTOCOL--------------------*/

/* Defines -------------------------------------------------------------------*/
#define HDLC_HEADER             10
#define MAX_MSG_LEN             (self->msgBufSize - HDLC_HEADER)
#define REQUEST_HEADER_LEN      9
#define RESP_HEADER_LEN         6
#define MAX_REQ_PAYLOAD_LEN     (MAX_MSG_LEN - REQUEST_HEADER_LEN)
#define MAX_RESP_PAYLOAD_LEN    (MAX_MSG_LEN - RESP_HEADER_LEN)
#define ADDRESS_SIZE            4 //number of bytes for the address in the protocol
#define LENGTH_SIZE             4 //number of bytes for the length in the protocol

//INDEXES OF DIFFERENT PARTS OF THE REQUEST MESSAGE (IN A BUFFER)
#define REQ_COMM_INDEX          0
#define REQ_ADDR_INDEX          1
#define REQ_LEN_INDEX           5
#define REQ_PAYLD_INDEX         9

//INDEXES OF DIFFERENT PARTS OF THE RESPONSE MESSAGE (IN A BUFFER)
#define RESP_COMM_INDEX         0
#define RESP_RETVAL_INDEX       1
#define RESP_BYTES_INDEX        2
#define RESP_PAYLD_INDEX        6

//RETURN MESSAGES
#define RET_OK                  0
#define RET_GENERIC_ERR         -1
#define RET_FILE_OPEN_ERR       -2
#define RET_WRITE_ERR           -3
#define RET_READ_ERR            -4
#define RET_LEN_OUT_OF_BOUNDS   -5
#define RET_ADDR_OUT_OF_BOUNDS  -6

/* Private functions prototypes ----------------------------------------------*/
static void constructMsg(uint8_t command, size_t addr, size_t length,
                         char const* buffer, char* message);
static void logError(int8_t err, const char* func);

static
bool
isValidStorageArea(
    Nvm* nvm,
    size_t const offset,
    size_t const size);

/* Private variables ---------------------------------------------------------*/

static const Nvm_Vtable ProxyNvm_vtable =
{
    .read       = ProxyNVM_read,
    .erase      = ProxyNVM_erase,
    .getSize    = ProxyNVM_getSize,
    .write      = ProxyNVM_write,
    .dtor       = ProxyNVM_dtor
};

/* Public functions ----------------------------------------------------------*/

bool ProxyNVM_ctor(ProxyNVM* self, ChanMuxClient* chanmux, char* msgBufer,
                   size_t msgBufersize)
{
    Debug_ASSERT_SELF(self);
    bool retval = true;
    Nvm* nvm = ProxyNVM_TO_NVM(self);

    nvm->vtable = &ProxyNvm_vtable;
    self->chanmux = chanmux;
    self->msgBuf = msgBufer;
    self->msgBufSize = msgBufersize;

    return retval;
}

size_t ProxyNVM_write(Nvm* nvm, size_t addr, void const* buffer, size_t length)
{
    ProxyNVM* self = (ProxyNVM*) nvm;
    Debug_ASSERT_SELF(self);
    Debug_ASSERT(buffer != NULL);

    if(!isValidStorageArea(nvm, addr, length))
    {
        Debug_LOG_ERROR(
            "%s: Unable to write to the given area (out of bounds): "
            "addr = %u, length = %u",
            __func__,
            addr,
            length);

        return 0;
    }

    size_t writtenTotal = 0;
    size_t bytes = 0;
    size_t confirmedWritten = 0;

    while (length > MAX_REQ_PAYLOAD_LEN)
    {
        constructMsg(COMMAND_WRITE, addr, MAX_REQ_PAYLOAD_LEN,
                     &((char*)buffer)[writtenTotal], self->msgBuf);
        ChanMuxClient_write(self->chanmux, self->msgBuf, MAX_MSG_LEN, &bytes);
        ChanMuxClient_read(self->chanmux, self->msgBuf, RESP_HEADER_LEN, &bytes);
        confirmedWritten = BitConverter_getUint32BE(&self->msgBuf[RESP_BYTES_INDEX]);

        if (self->msgBuf[RESP_RETVAL_INDEX] != RET_OK)
        {
            logError(self->msgBuf[RESP_RETVAL_INDEX], __func__);
            return 0;
        }
        if (confirmedWritten != MAX_REQ_PAYLOAD_LEN)
        {
            Debug_LOG_ERROR("%s: Tried to write %zu bytes, but successfully written %zu",
                            __func__, MAX_REQ_PAYLOAD_LEN, confirmedWritten);
            return 0;
        }

        writtenTotal += confirmedWritten;

        addr += confirmedWritten;
        length -= MAX_REQ_PAYLOAD_LEN;
    }

    if (length > 0)
    {
        constructMsg(COMMAND_WRITE, addr, length, &((char*)buffer)[writtenTotal],
                     self->msgBuf);
        ChanMuxClient_write(self->chanmux, self->msgBuf, length + REQUEST_HEADER_LEN,
                            &bytes);
        ChanMuxClient_read(self->chanmux, self->msgBuf, RESP_HEADER_LEN, &bytes);
        confirmedWritten = BitConverter_getUint32BE(&self->msgBuf[RESP_BYTES_INDEX]);

        if (self->msgBuf[RESP_RETVAL_INDEX] != RET_OK)
        {
            logError(self->msgBuf[RESP_RETVAL_INDEX], __func__);
            return 0;
        }
        if (confirmedWritten != length)
        {
            Debug_LOG_ERROR("%s: Tried to write %zu bytes, but successfully written %zu",
                            __func__, length, confirmedWritten);
            return 0;
        }

        writtenTotal += confirmedWritten;
    }

    return writtenTotal;
}

size_t ProxyNVM_read(Nvm* nvm, size_t addr, void* buffer, size_t length)
{
    ProxyNVM* self = (ProxyNVM*) nvm;
    Debug_ASSERT_SELF(self);
    Debug_ASSERT(buffer != NULL);

    if(!isValidStorageArea(nvm, addr, length))
    {
        Debug_LOG_ERROR(
            "%s: Unable to write to the given area (out of bounds): "
            "addr = %u, length = %u",
            __func__,
            addr,
            length);

        return 0;
    }

    size_t readTotal = 0;
    size_t bytes = 0;
    size_t confirmedRead = 0;

    while (length >= MAX_RESP_PAYLOAD_LEN)
    {
        constructMsg(COMMAND_READ, addr, MAX_RESP_PAYLOAD_LEN, NULL, self->msgBuf);
        ChanMuxClient_write(self->chanmux, self->msgBuf, REQUEST_HEADER_LEN, &bytes);
        ChanMuxClient_read(self->chanmux, self->msgBuf, MAX_MSG_LEN, &bytes);
        confirmedRead = BitConverter_getUint32BE(&self->msgBuf[RESP_BYTES_INDEX]);

        if (self->msgBuf[RESP_RETVAL_INDEX] != RET_OK)
        {
            logError(self->msgBuf[RESP_RETVAL_INDEX], __func__);
            return 0;
        }
        if (confirmedRead != MAX_RESP_PAYLOAD_LEN)
        {
            Debug_LOG_ERROR("%s: Tried to read %zu bytes, but successfully read %zu",
                            __func__, MAX_RESP_PAYLOAD_LEN, confirmedRead);
            return 0;
        }

        memcpy(&((char*)buffer)[readTotal], &self->msgBuf[6], confirmedRead);
        readTotal += confirmedRead;

        addr += (confirmedRead);
        length -= (MAX_RESP_PAYLOAD_LEN);
    }

    if (length > 0)
    {
        constructMsg(COMMAND_READ, addr, length, NULL, self->msgBuf);
        ChanMuxClient_write(self->chanmux, self->msgBuf, REQUEST_HEADER_LEN, &bytes);
        ChanMuxClient_read(self->chanmux, self->msgBuf, length + RESP_HEADER_LEN,
                           &bytes);
        confirmedRead = BitConverter_getUint32BE(&self->msgBuf[RESP_BYTES_INDEX]);

        if (self->msgBuf[RESP_RETVAL_INDEX] != RET_OK)
        {
            logError(self->msgBuf[RESP_RETVAL_INDEX], __func__);
            return 0;
        }
        if (confirmedRead != length)
        {
            Debug_LOG_ERROR("%s: Tried to read %zu bytes, but successfully read %zu!",
                            __func__, length, confirmedRead);
            return 0;
        }

        memcpy(&((char*)buffer)[readTotal], &self->msgBuf[6], confirmedRead);
        readTotal += confirmedRead;
    }

    return readTotal;
}

size_t ProxyNVM_erase(Nvm* nvm, size_t addr, size_t length)
{
    ProxyNVM* self = (ProxyNVM*) nvm;
    Debug_ASSERT_SELF(self);

    if(!isValidStorageArea(nvm, addr, length))
    {
        Debug_LOG_ERROR(
            "%s: Unable to write to the given area (out of bounds): "
            "addr = %u, length = %u",
            __func__,
            addr,
            length);

        return 0;
    }

    size_t erasedTotal = 0;
    size_t bytes = 0;
    size_t confirmedErased = 0;

    while (length > MAX_REQ_PAYLOAD_LEN)
    {
        constructMsg(COMMAND_WRITE, addr, MAX_REQ_PAYLOAD_LEN, NULL, self->msgBuf);
        ChanMuxClient_write(self->chanmux, self->msgBuf, MAX_MSG_LEN, &bytes);
        ChanMuxClient_read(self->chanmux, self->msgBuf, RESP_HEADER_LEN, &bytes);
        confirmedErased = BitConverter_getUint32BE(&self->msgBuf[RESP_BYTES_INDEX]);

        if (self->msgBuf[RESP_RETVAL_INDEX] != RET_OK)
        {
            logError(self->msgBuf[RESP_RETVAL_INDEX], __func__);
            return 0;
        }
        if (confirmedErased != MAX_REQ_PAYLOAD_LEN)
        {
            Debug_LOG_ERROR("%s: Tried to erase %zu bytes, but successfully erased %zu",
                            __func__, MAX_REQ_PAYLOAD_LEN, confirmedErased);
            return 0;
        }

        erasedTotal += confirmedErased;

        addr += confirmedErased;
        length -= MAX_REQ_PAYLOAD_LEN;
    }

    if (length > 0)
    {
        constructMsg(COMMAND_WRITE, addr, length, NULL, self->msgBuf);
        ChanMuxClient_write(self->chanmux, self->msgBuf, length + REQUEST_HEADER_LEN,
                            &bytes);
        ChanMuxClient_read(self->chanmux, self->msgBuf, RESP_HEADER_LEN, &bytes);
        confirmedErased = BitConverter_getUint32BE(&self->msgBuf[RESP_BYTES_INDEX]);

        if (self->msgBuf[RESP_RETVAL_INDEX] != RET_OK)
        {
            logError(self->msgBuf[RESP_RETVAL_INDEX], __func__);
            return 0;
        }
        if (confirmedErased != length)
        {
            Debug_LOG_ERROR("%s: Tried to erase %zu bytes, but successfully erased %zu",
                            __func__, length, confirmedErased);
            return 0;
        }

        erasedTotal += confirmedErased;
    }

    return erasedTotal;
}

size_t ProxyNVM_getSize(Nvm* nvm)
{
    ProxyNVM* self = (ProxyNVM*) nvm;
    Debug_ASSERT_SELF(self);

    size_t size = 0;
    size_t bytes = 0;
    constructMsg(COMMAND_GET_SIZE, 0, 0, NULL, self->msgBuf);
    ChanMuxClient_write(self->chanmux, self->msgBuf, 1, &bytes);

    ChanMuxClient_read(self->chanmux, self->msgBuf, RESP_HEADER_LEN, &bytes);

    if (self->msgBuf[RESP_RETVAL_INDEX] != RET_OK)
    {
        logError(self->msgBuf[RESP_RETVAL_INDEX], __func__);
    }

    size = BitConverter_getUint32BE(&self->msgBuf[RESP_BYTES_INDEX]);

    return size;
}

void ProxyNVM_dtor(Nvm* nvm)
{
    DECL_UNUSED_VAR(ProxyNVM * self) = (ProxyNVM*) nvm;
    Debug_ASSERT_SELF(self);
}

/* Private functions ---------------------------------------------------------*/
static
bool
isValidStorageArea(
    Nvm* nvm,
    size_t const offset,
    size_t const size)
{
    size_t const end = offset + size;
    // Checking integer overflow first. The end index is not part of the area,
    // but we allow offset = end with size = 0 here
    return ( (end >= offset) && (end <= ProxyNVM_getSize(nvm)) );
}

static void constructMsg(uint8_t command, size_t addr, size_t length,
                         char const* buffer, char* message)
{
    if (sizeof(addr) > ADDRESS_SIZE)
    {
        Debug_LOG_WARNING("%s: Passed address is %zu bytes, but the size of the address in the protocol is %d bytes. The address will be truncated!",
                          __func__, sizeof(addr), ADDRESS_SIZE);
    }

    if (sizeof(length) > LENGTH_SIZE)
    {
        Debug_LOG_WARNING("%s: Passed length is %zu bytes, but the size of the length in the protocol is %d bytes. The length will be truncated!",
                          __func__, sizeof(addr), ADDRESS_SIZE);
    }

    message[REQ_COMM_INDEX] = command;
    BitConverter_putUint32BE((uint32_t) addr, &message[REQ_ADDR_INDEX]);
    BitConverter_putUint32BE((uint32_t) length, &message[REQ_LEN_INDEX]);

    if (command == COMMAND_WRITE)
    {
        if (buffer != NULL)
        {
            //real write command
            memcpy(&(message[REQ_PAYLD_INDEX]), buffer, length);
        }
        else
        {
            //writing 0xFF => erase command
            memset(&(message[REQ_PAYLD_INDEX]), 0xFF, length);
        }
    }
}

static void logError(int8_t err, const char* func)
{
    switch (err)
    {
    case RET_FILE_OPEN_ERR:
        Debug_LOG_ERROR("%s: Operation failed, error: FILE OPEN ERROR", func);
        break;

    case RET_WRITE_ERR:
        Debug_LOG_ERROR("%s: Operation failed, error: WRITE ERROR", func);
        break;

    case RET_READ_ERR:
        Debug_LOG_ERROR("%s: Operation failed, error: READ ERROR", func);
        break;

    case RET_LEN_OUT_OF_BOUNDS:
        Debug_LOG_ERROR("%s: Operation failed, error: LENGTH OUT OF BOUNDS", func);
        break;

    case RET_ADDR_OUT_OF_BOUNDS:
        Debug_LOG_ERROR("%s: Operation failed, error: ADDRESS OUT OF BOUNDS", func);
        break;

    default:
        Debug_LOG_ERROR("%s: Operation failed, error: GENERIC ERROR", func);
        break;
    }
}

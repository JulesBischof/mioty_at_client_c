#include "serial_stub.h"
#include <stdio.h>
#include <string.h>

#include "unity.h"

#define STATIC_BUFFERSIZE (256)

static char write_buffer[STATIC_BUFFERSIZE] = {0};
static uint32_t write_buffer_watermark = 0;

static char read_buffer[STATIC_BUFFERSIZE] = {0};
static uint32_t read_buffer_watermark = 0;

static bool shall_trigger_error = false;

bool miotyAtClientWrite(uint8_t *msg, uint16_t msg_len)
{
    if (shall_trigger_error)
    {
        return false;
    }

    if (msg == NULL)
    {
        printf("[miotyAtClientWrite] - BUFFER WAS NULL");
        TEST_ASSERT(false);
    }

    if (msg_len == 0)
    {
        printf("[miotyAtClientWrite] - buffer_len WAS 0");
        TEST_ASSERT(false);
    }

    if ((write_buffer_watermark + msg_len) > STATIC_BUFFERSIZE)
    {
        printf("WRITER TEST BUFFER IS TOO SMALL - unit wants to write %d Bytes", msg_len);
        TEST_ASSERT(false);
    }

    write_buffer_watermark += msg_len;
    memcpy(write_buffer, msg, msg_len);
    return true;
}

bool miotyAtClientRead(uint8_t *buffer, uint8_t buffer_len, uint8_t *p_len)
{
    if (shall_trigger_error)
    {
        return false;
    }

    if (buffer == NULL)
    {
        printf("[miotyAtClientRead] BUFFER WAS NULL");
        TEST_ASSERT(false);
    }

    if (p_len == NULL)
    {
        printf("[miotyAtClientRead] P_LEN WAS NULL");
        TEST_ASSERT(false);
    }

    if (buffer_len < read_buffer_watermark)
    {
        printf("[miotyAtClientRead] PROVIDED BUFFER WAS TOO SMALL ");
        TEST_ASSERT(false);
    }

    memcpy(buffer, read_buffer, read_buffer_watermark);
    *p_len = read_buffer_watermark;
    return true;
}

// getters / setters

void serial_stub_get_write_buffer(char *buffer, uint8_t len, uint32_t *watermark)
{
    if (write_buffer_watermark == 0)
    {
        printf("WRITE BUFFER WAS EMPTY");
        TEST_ASSERT(false);
    }

    if (write_buffer_watermark > len)
    {
        printf("PROVIDE BIGGER BUFFER");
        TEST_ASSERT(false);
    }

    if (buffer == NULL)
    {
        printf("BUFFER IS NULL");
        TEST_ASSERT(false);
    }

    *watermark = write_buffer_watermark;
    memcpy(buffer, write_buffer, write_buffer_watermark);
}

void serial_stub_set_read_buffer(char *buffer, uint8_t len)
{
    if (buffer == NULL)
    {
        printf("BUFFER IS NULL");
        TEST_ASSERT(false);
    }

    if (len > STATIC_BUFFERSIZE)
    {
        printf("PROVIDE BIGGER READ BUFFER");
        TEST_ASSERT(false);
    }

    read_buffer_watermark += len;
    memcpy(read_buffer, buffer, len);
}

void serial_stub_next_call_shall_trigger_error(bool flag)
{
    shall_trigger_error = flag;
}
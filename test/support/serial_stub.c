#include "serial_stub.h"
#include <string.h>

#include "unity.h"

typedef struct ReadVectorEntry_t
{
    uint8_t buffer[BUFFERSIZE];
    size_t len;
} ReadVectorEntry_t;

static char write_buffer[BUFFERSIZE] = {0};
static uint32_t write_buffer_watermark = 0;

static ReadVectorEntry_t read_vector[READ_VECTOR_LENGTH] = {0};
static size_t read_vector_read_idx = 0;
static size_t read_vector_write_idx = 0;

static bool shall_trigger_error = false;

bool miotyAtClientWrite(uint8_t *msg, uint16_t msg_len)
{
    if (shall_trigger_error)
    {
        return false;
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(msg, "[miotyAtClientWrite] msg was NULL");
    TEST_ASSERT_MESSAGE(msg_len, "[miotyAtClientWrite] msg_len was 0");
    TEST_ASSERT_MESSAGE(BUFFERSIZE > (write_buffer_watermark + msg_len), "[miotyAtClientwrite] Write buffer is to small, increase BUFFERSIZE");

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

    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "[miotyAtClientRead] BUFFER was NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(p_len, "[miotyAtClientRead] BUFFER was NULL");
    TEST_ASSERT_MESSAGE(read_vector[read_vector_read_idx].len <= buffer_len, "[miotyAtClientRead] Provided buffer is too small");
    TEST_ASSERT_MESSAGE(read_vector_read_idx < read_vector_write_idx, "[miotyAtClientRead] read vector is EMPTY");

    memcpy(buffer, read_vector[read_vector_read_idx].buffer, read_vector[read_vector_read_idx].len);
    *p_len = read_vector[read_vector_read_idx].len;

    read_vector_read_idx++;

    return true;
}

// getters / setters

void serial_stub_get_write_buffer(char *buffer, uint8_t len, uint32_t *watermark)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "[serial_stub_get_write_buffer] BUFFER was NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(watermark, "[serial_stub_get_write_buffer] watermark was NULL");
    TEST_ASSERT_MESSAGE(write_buffer_watermark <= len, "[serial_stub_get_write_buffer] provided buffer was too small");

    *watermark = write_buffer_watermark;
    memcpy(buffer, write_buffer, write_buffer_watermark);
}

void serial_stub_push_to_read_vector(char *buffer, uint8_t len)
{
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_MESSAGE(len, "provided length was 0");
    TEST_ASSERT_MESSAGE(BUFFERSIZE > len, "increase BUFFERSIZE - message to push is too long");

    memcpy(read_vector[read_vector_write_idx].buffer, buffer, len);
    read_vector[read_vector_write_idx].len = len;
    read_vector_write_idx++;
}

void serial_stub_next_call_shall_trigger_error(bool flag)
{
    shall_trigger_error = flag;
}

void serial_stub_reset(void)
{
    memset(read_vector, 0, sizeof(read_vector));
    read_vector_read_idx = 0;
    read_vector_write_idx = 0;
}
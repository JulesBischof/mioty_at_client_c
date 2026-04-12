// #ifdef TEST

#include "unity.h"
#include "serial_stub.h"
#include "miotyAtClient.h"
#include <string.h>

#include "char_tools.h"
#include "string_tools.h"

typedef struct ReadVectorEntry_t
{
    char *buffer;
    uint8_t len;
} ReadVectorEntry_t;

/* ============================
 * TEST inits
 * ============================ */

void setUp(void)
{
    serial_stub_reset();
}

void tearDown(void)
{
}

/* ============================
 * TEST utils
 * ============================ */

/* ============================
 * TEST local stubs
 * ============================ */
void miotyAtClientOnIdle(uint32_t message_len)
{
    printf("IDLE cb");
}

void miotyAtClientTx_start_cb(void)
{
}

void miotyAtclientTx_stop_cb(void)
{
}

/* ============================
 * TEST suites
 * ============================ */

void test_at_client_formats_uni_message_correctly_and_extracts_the_packet_counter(void)
{
    // given that
    char send_msg[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // fill read buffer
    ReadVectorEntry_t read_vector[] =
        {
            {"-MPCT:42\r\n", 10},
            {"-TXA:1\r\n", 8},
            {"-TXA:0\r\n0\r\n", 11},
            {NULL, 0}};

    for (uint32_t i = 0; read_vector[i].buffer != NULL; i++)
    {
        serial_stub_push_to_read_vector(read_vector[i].buffer, read_vector[i].len);
    }

    // run
    uint32_t packet_counter = 0;
    miotyAtClient_returnCode r = miotyAtClient_sendMessageUni(send_msg, sizeof(send_msg), &packet_counter);

    // then
    uint8_t expectation[] = "AT-U=8\t0123456789ABCDEF\x1A\r";

    char result[sizeof(expectation) - 1] = {0}; // -1 due to \0 termination of expectation
    uint32_t result_len;
    serial_stub_get_write_buffer(result, sizeof(result), &result_len);

    TEST_ASSERT_EQUAL(MIOTYATCLIENT_RETURN_CODE_OK, r);
    TEST_ASSERT_EQUAL(42, packet_counter);
    TEST_ASSERT_EQUAL(result_len, sizeof(result));
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expectation, result, result_len);
}

void test_at_client_formats_uni_mpf_message_correctly_and_extracts_the_packet_counter(void)
{
    // given that
    char send_msg[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // fill read buffer
    ReadVectorEntry_t read_vector[] =
        {
            {"-MPCT:42\r\n", 10},
            {"-TXA:1\r\n", 8},
            {"-TXA:0\r\n0\r\n", 11},
            {NULL, 0}};

    for (uint32_t i = 0; read_vector[i].buffer != NULL; i++)
    {
        serial_stub_push_to_read_vector(read_vector[i].buffer, read_vector[i].len);
    }

    // run
    uint32_t packet_counter = 0;
    miotyAtClient_returnCode r = miotyAtClient_sendMessageUniMPF(send_msg, sizeof(send_msg), &packet_counter);

    // then
    uint8_t expectation[] = "AT-UMPF=8\t0123456789ABCDEF\x1A\r";

    char result[sizeof(expectation) - 1] = {0}; // -1 due to \0 termination of expectation
    uint32_t result_len;
    serial_stub_get_write_buffer(result, sizeof(result), &result_len);

    TEST_ASSERT_EQUAL(MIOTYATCLIENT_RETURN_CODE_OK, r);
    TEST_ASSERT_EQUAL(42, packet_counter);
    TEST_ASSERT_EQUAL(result_len, sizeof(result));
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expectation, result, result_len);
}

void test_at_client_formats_uni_transparent_message_correctly(void)
{
    // given that
    char send_msg[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // fill read buffer
    ReadVectorEntry_t read_vector[] =
        {
            {"-TXA:1\r\n", 8},
            {"-TXA:0\r\n0\r\n", 11},
            {NULL, 0}};

    for (uint32_t i = 0; read_vector[i].buffer != NULL; i++)
    {
        serial_stub_push_to_read_vector(read_vector[i].buffer, read_vector[i].len);
    }

    // run
    miotyAtClient_returnCode r = miotyAtClient_sendMessageUniTransparent(send_msg, sizeof(send_msg));

    // then
    uint8_t expectation[] = "AT-TU=8\t0123456789ABCDEF\x1A\r";

    char result[sizeof(expectation) - 1] = {0}; // -1 due to \0 termination of expectation
    uint32_t result_len;
    serial_stub_get_write_buffer(result, sizeof(result), &result_len);

    TEST_ASSERT_EQUAL(MIOTYATCLIENT_RETURN_CODE_OK, r);
    TEST_ASSERT_EQUAL(result_len, sizeof(result));
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expectation, result, result_len);
}

void test_eui64_request_can_get_received(void)
{

    // given that
    ReadVectorEntry_t read_vector[] =
        {
            {"-MEUI:8\t123456789ABCDEF0\x1A\r\n0\r\n", 30},
            {NULL, 0}};

    for (uint32_t i = 0; read_vector[i].buffer != NULL; i++)
    {
        serial_stub_push_to_read_vector(read_vector[i].buffer, read_vector[i].len);
    }

    // then
    uint8_t eui64_result[8] = {0};
    miotyAtClient_returnCode res = miotyAtClient_getOrSetEui(eui64_result, false);

    // extract results
    char write_expectation[] = "AT-MEUI?\r";
    uint8_t expected_write_buffer_len = strlen(write_expectation);
    char write_buffer[expected_write_buffer_len];
    memset(write_buffer, 0, expected_write_buffer_len);

    uint32_t result_len = 0;
    serial_stub_get_write_buffer(write_buffer, expected_write_buffer_len, &result_len);

    // assert
    TEST_ASSERT(res == MIOTYATCLIENT_RETURN_CODE_OK);
    TEST_ASSERT(result_len == expected_write_buffer_len);

    TEST_ASSERT_EQUAL_CHAR_ARRAY(write_expectation, write_buffer, result_len);
}

// #endif // TEST

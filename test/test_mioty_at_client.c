// #ifdef TEST

#include "unity.h"
#include "serial_stub.h"
#include "miotyAtClient.h"

#include "char_tools.h"
#include "string_tools.h"

/* ============================
 * TEST inits
 * ============================ */

void setUp(void)
{
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

void miotyatclientTx_stop_cb(void)
{
}

/* ============================
 * TEST suites
 * ============================ */

void test_at_client_formats_uni_message_correctly(void)
{
    // given that
    char msg[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // run
    uint32_t packet_counter = 0;
    miotyAtClient_returnCode r = miotyAtClient_sendMessageUni(msg, sizeof(msg), &packet_counter);

    // then
    uint8_t expectation[] = "AT-U=8\t0123456789ABCDEF\x1A\r";

    char result[sizeof(expectation) - 1] = {0}; // -1 due to \0 termination of expectation
    uint32_t result_len;
    serial_stub_get_write_buffer(result, sizeof(result), &result_len);

    TEST_ASSERT_NOT_EQUAL(MIOTYATCLIENT_RETURN_CODE_OK, r);
    TEST_ASSERT_EQUAL(result_len, sizeof(result));
    TEST_ASSERT_EQUAL_CHAR_ARRAY(expectation, result, result_len);
}

// #endif // TEST

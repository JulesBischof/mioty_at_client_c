#define TEST

#ifdef TEST

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

}

#endif // TEST

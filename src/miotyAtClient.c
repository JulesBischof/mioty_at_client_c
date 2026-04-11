/**
 * \copyright    Copyright 2019 - 2022 Fraunhofer Institute for Integrated Circuits IIS, Erlangen Germany
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file
 * \version     0.0.2
 * \brief       Client side of communication with a MIOTY™ module via AT protocol v2.x.x
 */

#include "miotyAtClient.h"
#include "data_tools/string_tools.h"

/* ====================================================
 * PRIVATES
 * ====================================================*/

static miotyAtClient_returnCode _uni_fsm_receive_mpct(uint32_t *packetCounter)
{
    /*
     * init read buffer ...
     * REFERENCE MANUAL - PacketCounter is a 32 bit number: max 10 decimal digits!
     * p.29: longest expected Message "-MPCT:<n>\r\n"
     *
     * Therefore: 18 bit deep buffer is sufficient
     */

    uint8_t read_buffer[18] = {0};
    uint8_t received_bytes = 0;

    if (miotyAtClientRead(read_buffer, sizeof(read_buffer), &received_bytes) != true)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }

    if (sizeof(read_buffer) <= received_bytes) // validation - could be removed on release
    {
        for (;;)
            ; // that would mean an ovverrun happened - block!
    }

    // get the position of the col char and the "\r\n" and validate them
    char *pCol = strstr((const char *)read_buffer, (const char *)":");
    char *pEnd = strstr((const char *)read_buffer, (const char *)"\r\n");
    if (pCol == NULL || pEnd == NULL)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }
    // convert to int
    uint8_t slice_len = pEnd - pCol - 1;                                            // there is one offset since we subtract the EndPos, not the last digit
    *packetCounter = string_dec2uint((const unsigned char *)(pCol + 1), slice_len); // +1 since pCol points to ':'

    return MIOTYATCLIENT_RETURN_CODE_OK;
}

static miotyAtClient_returnCode _uni_fsm_receive_txa(bool txa_one_expected)
{
    uint8_t buffersize = txa_one_expected ? sizeof("-TXA:1\r\n") : sizeof("-TXA:0\r\n0\r\n");
    buffersize--; // -1 due to strings get followed by a \0 we are not going to receive!
    uint8_t read_buffer[buffersize];
    memset(read_buffer, 0, sizeof(read_buffer)); // clear for now in order to debug but can be removed later
    uint8_t received_bytes = 0;

    if (miotyAtClientRead(read_buffer, sizeof(read_buffer), &received_bytes) != true)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }

    // validate buffer_len - remove for release
    if (received_bytes != buffersize)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }

    // validate if TXA was received
    if (strstr((const char *)read_buffer, "TXA") == NULL)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }

    char *endString = txa_one_expected ? "\r\n" : "\r\n0\r\n"; // same reason as difference in buffersize

    // check the argument - is it a TXA 1?
    char *pCol = strstr((const char *)read_buffer, (const char *)":");
    char *pEnd = strstr((const char *)read_buffer, (const char *)endString);
    if (pCol == NULL || pEnd == NULL)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }

    char expectation = txa_one_expected ? '1' : '0';
    if (*(pCol + 1) != expectation)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }

    // if code got here - everything was valid: run callbacks
    if (txa_one_expected)
    {
        miotyAtClientTx_start_cb();
    }
    else
    {
        miotyatclientTx_stop_cb();
    }

    return MIOTYATCLIENT_RETURN_CODE_OK;
}

static miotyAtClient_returnCode _handle_uni_uplink_response_fsm(uint32_t *packetCounter)
{
    /* now receive the package counter & parse to an integer */
    if (_uni_fsm_receive_mpct(packetCounter) != MIOTYATCLIENT_RETURN_CODE_OK)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }

    /* wait for the "TXA:1\r\n" (ack, that transmit has started) */
    if (_uni_fsm_receive_txa(true) != MIOTYATCLIENT_RETURN_CODE_OK)
    {
        return MIOTYATCLIENT_RETURN_CODE_OK;
    }

    /* wait for the Transmission to be over: "TXA:0\r\n0\r\n" */
    if (_uni_fsm_receive_txa(false) != MIOTYATCLIENT_RETURN_CODE_OK)
    {
        return MIOTYATCLIENT_RETURN_CODE_OK;
    }

    return MIOTYATCLIENT_RETURN_CODE_OK;
}

static void internalGetPacketCounter(char *response_buf, uint32_t *packetCounter)
{
    char *pos = strstr(response_buf, "-MPCT:");
    if ((pos != NULL) && (packetCounter != NULL))
    {
        *packetCounter = atoi((pos + 6));
    }
}

static void get_MSTA(uint8_t *response_buf, uint8_t *MSTA)
{
    char *pos = strstr(response_buf, "-MSTA:");
    if (pos != NULL)
        *MSTA = atoi(pos + 6);
}

// converts uint8_t data to hexadecimal string representation
static bool write_cmd_bytes(uint8_t *AT_cmd, uint8_t sizeCmd, uint8_t *data, uint8_t sizeData)
{
    // convert data size into string
    uint32_t digits = 1; // smallest ammount of digits is 1 (even a 0 needs to be represented by one digit)
    for (uint32_t size = sizeData; size /= 10; digits++)
        ;
    char data_size_string[digits + 1]; // +1 due to \0 termination
    string_uint2str_la_zt(sizeCmd, data_size_string);

    // convert payload into hex coded string
    const uint32_t data_string_size = sizeData * 2; // hex representation
    char data_string[data_string_size]; // string_byteArray2hex does not add zero termination!
    string_byteArray2hex(data, sizeData, data_string, data_string_size);

    // prepare command buffer
    const uint32_t cmd_size = sizeCmd + digits + data_string_size + 4;
    char cmd[cmd_size];
    memset(cmd, 0, sizeof(cmd));

    // assemble command
    char *pWrite = cmd; // pointer to first char
    memcpy(pWrite, AT_cmd, sizeCmd);
    pWrite += sizeCmd;
    *pWrite++ = '=';
    memcpy(pWrite, data_size_string, digits);
    pWrite += digits;
    *pWrite++ = '\t';
    memcpy(pWrite, data_string, data_string_size);
    pWrite += data_string_size;
    *pWrite++ = 0x1A;
    *pWrite = '\r';

    return miotyAtClientWrite((uint8_t *)cmd, sizeof(cmd));
}

static miotyAtClient_returnCode check_ATresponse(char *response_buf)
{
    uint8_t pos = 0;
    miotyAtClient_returnCode return_code = MIOTYATCLIENT_RETURN_CODE_ERR;
    while (1)
    {
        uint8_t buf[30];
        uint8_t len = 30;
        if (miotyAtClientRead(buf, sizeof(buf), &len))
        {
            for (uint8_t i = 0; i < len; i++)
            {
                if (isalpha(buf[i]))
                    buf[i] = toupper(buf[i]);
                response_buf[i + pos] = buf[i];
            }
            pos += len;
            response_buf[pos + 1] = '\0';
            if (strstr(response_buf, "\r\n0\r\n") || strstr(response_buf, "0\r\n") == response_buf)
            {
                return_code = MIOTYATCLIENT_RETURN_CODE_OK;
                break;
            }
            else if (strstr(response_buf, "\r\n1\r\n"))
            {
                char *err_pos = strstr(response_buf, "-MNFO:");
                if (err_pos == NULL)
                    err_pos = strstr(response_buf, "-MERR:");
                if (err_pos == NULL)
                {
                    return_code = MIOTYATCLIENT_RETURN_CODE_ERR;
                    break;
                }
                err_pos += 6;
                return_code = atoi(err_pos);
                break;
            }
            else if (strstr(response_buf, "\r\n2\r\n"))
            {
                char *err_pos = strstr(response_buf, "AT!ERR:");
                if (err_pos == NULL)
                {
                    return_code = MIOTYATCLIENT_RETURN_CODE_ATErr;
                    break;
                }
                err_pos += 7;
                return_code = atoi(err_pos) + 16;
                break;
            }
        }
        else
        {
            return MIOTYATCLIENT_RETURN_CODE_ATReadFailed;
        }
    }
    return return_code;
}

static miotyAtClient_returnCode checkATresponseMsg(uint32_t *packetCounter)
{
    char response_buf[200] = {0};
    miotyAtClient_returnCode ret = check_ATresponse(response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    internalGetPacketCounter(response_buf, packetCounter); // EXAMPLE VALUE "-MPCT:6556\r\n-TXA:1\r\n-TXA:0\r\n0\r\n"

    return ret;
}

static miotyAtClient_returnCode get_int_data_ATresponse(uint8_t *AT_cmd, uint8_t sizeCmd, uint32_t *res, char *response_buf)
{
    uint8_t pos = 0;
    miotyAtClient_returnCode return_code = MIOTYATCLIENT_RETURN_CODE_ERR;
    while (1)
    {
        uint8_t buf[30];
        uint8_t len = 30;
        if (miotyAtClientRead(buf, sizeof(buf), &len) == true)
        {

            for (uint8_t i = 0; i < len; i++)
            {
                if (isalpha(buf[i]))
                    buf[i] = toupper(buf[i]);
                response_buf[i + pos] = buf[i];
            }
            pos += len;
            response_buf[pos] = '\0';
            if (strstr(response_buf, "\r\n0\r\n") || strstr(response_buf, "0\r\n") == response_buf)
            {
                return_code = MIOTYATCLIENT_RETURN_CODE_OK;
                char *pos = strstr(response_buf, AT_cmd + 2);
                char *end_pos = NULL;
                pos += sizeCmd - 1;
                *res = atoi(pos);
                break;
            }
            else if (strstr(response_buf, "\r\n1\r\n"))
            {
                char *err_pos = strstr(response_buf, "-MNFO:");
                if (err_pos == NULL)
                    err_pos = strstr(response_buf, "-MERR:");
                if (err_pos == NULL)
                {
                    return_code = MIOTYATCLIENT_RETURN_CODE_ERR;
                    break;
                }
                err_pos += 6;
                return_code = atoi(err_pos);
                break;
            }
            else if (strstr(response_buf, "\r\n2\r\n"))
            {
                char *err_pos = strstr(response_buf, "AT!ERR:");
                if (err_pos == NULL)
                {
                    return_code = MIOTYATCLIENT_RETURN_CODE_ATErr;
                    break;
                }
                err_pos += 7;
                return_code = atoi(err_pos) + 16;
                break;
            }
        }
        else
        {
            return MIOTYATCLIENT_RETURN_CODE_ATReadFailed;
        }
    }
    return return_code;
}

static miotyAtClient_returnCode get_data_ATresponse(uint8_t *AT_cmd, uint8_t sizeCmd, uint8_t *buffer, uint8_t *sizeBuf, char *response_buf)
{
    uint8_t pos = 0;
    miotyAtClient_returnCode return_code = MIOTYATCLIENT_RETURN_CODE_ERR;
    while (1)
    {
        uint8_t buf[30];
        uint8_t len = 30;
        if (miotyAtClientRead(buf, sizeof(buf), &len))
        {
            for (uint8_t i = 0; i < len; i++)
            {
                if (isalpha(buf[i]))
                    buf[i] = toupper(buf[i]);
                response_buf[i + pos] = buf[i];
            }
            pos += len;
            response_buf[pos + 1] = '\0';
            if (strstr(response_buf, "\r\n0\r\n") || strstr(response_buf, "0\r\n") == response_buf)
            {
                return_code = MIOTYATCLIENT_RETURN_CODE_OK;

                // get format positions
                char *pCol = strstr(response_buf, ":");
                char *pTab = strstr(response_buf, "\t");
                char *pEnd = strstr(response_buf, "\032");

                // validate them
                if (pTab == NULL || pCol == NULL || pEnd == NULL || pCol > pTab || pTab > pEnd)
                {
                    return MIOTYATCLIENT_RETURN_CODE_ERR;
                }

                // check data len field (in between : and \t)
                uint32_t data_len_slice_len = pTab - pCol - 1; // one ofset since we're counting "fence to fence"
                char *data_len_slice = pCol + 1;
                uint32_t data_len = string_dec2uint((const unsigned char *)data_len_slice, (const uint8_t)data_len_slice_len);

                // and now get the data
                uint32_t data_slice_len = pEnd - pTab - 1; // -1 due to from : to \032 there is 1 offset
                char *data_slice = pTab + 1;               // first letter after \t

                // one last validation
                if ((2 * data_len) != (data_slice_len)) // we're missing bytes
                {
                    return MIOTYATCLIENT_RETURN_CODE_ERR;
                }

                // use string utils
                string_hex2byteArray((const unsigned char *)data_slice, (const uint8_t)data_slice_len, buffer, *sizeBuf);

                break;
            }
            else if (strstr(response_buf, "\r\n1\r\n"))
            {
                char *err_pos = strstr(response_buf, "-MNFO:");
                if (err_pos == NULL)
                    err_pos = strstr(response_buf, "-MERR:");
                if (err_pos == NULL)
                {
                    return_code = MIOTYATCLIENT_RETURN_CODE_ERR;
                    break;
                }
                err_pos += 6;
                return_code = atoi(err_pos);
                break;
            }
            else if (strstr(response_buf, "\r\n2\r\n"))
            {
                char *err_pos = strstr(response_buf, "AT!ERR:");
                if (err_pos == NULL)
                {
                    return_code = MIOTYATCLIENT_RETURN_CODE_ATErr;
                    break;
                }
                err_pos += 7;
                return_code = atoi(err_pos) + 16;
                break;
            }
        }
        else
        {
            return MIOTYATCLIENT_RETURN_CODE_ATReadFailed;
        }
    }
    return return_code;
}

static miotyAtClient_returnCode get_info_bytes(uint8_t *AT_cmd, uint8_t sizeCmd, uint8_t *buffer, uint8_t *sizeBuf)
{
    char cmd[sizeCmd + 2];
    strcpy(cmd, AT_cmd);
    cmd[sizeCmd] = '?';
    cmd[sizeCmd + 1] = '\r';
    miotyAtClientWrite((uint8_t *)cmd, sizeof(cmd));
    char response_buf[200];
    return get_data_ATresponse(AT_cmd, sizeCmd, buffer, sizeBuf, response_buf);
}

static miotyAtClient_returnCode set_info_bytes(uint8_t *AT_cmd, uint8_t sizeCmd, uint8_t *data, uint8_t sizeData)
{
    write_cmd_bytes(AT_cmd, sizeCmd, data, sizeData);
    char response_buf[200];
    return check_ATresponse(response_buf);
}

miotyAtClient_returnCode get_info_int(uint8_t *AT_cmd, uint8_t sizeCmd, uint32_t *res)
{
    char cmd[sizeCmd + 2];
    strcpy(cmd, AT_cmd);
    cmd[sizeCmd] = '?';
    cmd[sizeCmd + 1] = '\r';
    miotyAtClientWrite((uint8_t *)cmd, sizeof(cmd));
    char response_buf[200];
    return get_int_data_ATresponse(AT_cmd, sizeCmd, res, response_buf);
}

miotyAtClient_returnCode set_info_int(uint8_t *AT_cmd, uint8_t sizeCmd, uint32_t *info)
{
    char buf[12] = {0};
    uint16_t len_info = string_uint2str_la_zt(*info, buf) - buf;
    char cmd[sizeCmd + 2 + len_info];
    strcpy(cmd, AT_cmd);
    cmd[sizeCmd] = '=';
    strcpy(cmd + sizeCmd + 1, buf);
    cmd[sizeCmd + 1 + len_info] = '\r';
    miotyAtClientWrite((uint8_t *)cmd, sizeof(cmd));
    char response_buf[200];
    return check_ATresponse(response_buf);
}

/* ====================================================
 * PUBLICS
 * ==================================================== */

miotyAtClient_returnCode miotyAtClient_reset(void)
{
    char cmd[7] = "AT-RST\r";
    miotyAtClientWrite((uint8_t *)cmd, sizeof(cmd));
    char response_buf[200];
    return check_ATresponse(response_buf);
}

miotyAtClient_returnCode miotyAtClient_factoryReset(void)
{
    char cmd[4] = "ATZ\r";
    miotyAtClientWrite((uint8_t *)cmd, sizeof(cmd));
    char response_buf[200];
    return check_ATresponse(response_buf);
}

miotyAtClient_returnCode miotyAtClient_setNetworkKey(uint8_t *nwKey)
{
    return set_info_bytes("AT-MNWK", 7, nwKey, 16);
}

miotyAtClient_returnCode miotyAtClient_getOrSetIPv6SubnetMask(uint8_t *ipv6, bool set)
{
    if (set)
        return set_info_bytes("AT-MIP6", 7, ipv6, 8);
    uint8_t size_bytes = 8;
    return get_info_bytes("AT-MIP6", 7, ipv6, &size_bytes);
}

miotyAtClient_returnCode miotyAtClient_getOrSetEui(uint8_t *eui64, bool set)
{
    if (set)
        return set_info_bytes("AT-MEUI", 7, eui64, 8);
    uint8_t size_bytes = 8;
    return get_info_bytes("AT-MEUI", 7, eui64, &size_bytes);
}

miotyAtClient_returnCode miotyAtClient_getOrSetShortAdress(uint8_t *shortAdress, bool set)
{
    if (set)
        return set_info_bytes("AT-MSAD", 7, shortAdress, 2);
    uint8_t size_bytes = 2;
    return get_info_bytes("AT-MSAD", 7, shortAdress, &size_bytes);
}

miotyAtClient_returnCode miotyAtClient_getPacketCounter(uint32_t *counter)
{
    return get_info_int("AT-MPCT", 7, counter);
}

miotyAtClient_returnCode miotyAtClient_getOrSetBaudrate(uint32_t *baud, bool set)
{
    if (set)
        return set_info_int("AT+IPR", 6, baud);
    return get_info_int("AT+IPR", 6, baud);
}

miotyAtClient_returnCode miotyAtClient_getOrSetTransmitPower(uint32_t *txPower, bool set)
{
    if (set)
        return set_info_int("AT-UTPL", 7, txPower);
    return get_info_int("AT-UTPL", 7, txPower);
}

miotyAtClient_returnCode miotyAtClient_uplinkMode(uint32_t *ulMode, bool set)
{
    if (set)
        return set_info_int("AT-UM", 5, ulMode);
    return get_info_int("AT-UM", 5, ulMode);
}

miotyAtClient_returnCode miotyAtClient_uplinkSyncBurst(uint32_t *ulSyncBurst, bool set)
{
    if (set)
        return set_info_int("AT-US", 5, ulSyncBurst);
    return get_info_int("AT-US", 5, ulSyncBurst);
}

miotyAtClient_returnCode miotyAtClient_uplinkProfile(uint32_t *ulProfile, bool set)
{
    if (set)
        return set_info_int("AT-UP", 5, ulProfile);
    return get_info_int("AT-UP", 5, ulProfile);
}

miotyAtClient_returnCode miotyAtClient_appCryptoMode(uint32_t *appCryptoMode, bool set)
{
    if (set)
        return set_info_int("AT-ACM", 6, appCryptoMode);
    return get_info_int("AT-ACM", 6, appCryptoMode);
}

miotyAtClient_returnCode miotyAtClient_setAppCryptoKey(uint8_t *appCryptoKey)
{
    return set_info_bytes("AT-ACK", 6, appCryptoKey, 16);
}

miotyAtClient_returnCode miotyAtClient_sendMessageUniTransparent(uint8_t *msg, uint8_t sizeMsg, uint32_t *packetCounter)
{
    write_cmd_bytes("AT-TU", 5, msg, sizeMsg);
    miotyAtClientOnIdle(sizeMsg);
    return checkATresponseMsg(packetCounter);
}

miotyAtClient_returnCode miotyAtClient_sendMessageUniMPF(uint8_t *msg, uint8_t sizeMsg, uint32_t *packetCounter)
{
    write_cmd_bytes("AT-UMPF", 7, msg, sizeMsg);
    miotyAtClientOnIdle(sizeMsg);
    return checkATresponseMsg(packetCounter);
}

miotyAtClient_returnCode miotyAtClient_sendMessageUni(uint8_t *msg, uint8_t sizeMsg, uint32_t *packetCounter)
{
    // write_cmd_bytes("AT-U", 4, msg, sizeMsg);
    // miotyAtClientOnIdle(sizeMsg);
    // return checkATresponseMsg(packetCounter);

    const char *at_cmd = "AT-U";
    if (write_cmd_bytes(at_cmd, sizeof(at_cmd), msg, sizeMsg) == false)
    {
        return MIOTYATCLIENT_RETURN_CODE_ERR;
    }
    return _handle_uni_uplink_response_fsm(packetCounter);
}

miotyAtClient_returnCode miotyAtClient_sendMessageBidiTransparent(uint8_t *msg, uint8_t sizeMsg, uint8_t *data, uint8_t *size_data, uint32_t *packetCounter)
{
    write_cmd_bytes("AT-TB", 5, msg, sizeMsg);
    char response_buf[200];
    miotyAtClient_returnCode ret = get_data_ATresponse("AT-TB", 5, data, size_data, response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    internalGetPacketCounter(response_buf, packetCounter);

    return ret;
}

miotyAtClient_returnCode miotyAtClient_sendMessageBidiMPF(uint8_t *msg, uint8_t sizeMsg, uint8_t *data, uint8_t *size_data, uint32_t *packetCounter)
{
    write_cmd_bytes("AT-BMPF", 7, msg, sizeMsg);
    char response_buf[200];
    miotyAtClient_returnCode ret = get_data_ATresponse("AT-BMPF", 7, data, size_data, response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    internalGetPacketCounter(response_buf, packetCounter);

    return ret;
}

miotyAtClient_returnCode miotyAtClient_sendMessageBidi(uint8_t *msg, uint8_t sizeMsg, uint8_t *data, uint8_t *size_data, uint32_t *packetCounter)
{
    write_cmd_bytes("AT-B", 4, msg, sizeMsg);
    char response_buf[200];
    miotyAtClient_returnCode ret = get_data_ATresponse("AT-B", 4, data, size_data, response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    internalGetPacketCounter(response_buf, packetCounter);

    return ret;
}

miotyAtClient_returnCode miotyAtClient_macDetach(uint8_t *data, uint8_t sizeData, uint8_t *MSTA)
{
    write_cmd_bytes("AT-MDOA", 7, data, sizeData);
    char response_buf[200];
    miotyAtClient_returnCode ret = check_ATresponse(response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    get_MSTA(response_buf, MSTA);

    return ret;
}

miotyAtClient_returnCode miotyAtClient_macAttach(uint8_t *data, uint8_t *MSTA)
{
    write_cmd_bytes("AT-MAOA", 7, data, 4);
    char response_buf[200];
    miotyAtClient_returnCode ret = check_ATresponse(response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    get_MSTA(response_buf, MSTA);

    return ret;
}

miotyAtClient_returnCode miotyAtClient_macAttachLocal(uint8_t *MSTA)
{
    miotyAtClientWrite("AT-MALO\r", 8);
    char response_buf[200];
    miotyAtClient_returnCode ret = check_ATresponse(response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    get_MSTA(response_buf, MSTA);

    return ret;
}

miotyAtClient_returnCode miotyAtClient_macDetachLocal(uint8_t *MSTA)
{
    miotyAtClientWrite("AT-MDLO\r", 8);
    char response_buf[200];
    miotyAtClient_returnCode ret = check_ATresponse(response_buf);
    if (ret != MIOTYATCLIENT_RETURN_CODE_OK)
        return ret;
    get_MSTA(response_buf, MSTA);

    return ret;
}
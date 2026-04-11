#ifndef SERIAL_STUB_H
#define SERIAL_STUB_H

#include <stdint.h>
#include <stdbool.h>

bool miotyAtClientWrite(uint8_t *buffer, uint16_t buffer_len);

bool miotyAtClientRead(uint8_t *msg, uint8_t msg_len, uint8_t *p_len);

#endif // SERIAL_STUB_H
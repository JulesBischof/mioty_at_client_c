#ifndef SERIAL_STUB_H
#define SERIAL_STUB_H

#include <stdint.h>
#include <stdbool.h>

bool miotyAtClientWrite(uint8_t *buffer, uint16_t buffer_len);

bool miotyAtClientRead(uint8_t *msg, uint8_t msg_len, uint8_t *p_len);

void serial_stub_get_write_buffer(char *buffer, uint8_t len, uint32_t *watermark);
void serial_stub_set_read_buffer(char *buffer, uint8_t len);
void serial_stub_next_call_shall_trigger_error(bool flag);

#endif // SERIAL_STUB_H
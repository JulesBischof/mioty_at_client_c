#ifndef SERIAL_STUB_H
#define SERIAL_STUB_H

#include <stdint.h>
#include <stdbool.h>

#define BUFFERSIZE (256)
#define READ_VECTOR_LENGTH (32)

bool miotyAtClientWrite(uint8_t *buffer, uint16_t buffer_len);

bool miotyAtClientRead(uint8_t *msg, uint8_t msg_len, uint8_t *p_len);

void serial_stub_get_write_buffer(char *buffer, uint8_t len, uint32_t *watermark);

void serial_stub_push_to_read_vector(char* buffer, uint8_t len);

void serial_stub_next_call_shall_trigger_error(bool flag);

void serial_stub_reset(void);

#endif // SERIAL_STUB_H
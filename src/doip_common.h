#ifndef DOIP_COMMON_H
#define DOIP_COMMON_H

#include <stdint.h>

#define DOIP_PROTOCOL_VERSION 0x02
#define DOIP_PORT 13400

/* Payload types used in this demo */
#define DOIP_PT_VEHICLE_IDENT_REQ 0x0001
#define DOIP_PT_VEHICLE_IDENT_RES 0x0004
#define DOIP_PT_ROUTING_ACTIVATION_REQ 0x0005
#define DOIP_PT_ROUTING_ACTIVATION_RES 0x0006

/* DoIP generic header is always 8 bytes */
typedef struct {
    uint8_t protocol_version;
    uint8_t inverse_protocol_version;
    uint16_t payload_type;
    uint32_t payload_length;
} doip_header_t;

static inline uint16_t read_u16_be(const uint8_t *buf) {
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

static inline uint32_t read_u32_be(const uint8_t *buf) {
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
           (uint32_t)buf[3];
}

static inline void write_u16_be(uint8_t *buf, uint16_t v) {
    buf[0] = (uint8_t)((v >> 8) & 0xFF);
    buf[1] = (uint8_t)(v & 0xFF);
}

static inline void write_u32_be(uint8_t *buf, uint32_t v) {
    buf[0] = (uint8_t)((v >> 24) & 0xFF);
    buf[1] = (uint8_t)((v >> 16) & 0xFF);
    buf[2] = (uint8_t)((v >> 8) & 0xFF);
    buf[3] = (uint8_t)(v & 0xFF);
}

#endif

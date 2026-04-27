#include "doip_common.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int recv_exact(int fd, uint8_t *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        ssize_t n = recv(fd, buf + done, len - done, 0);
        if (n <= 0) {
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

static int send_all(int fd, const uint8_t *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        ssize_t n = send(fd, buf + done, len - done, 0);
        if (n <= 0) {
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

static int read_doip_header(int fd, doip_header_t *hdr) {
    uint8_t raw[8];
    if (recv_exact(fd, raw, sizeof(raw)) != 0) {
        return -1;
    }

    hdr->protocol_version = raw[0];
    hdr->inverse_protocol_version = raw[1];
    hdr->payload_type = read_u16_be(&raw[2]);
    hdr->payload_length = read_u32_be(&raw[4]);
    return 0;
}

static int send_doip_message(int fd, uint16_t payload_type, const uint8_t *payload, uint32_t payload_len) {
    uint8_t header[8];
    header[0] = DOIP_PROTOCOL_VERSION;
    header[1] = (uint8_t)(~DOIP_PROTOCOL_VERSION);
    write_u16_be(&header[2], payload_type);
    write_u32_be(&header[4], payload_len);

    if (send_all(fd, header, sizeof(header)) != 0) {
        return -1;
    }
    if (payload_len > 0 && send_all(fd, payload, payload_len) != 0) {
        return -1;
    }
    return 0;
}

static int handle_client(int client_fd) {
    for (;;) {
        doip_header_t hdr;
        if (read_doip_header(client_fd, &hdr) != 0) {
            return -1;
        }

        if ((uint8_t)(hdr.protocol_version ^ hdr.inverse_protocol_version) != 0xFF) {
            fprintf(stderr, "Invalid DoIP protocol version fields\n");
            return -1;
        }

        if (hdr.payload_length > 4096) {
            fprintf(stderr, "Payload too large: %u\n", hdr.payload_length);
            return -1;
        }

        uint8_t payload[4096];
        if (hdr.payload_length > 0 && recv_exact(client_fd, payload, hdr.payload_length) != 0) {
            return -1;
        }

        printf("RX payload_type=0x%04X length=%u\n", hdr.payload_type, hdr.payload_length);

        if (hdr.payload_type == DOIP_PT_VEHICLE_IDENT_REQ && hdr.payload_length == 0) {
            uint8_t resp[33] = {0};
            /* Vehicle identification response (simplified demo fields). */
            write_u16_be(&resp[0], 0x0E00); /* logical address */
            memcpy(&resp[2], "VINDEMO1234567890", 17);
            memset(&resp[19], 0x11, 6); /* EID */
            memset(&resp[25], 0x22, 6); /* GID */
            resp[31] = 0x00;            /* further action required */
            resp[32] = 0x10;            /* VIN/GID sync status */

            if (send_doip_message(client_fd, DOIP_PT_VEHICLE_IDENT_RES, resp, sizeof(resp)) != 0) {
                return -1;
            }
            printf("TX vehicle identification response\n");
        } else if (hdr.payload_type == DOIP_PT_ROUTING_ACTIVATION_REQ && hdr.payload_length >= 7) {
            uint16_t source_addr = read_u16_be(&payload[0]);

            uint8_t resp[9] = {0};
            write_u16_be(&resp[0], 0x0E00); /* tester logical address (server side demo) */
            write_u16_be(&resp[2], source_addr);
            resp[4] = 0x10; /* routing activation response code: success (demo) */
            /* bytes 5..8 reserved/ISO */

            if (send_doip_message(client_fd, DOIP_PT_ROUTING_ACTIVATION_RES, resp, sizeof(resp)) != 0) {
                return -1;
            }
            printf("TX routing activation response\n");
        } else {
            printf("Unhandled payload type 0x%04X\n", hdr.payload_type);
        }
    }
}

int main(int argc, char **argv) {
    const char *bind_ip = "0.0.0.0";
    int port = DOIP_PORT;
    if (argc >= 2) {
        bind_ip = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        perror("setsockopt");
        close(listen_fd);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, bind_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid bind IP: %s\n", bind_ip);
        close(listen_fd);
        return 1;
    }

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 4) != 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("DoIP server listening on %s:%d\n", bind_ip, port);

    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            break;
        }

        char ipbuf[64] = {0};
        inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));
        printf("Client connected: %s:%u\n", ipbuf, ntohs(peer.sin_port));

        (void)handle_client(client_fd);
        close(client_fd);
        printf("Client disconnected\n");
    }

    close(listen_fd);
    return 0;
}

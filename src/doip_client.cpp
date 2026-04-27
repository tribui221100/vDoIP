#include "doip_common.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

static bool recvExact(int fd, uint8_t *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        const ssize_t n = recv(fd, buf + done, len - done, 0);
        if (n <= 0) {
            return false;
        }
        done += static_cast<size_t>(n);
    }
    return true;
}

static bool sendAll(int fd, const uint8_t *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        const ssize_t n = send(fd, buf + done, len - done, 0);
        if (n <= 0) {
            return false;
        }
        done += static_cast<size_t>(n);
    }
    return true;
}

static bool sendDoipMessage(int fd, uint16_t payloadType, const std::vector<uint8_t> &payload) {
    uint8_t hdr[8];
    hdr[0] = DOIP_PROTOCOL_VERSION;
    hdr[1] = static_cast<uint8_t>(~DOIP_PROTOCOL_VERSION);
    write_u16_be(&hdr[2], payloadType);
    write_u32_be(&hdr[4], static_cast<uint32_t>(payload.size()));

    if (!sendAll(fd, hdr, sizeof(hdr))) {
        return false;
    }
    if (!payload.empty() && !sendAll(fd, payload.data(), payload.size())) {
        return false;
    }
    return true;
}

static bool readDoipMessage(int fd, doip_header_t &hdr, std::vector<uint8_t> &payload) {
    uint8_t raw[8];
    if (!recvExact(fd, raw, sizeof(raw))) {
        return false;
    }

    hdr.protocol_version = raw[0];
    hdr.inverse_protocol_version = raw[1];
    hdr.payload_type = read_u16_be(&raw[2]);
    hdr.payload_length = read_u32_be(&raw[4]);

    if ((static_cast<uint8_t>(hdr.protocol_version ^ hdr.inverse_protocol_version) != 0xFF) ||
        (hdr.payload_length > 4096)) {
        return false;
    }

    payload.assign(hdr.payload_length, 0);
    if (hdr.payload_length > 0 && !recvExact(fd, payload.data(), hdr.payload_length)) {
        return false;
    }
    return true;
}

static void printVehicleIdResponse(const std::vector<uint8_t> &payload) {
    if (payload.size() < 33) {
        std::cout << "Vehicle identification response too short\n";
        return;
    }

    const uint16_t logicalAddr = read_u16_be(payload.data());
    std::string vin(reinterpret_cast<const char *>(&payload[2]), 17);
    std::cout << "Vehicle ID response:\n";
    std::cout << "  Logical address: 0x" << std::hex << logicalAddr << std::dec << "\n";
    std::cout << "  VIN: " << vin << "\n";
}

int main(int argc, char **argv) {
    std::string serverIp = "127.0.0.1";
    int port = DOIP_PORT;
    if (argc >= 2) {
        serverIp = argv[1];
    }
    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, serverIp.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "Invalid server IP: " << serverIp << "\n";
        close(fd);
        return 1;
    }

    if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    std::cout << "Connected to " << serverIp << ":" << port << "\n";

    if (!sendDoipMessage(fd, DOIP_PT_VEHICLE_IDENT_REQ, {})) {
        std::cerr << "Failed to send vehicle identification request\n";
        close(fd);
        return 1;
    }
    std::cout << "TX vehicle identification request\n";

    doip_header_t rxHdr {};
    std::vector<uint8_t> rxPayload;
    if (!readDoipMessage(fd, rxHdr, rxPayload)) {
        std::cerr << "Failed reading first response\n";
        close(fd);
        return 1;
    }
    std::cout << "RX payload_type=0x" << std::hex << rxHdr.payload_type << std::dec
              << " len=" << rxHdr.payload_length << "\n";

    if (rxHdr.payload_type == DOIP_PT_VEHICLE_IDENT_RES) {
        printVehicleIdResponse(rxPayload);
    }

    std::vector<uint8_t> raReq(7, 0);
    write_u16_be(&raReq[0], 0x0E80); /* source logical address */
    raReq[2] = 0x00;                 /* activation type */
    /* bytes 3..6 reserved */

    if (!sendDoipMessage(fd, DOIP_PT_ROUTING_ACTIVATION_REQ, raReq)) {
        std::cerr << "Failed to send routing activation request\n";
        close(fd);
        return 1;
    }
    std::cout << "TX routing activation request\n";

    if (!readDoipMessage(fd, rxHdr, rxPayload)) {
        std::cerr << "Failed reading routing activation response\n";
        close(fd);
        return 1;
    }
    std::cout << "RX payload_type=0x" << std::hex << rxHdr.payload_type << std::dec
              << " len=" << rxHdr.payload_length << "\n";

    if (rxHdr.payload_type == DOIP_PT_ROUTING_ACTIVATION_RES && rxPayload.size() >= 5) {
        const uint8_t responseCode = rxPayload[4];
        std::cout << "Routing activation response code: 0x" << std::hex
                  << static_cast<int>(responseCode) << std::dec << "\n";
    }

    close(fd);
    return 0;
}

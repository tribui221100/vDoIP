CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O2
CXXFLAGS := -Wall -Wextra -Wpedantic -std=c++17 -O2
BUILD_DIR := build
SRC_DIR := src

SERVER := $(BUILD_DIR)/doip_server
CLIENT := $(BUILD_DIR)/doip_client

.PHONY: all clean

all: $(SERVER) $(CLIENT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SERVER): $(SRC_DIR)/doip_server.c $(SRC_DIR)/doip_common.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/doip_server.c -o $(SERVER)

$(CLIENT): $(SRC_DIR)/doip_client.cpp $(SRC_DIR)/doip_common.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/doip_client.cpp -o $(CLIENT)

clean:
	rm -rf $(BUILD_DIR)

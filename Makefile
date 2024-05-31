TARGET_SERVER = srv
TARGET_CLIENT = clnt

SRC_DIR = src

OBJ_DIR = obj

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

SERVER_SOURCES := $(shell find $(SRC_DIR)/server $(SRC_DIR)/utils -name '*.c')
CLIENT_SOURCES := $(shell find $(SRC_DIR)/client $(SRC_DIR)/utils -name '*.c')

SERVER_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SERVER_SOURCES))
CLIENT_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CLIENT_SOURCES))

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(TARGET_CLIENT): $(CLIENT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/server $(OBJ_DIR)/client $(OBJ_DIR)/utils

clean:
	rm -rf $(OBJ_DIR) $(TARGET_SERVER) $(TARGET_CLIENT)

.PHONY: all clean
all: $(TARGET_SERVER) $(TARGET_CLIENT)
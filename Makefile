CC := cc
TARGET := a.out

SRC := main.c request.c server.c
INC := -Iinclude

BUILD ?= debug

CFLAGS_COMMON := -std=c11 \
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
  $(INC)

LDFLAGS_COMMON :=

ifeq ($(BUILD),debug)
  CFLAGS := $(CFLAGS_COMMON) -g -O0 -DDEBUG=1 \
    -fsanitize=address,undefined -fno-omit-frame-pointer
  LDFLAGS := $(LDFLAGS_COMMON) -fsanitize=address,undefined
else ifeq ($(BUILD),release)
  CFLAGS := $(CFLAGS_COMMON) -O2 -DNDEBUG
  LDFLAGS := $(LDFLAGS_COMMON)
else
  $(error Unknown BUILD '$(BUILD)'; use BUILD=debug or BUILD=release)
endif

.PHONY: all run clean

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)


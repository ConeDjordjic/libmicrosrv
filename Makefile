CC := cc
TARGET := a.out

SRC := main.c request.c server.c
INC := -Iinclude

CFLAGS := -std=c11 \
          -g -O0 \
          -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
          -fsanitize=address,undefined -fno-omit-frame-pointer \
          $(INC)

LDFLAGS := -fsanitize=address,undefined

.PHONY: all run clean

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)


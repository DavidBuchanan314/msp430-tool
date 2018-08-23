CC := gcc
CFLAGS := -Wall -Wpedantic -Wextra -O1 -std=c99 -Wno-missing-field-initializers

SRCDIR= ./src
OBJS := main.o disas.o queue.o
TARGET := msp430-tool

.PHONY: clean

all: $(TARGET)

%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm $(TARGET) $(OBJS)

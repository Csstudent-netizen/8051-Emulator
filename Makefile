CC = clang
CFLAGS = -Wall -g
TARGET = emulator
SRCS = main.c system.c cpu.c peripherals.c

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
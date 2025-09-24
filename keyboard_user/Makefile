CC = gcc
CFLAGS = -Wall -O2
TARGET = keyboard_user

all: $(TARGET)

$(TARGET): keyboard_user.c
	$(CC) $(CFLAGS) -o $(TARGET) keyboard_user.c

clean:
	rm -f $(TARGET) *.o *~

CC = gcc
CFLAGS=-pedantic -Wall -ansi -O4
SRCDIR=src/
TARGET = shell

all: $(TARGET)

$(TARGET): $(SRCDIR)$(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCDIR)*.c

clean:
	$(RM) $(TARGET)

CC = gcc

# assumes mpc installed to default PREFIX
MPCRPATH = /usr/local/lib

CFLAGS = -g -Wall -Wextra -std=c11

LDFLAGS = -lreadline
LDFLAGS += -lmpc -Wl,-rpath=$(MPCRPATH)

TARGET = buildyourownlisp

SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

CC=gcc
CFLAGS=-Wall

SRC=aesdsocket.c
OUTPUT=aesdsocket

all: clean $(OUTPUT)

aesdsocket: $(SRC)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC)

clean:
	rm -f $(OUTPUT)

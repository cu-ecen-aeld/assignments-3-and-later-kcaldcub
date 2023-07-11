CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall 
LDFLAGS ?= -pthread

SRC=aesdsocket.c
OUTPUT=aesdsocket

all: clean $(OUTPUT)

aesdsocket: $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)

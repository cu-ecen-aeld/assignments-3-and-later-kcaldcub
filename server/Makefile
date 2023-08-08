APP ?= aesdsocket
CFLAGS ?= -Wall -Werror -g -O0
LDFLAGS ?= -lpthread -lrt
CFILE ?= $(APP).c
OBJECTS ?= $(APP).o
BINARY ?= $(APP)

ifdef CROSS_COMPILE
	CC ?= $(CROSS_COMPILE)gcc
else
	CC ?= gcc
endif

default : $(BINARY)

all : $(BINARY)

$(BINARY) : $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)
	sudo chown root $(BINARY)
	sudo chmod +s $(BINARY)

%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean : 
	rm -rf $(BINARY) $(OBJECTS)

run:
	@if [ $$(id -u) -eq 0 ]; then \
		./$(BINARY); \
	else \
		sudo ./$(BINARY); \
	fi

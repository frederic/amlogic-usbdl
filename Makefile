CC      ?= gcc
CFLAGS  ?= -g -O0 -Wall
LDFLAGS ?= 
LDLIBS  ?= -lusb-1.0

OBJS  = amlogic-usbdl.o
BIN   = amlogic-usbdl

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

.PHONY: clean
clean:
	rm $(BIN) $(OBJS)
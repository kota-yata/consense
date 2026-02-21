CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

BIN := consense
SRC := src/consense.c src/compress.c src/decompress.c

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
INSTALL ?= install

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

install: $(BIN)
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 0755 "$(BIN)" "$(DESTDIR)$(BINDIR)/$(BIN)"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/$(BIN)"

clean:
	rm -f $(BIN)

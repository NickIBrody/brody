CC      = gcc
CFLAGS  = -Wall -O2 -Iinclude
LDFLAGS = -lm
SRC     = src/main.c src/lexer.c src/ast.c src/parser.c src/interp.c
BIN     = brody

all: $(BIN)

$(BIN): $(SRC)
        $(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

install: $(BIN)
        install -Dm755 $(BIN) /usr/bin/$(BIN)

clean:
        rm -f $(BIN)

deb: $(BIN)
        mkdir -p deb/DEBIAN deb/usr/bin deb/usr/share/brody/examples
        cp $(BIN) deb/usr/bin/
        cp examples/hello.br deb/usr/share/brody/examples/
        cp debian/control deb/DEBIAN/
        dpkg-deb --build deb brody_1.0.0_amd64.deb

.PHONY: all install clean deb

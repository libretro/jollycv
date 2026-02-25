.POSIX:

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

CC = cc
CFLAGS = -O2

PKG_CONFIG = pkg-config

NAME = jollycv-example

FLAGS = -std=c11 -Wall -Wextra -Wshadow -pedantic

CFLAGS_LIBJOLLYCV = `$(PKG_CONFIG) --cflags libjollycv`
LIBS_LIBJOLLYCV = `$(PKG_CONFIG) --libs libjollycv`

CFLAGS_SDL3 = `$(PKG_CONFIG) --cflags sdl3`
LIBS_SDL3 = `$(PKG_CONFIG) --libs sdl3`

CFLAGS_SPEEXDSP = `$(PKG_CONFIG) --cflags speexdsp`
LIBS_SPEEXDSP = `$(PKG_CONFIG) --libs speexdsp`

INCLUDES = $(CFLAGS_LIBJOLLYCV) $(CFLAGS_SDL3) $(CFLAGS_SPEEXDSP)
LIBS = $(LIBS_LIBJOLLYCV) $(LIBS_SDL3) $(LIBS_SPEEXDSP)

BUILD_FLAGS = $(CFLAGS) $(FLAGS) $(CPPFLAGS) $(INCLUDES)

all: $(NAME)

$(NAME): example.c
	$(CC) $(BUILD_FLAGS) -o $@ $? $(LDFLAGS) $(LIBS)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(NAME) $(DESTDIR)$(BINDIR)/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(NAME)

clean:
	rm -f $(NAME)

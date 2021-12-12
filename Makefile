SOURCEDIR := $(abspath $(patsubst %/,%,$(dir $(abspath $(lastword \
	$(MAKEFILE_LIST))))))

CC ?= cc
CFLAGS ?= -O2
FLAGS := -fPIC -std=c99 -Wall -Wextra -Wshadow -Wmissing-prototypes -pedantic
DEPDIR := $(SOURCEDIR)/deps
SRCDIR := $(SOURCEDIR)/core

PKGCONF ?= pkg-config
CFLAGS_JG := $(shell $(PKGCONF) --cflags jg)

INCLUDES := -I$(SRCDIR) -I$(SRCDIR)/z80
SHARED := -fPIC

NAME := jollycv
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
DATAROOTDIR ?= $(PREFIX)/share
DOCDIR ?= $(DATAROOTDIR)/doc/$(NAME)

USE_VENDORED_SPEEXDSP ?= 0

UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
	SHARED += -dynamiclib
	TARGET := $(NAME).dylib
else ifeq ($(OS), Windows_NT)
	SHARED += -shared
	TARGET := $(NAME).dll
else
	SHARED += -shared
	TARGET := $(NAME).so
endif

OBJDIR := objs

CSRCS := $(OBJDIR)/z80/z80.c \
	$(OBJDIR)/jcv.c \
	$(OBJDIR)/jcv_memio.c \
	$(OBJDIR)/jcv_mixer.c \
	$(OBJDIR)/jcv_psg.c \
	$(OBJDIR)/jcv_sgmpsg.c \
	$(OBJDIR)/jcv_vdp.c \
	$(OBJDIR)/jcv_z80.c \
	$(OBJDIR)/jg.c

ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	CFLAGS_SPEEX := -I$(DEPDIR)
	CSRCS += $(OBJDIR)/speex/resample.c
else
	CFLAGS_SPEEX := $(shell $(PKGCONF) --cflags speexdsp)
	LIBS_SPEEX := $(shell $(PKGCONF) --libs speexdsp)
endif

INCLUDES += $(CFLAGS_SPEEX)
LIBS := $(LIBS_SPEEX)

# Object dirs
MKDIRS := $(OBJDIR)/speex $(OBJDIR)/z80

# List of object files
OBJS := $(CSRCS:.c=.o)

.PHONY: all clean install install-strip uninstall

# Rules
$(OBJDIR)/%.o: $(DEPDIR)/%.c $(OBJDIR)/.tag
	$(info $(CC) $(CFLAGS) $(FLAGS) \
		$(subst $(SOURCEDIR)/,,$(INCLUDES) $<) -o $@)
	@$(CC) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)/.tag
	$(info $(CC) $(CFLAGS) $(FLAGS) \
		$(subst $(SOURCEDIR)/,,$(INCLUDES) $<) -o $@)
	@$(CC) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/%.o: $(SOURCEDIR)/%.c $(OBJDIR)/.tag
	$(info $(CC) $(CFLAGS) $(FLAGS) \
		$(subst $(SOURCEDIR)/,,$(INCLUDES) $(CFLAGS_JG) $<) -o $@)
	@$(CC) $(CFLAGS) $(FLAGS) $(INCLUDES) $(CFLAGS_JG) -c $< -o $@

all: $(TARGET)

$(OBJDIR)/.tag:
	@mkdir -p -- $(sort $(MKDIRS))
	@touch $@

$(TARGET): $(OBJS)
	@mkdir -p $(NAME)
	$(CC) $^ $(LDFLAGS) $(LIBS) $(SHARED) -o $(NAME)/$(TARGET)

clean:
	rm -rf $(OBJDIR)/ $(NAME)/

install: all
	@mkdir -p $(DESTDIR)$(DOCDIR)
	@mkdir -p $(DESTDIR)$(LIBDIR)/jollygood
	cp $(NAME)/$(TARGET) $(DESTDIR)$(LIBDIR)/jollygood/
	cp $(SOURCEDIR)/core/z80/LICENSE $(DESTDIR)$(DOCDIR)/LICENSE-z80
	cp $(SOURCEDIR)/LICENSE $(DESTDIR)$(DOCDIR)
	cp $(SOURCEDIR)/README $(DESTDIR)$(DOCDIR)

install-strip: install
	strip $(DESTDIR)$(LIBDIR)/jollygood/$(TARGET)

uninstall:
	rm -rf $(DESTDIR)$(DOCDIR)
	rm -f $(DESTDIR)$(LIBDIR)/jollygood/$(TARGET)

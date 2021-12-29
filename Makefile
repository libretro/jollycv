SOURCEDIR := $(abspath $(patsubst %/,%,$(dir $(abspath $(lastword \
	$(MAKEFILE_LIST))))))

CC ?= cc
CFLAGS ?= -O2
FLAGS := -std=c99 -Wall -Wextra -Wshadow -Wmissing-prototypes -pedantic
DEPDIR := $(SOURCEDIR)/deps
SRCDIR := $(SOURCEDIR)/core

PKGCONF ?= pkg-config
CFLAGS_JG := $(shell $(PKGCONF) --cflags jg)

INCLUDES := -I$(SRCDIR) -I$(SRCDIR)/z80
PIC := -fPIC
SHARED := $(PIC)

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
	Q_SPEEXDSP :=
	CFLAGS_SPEEXDSP := -I$(DEPDIR)
	LIBS_SPEEXDSP :=
	CSRCS += $(OBJDIR)/speex/resample.c
else
	Q_SPEEXDSP := @
	CFLAGS_SPEEXDSP := $(shell $(PKGCONF) --cflags speexdsp)
	LIBS_SPEEXDSP := $(shell $(PKGCONF) --libs speexdsp)
endif

INCLUDES += $(CFLAGS_SPEEXDSP)
LIBS := $(LIBS_SPEEXDSP)

# Object dirs
MKDIRS := $(OBJDIR)/speex $(OBJDIR)/z80

# List of object files
OBJS := $(CSRCS:.c=.o)

# Compiler command
COMPILE = $(strip $(1) $(CPPFLAGS) $(PIC) $(2) -c $< -o $@)
COMPILE_C = $(call COMPILE, $(CC) $(CFLAGS), $(1))

# Info command
COMPILE_INFO = $(info $(subst $(SOURCEDIR)/,,$(1)))

# Core commands
BUILD_JG = $(call COMPILE_C, $(FLAGS) $(INCLUDES) $(CFLAGS_JG))
BUILD_MAIN = $(call COMPILE_C, $(FLAGS) $(INCLUDES))

.PHONY: all clean install install-strip uninstall

# Rules
$(OBJDIR)/%.o: $(DEPDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO, $(BUILD_MAIN))
	@$(BUILD_MAIN)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO, $(BUILD_MAIN))
	@$(BUILD_MAIN)

$(OBJDIR)/%.o: $(SOURCEDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO, $(BUILD_JG))
	@$(BUILD_JG)

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
	$(Q_SPEEXDSP)if test $(USE_VENDORED_SPEEXDSP) != 0; then \
		cp $(DEPDIR)/speex/COPYING \
			$(DESTDIR)$(DOCDIR)/COPYING-speexdsp; \
	fi

install-strip: install
	strip $(DESTDIR)$(LIBDIR)/jollygood/$(TARGET)

uninstall:
	rm -rf $(DESTDIR)$(DOCDIR)
	rm -f $(DESTDIR)$(LIBDIR)/jollygood/$(TARGET)

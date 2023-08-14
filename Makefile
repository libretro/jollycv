SOURCEDIR := $(abspath $(patsubst %/,%,$(dir $(abspath $(lastword \
	$(MAKEFILE_LIST))))))

# https://semver.org/
VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 1
VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

AR ?= ar
CC ?= cc
CFLAGS ?= -O2
FLAGS := -std=c11 -Wall -Wextra -Wshadow -Wmissing-prototypes -pedantic
DEPDIR := $(SOURCEDIR)/deps
SRCDIR := $(SOURCEDIR)/src

PKG_CONFIG ?= pkg-config
CFLAGS_JG := $(shell $(PKG_CONFIG) --cflags jg)

INCLUDES := -I$(SRCDIR) -I$(SRCDIR)/z80
PIC := -fPIC
SHARED := $(PIC)

NAME := jollycv
PREFIX ?= /usr/local
EXEC_PREFIX ?= $(PREFIX)
LIBDIR ?= $(EXEC_PREFIX)/lib
DATAROOTDIR ?= $(PREFIX)/share
DOCDIR ?= $(DATAROOTDIR)/doc/$(NAME)

LIBPATH := $(LIBDIR)/jollygood
PKGCONFLIBDIR := $(shell $(SOURCEDIR)/lib/pkgconf.sh "$(EXEC_PREFIX)" "$(LIBDIR)")

ifeq ($(PREFIX), $(EXEC_PREFIX))
	PKGCONFEXECDIR := $${prefix}
else
	PKGCONFEXECDIR := $(EXEC_PREFIX)
endif

DISABLE_MODULE ?= 0
ENABLE_SHARED ?= 0
ENABLE_STATIC ?= 0
ENABLE_STATIC_JG ?= 0
USE_VENDORED_SPEEXDSP ?= 0

UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
	LIBRARY := $(NAME).dylib
else ifeq ($(OS), Windows_NT)
	LIBRARY := $(NAME).dll
else
	LIBRARY := $(NAME).so
endif

LIB_PC := lib$(NAME).pc
LIB_SHARED := lib$(LIBRARY)
LIB_STATIC := lib$(NAME).a

ifeq ($(UNAME), Darwin)
	LIB_MAJOR := lib$(NAME).$(VERSION_MAJOR).dylib
	LIB_VERSION := lib$(NAME).$(VERSION).dylib
	SHARED += -dynamiclib
	SONAME := -Wl,-install_name,$(LIB_MAJOR)
else
	LIB_MAJOR := $(LIB_SHARED).$(VERSION_MAJOR)
	LIB_VERSION := $(LIB_SHARED).$(VERSION)
	SHARED += -shared
	SONAME := -Wl,-soname,$(LIB_MAJOR)
endif

ifeq ($(UNAME), Linux)
	UNDEFINED := -Wl,--no-undefined
endif

REQUIRES_PRIVATE := Requires.private:

CSRCS := z80/z80.c \
	jcv.c \
	jcv_memio.c \
	jcv_mixer.c \
	jcv_psg.c \
	jcv_serial.c \
	jcv_sgmpsg.c \
	jcv_vdp.c \
	jcv_z80.c

JGSRCS := jg.c

ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	CFLAGS_SPEEXDSP := -I$(DEPDIR)
	LIBS_SPEEXDSP :=
	CSRCS += speex/resample.c
else
	REQUIRES_PRIVATE += speexdsp
	CFLAGS_SPEEXDSP := $(shell $(PKG_CONFIG) --cflags speexdsp)
	LIBS_SPEEXDSP := $(shell $(PKG_CONFIG) --libs speexdsp)
endif

INCLUDES += $(CFLAGS_SPEEXDSP)
LIBS := $(LIBS_SPEEXDSP)

# Object dirs
MKDIRS := speex z80

OBJDIR := objs

# List of object files
OBJS := $(patsubst %,$(OBJDIR)/%,$(CSRCS:.c=.o))
OBJS_JG := $(patsubst %,$(OBJDIR)/%,$(JGSRCS:.c=.o))
OBJS_JG_STATIC := $(patsubst %,$(OBJDIR)/%,$(JGSRCS:.c=-static.o))

# Library targets
TARGET :=
TARGET_MODULE := $(NAME)/$(LIBRARY)
TARGET_SHARED := $(OBJDIR)/$(LIB_VERSION)
TARGET_STATIC := $(OBJDIR)/$(LIB_STATIC)
TARGET_STATIC_JG := $(NAME)/lib$(NAME)-jg.a

ifeq ($(DISABLE_MODULE), 0)
	TARGET += $(TARGET_MODULE)
endif

ifneq ($(ENABLE_STATIC), 0)
	TARGET += $(TARGET_STATIC)
	OBJS_SHARED := $(TARGET_STATIC)
else
	OBJS_SHARED := $(OBJS)
endif

ifneq ($(ENABLE_SHARED), 0)
	TARGET += $(OBJDIR)/$(LIB_MAJOR) $(OBJDIR)/$(LIB_SHARED)
	LIBS_MODULE := -L$(OBJDIR) -ljollycv
else
	LIBS_MODULE := $(OBJS_SHARED)
endif

ifneq ($(ENABLE_STATIC_JG), 0)
	TARGET += $(NAME)/jg-static.mk
endif

ifneq ($(ENABLE_SHARED), 0)
	OBJS_MODULE := $(OBJDIR)/$(LIB_MAJOR)
	ENABLE_PKGCONF := 1
else ifneq ($(ENABLE_STATIC), 0)
	OBJS_MODULE := $(OBJS_SHARED)
	ENABLE_PKGCONF := 1
else
	OBJS_MODULE := $(OBJS)
	ENABLE_PKGCONF := 0
endif

# Compiler command
COMPILE = $(strip $(1) $(CPPFLAGS) $(2) -c $< -o $@)
COMPILE_C = $(call COMPILE, $(CC) $(CFLAGS), $(1))

# Info command
COMPILE_INFO = $(info $(subst $(SOURCEDIR)/,,$(1)))

# Core commands
BUILD_JG = $(call COMPILE_C, $(FLAGS) $(INCLUDES) $(CFLAGS_JG) $(PIC))
BUILD_JG_STATIC = $(call COMPILE_C, $(FLAGS) $(INCLUDES) $(CFLAGS_JG))
BUILD_MAIN = $(call COMPILE_C, $(FLAGS) $(INCLUDES))

.PHONY: all clean install install-strip uninstall

all: $(TARGET)

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

$(OBJDIR)/%-static.o: $(SOURCEDIR)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO, $(BUILD_JG_STATIC))
	@$(BUILD_JG_STATIC)

$(OBJDIR)/.tag:
	@mkdir -p -- $(patsubst %,$(OBJDIR)/%,$(MKDIRS))
	@touch $@

$(TARGET_MODULE): $(OBJS_JG) $(OBJS_MODULE)
	@mkdir -p $(NAME)
	$(strip $(CC) -o $@ $< $(LDFLAGS) $(LIBS_MODULE) $(LIBS) $(UNDEFINED) $(SHARED))

$(TARGET_SHARED): $(OBJS)
	$(strip $(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(UNDEFINED) $(SHARED) $(SONAME))

$(TARGET_STATIC): $(OBJS)
	$(strip $(AR) rcs $@ $^)

$(TARGET_STATIC_JG): $(OBJS_JG_STATIC) $(OBJS_SHARED)
	@mkdir -p $(NAME)
	$(strip $(AR) rcs $@ $^)

$(NAME)/jg-static.mk: $(TARGET_STATIC_JG)
	@printf '%s\n%s\n%s\n' 'NAME := $(NAME)' 'ASSETS :=' \
		'LIBS_STATIC := $(LIBS)' > $@

$(OBJDIR)/$(LIB_MAJOR) $(OBJDIR)/$(LIB_SHARED): $(TARGET_SHARED)
	ln -s $(LIB_VERSION) $@

clean:
	rm -rf $(OBJDIR) $(NAME)

install: all
	@mkdir -p $(DESTDIR)$(DOCDIR)
ifeq ($(DISABLE_MODULE), 0)
	@mkdir -p $(DESTDIR)$(LIBPATH)
	cp $(TARGET_MODULE) $(DESTDIR)$(LIBPATH)/
else ifeq ($(ENABLE_PKGCONF), 0)
	@mkdir -p $(DESTDIR)$(LIBDIR)
endif
ifneq ($(ENABLE_PKGCONF), 0)
	@mkdir -p $(DESTDIR)$(LIBDIR)/pkgconfig
	sed -e 's|@PREFIX@|$(PREFIX)|' -e 's|@EXEC_PREFIX@|$(PKGCONFEXECDIR)|' \
		-e 's|@LIBDIR@|$(PKGCONFLIBDIR)|' -e '/URL:/a\' \
		-e '$(REQUIRES_PRIVATE)' $(SOURCEDIR)/lib/$(LIB_PC).in \
		> $(DESTDIR)$(LIBDIR)/pkgconfig/$(LIB_PC)
endif
ifneq ($(ENABLE_SHARED), 0)
	cp $(TARGET_SHARED) $(DESTDIR)$(LIBDIR)/
	cp -P $(OBJDIR)/$(LIB_MAJOR) $(DESTDIR)$(LIBDIR)/
	cp -P $(OBJDIR)/$(LIB_SHARED) $(DESTDIR)$(LIBDIR)/
endif
ifneq ($(ENABLE_STATIC), 0)
	cp $(TARGET_STATIC) $(DESTDIR)$(LIBDIR)/
endif
	cp $(SRCDIR)/z80/LICENSE $(DESTDIR)$(DOCDIR)/LICENSE-z80
	cp $(SOURCEDIR)/LICENSE $(DESTDIR)$(DOCDIR)
	cp $(SOURCEDIR)/README $(DESTDIR)$(DOCDIR)
ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	cp $(DEPDIR)/speex/COPYING $(DESTDIR)$(DOCDIR)/COPYING-speexdsp
endif

install-strip: install
ifeq ($(DISABLE_MODULE), 0)
	strip $(DESTDIR)$(LIBPATH)/$(LIBRARY)
endif
ifneq ($(ENABLE_SHARED), 0)
	strip $(DESTDIR)$(LIBDIR)/$(LIB_VERSION)
endif

uninstall:
	rm -rf $(DESTDIR)$(DOCDIR)
	rm -f $(DESTDIR)$(LIBPATH)/$(LIBRARY)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_STATIC)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SHARED)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_MAJOR)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_VERSION)
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/$(LIB_PC)

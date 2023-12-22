DISABLE_MODULE ?= 0
ENABLE_SHARED ?= 0
ENABLE_STATIC ?= 0
ENABLE_STATIC_JG ?= 0

AR ?= ar
CC ?= cc
CXX ?= c++
CC_FOR_BUILD ?= $(CC)
CXX_FOR_BUILD ?= $(CXX)

VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

PKG_CONFIG ?= pkg-config
CFLAGS_JG := $(shell $(PKG_CONFIG) --cflags jg)

PIC := -fPIC
SHARED := $(PIC)

PREFIX ?= /usr/local
EXEC_PREFIX ?= $(PREFIX)
LIBDIR ?= $(EXEC_PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
DATAROOTDIR ?= $(PREFIX)/share
DATADIR ?= $(DATAROOTDIR)
DOCDIR ?= $(DATAROOTDIR)/doc/$(NAME)

INCPATH := $(INCLUDEDIR)/$(JGNAME)
LIBPATH := $(LIBDIR)/jollygood
OBJDIR := objs

LIBS_PRIVATE := Libs.private:
REQUIRES_PRIVATE := Requires.private:

PKGCONF_SH := $(wildcard $(SOURCEDIR)/lib/pkgconf.sh)

ifneq ($(PKGCONF_SH),)
	PKGCONFLIBDIR := $(shell $(PKGCONF_SH) "$(EXEC_PREFIX)" "$(LIBDIR)" exec_)
	PKGCONFINCDIR := $(shell $(PKGCONF_SH) "$(PREFIX)" "$(INCLUDEDIR)")
endif

ifeq ($(PREFIX), $(EXEC_PREFIX))
	PKGCONFEXECDIR := $${prefix}
else
	PKGCONFEXECDIR := $(EXEC_PREFIX)
endif

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
	SHARED += -dynamiclib -Wl,-undefined,error
	SONAME := -Wl,-install_name,$(LIB_MAJOR)
else
	LIB_MAJOR := $(LIB_SHARED).$(VERSION_MAJOR)
	LIB_VERSION := $(LIB_SHARED).$(VERSION)
	SHARED += -shared -Wl,--no-undefined
	SONAME := -Wl,-soname,$(LIB_MAJOR)
endif

# Desktop File
DESKTOP := $(JGNAME).desktop

DESKTOP_TARGET := $(NAME)/$(DESKTOP)

# Icons
ICONS := $(wildcard $(SOURCEDIR)/icons/*.png $(SOURCEDIR)/icons/$(NAME).svg)

ICONS_BASE := $(notdir $(ICONS))
ICONS_TARGET := $(ICONS_BASE:%=$(NAME)/icons/%)

# Library targets
override TARGET :=
override TARGET_INSTALL := install-data install-docs install-library
override TARGET_MODULE := $(NAME)/$(LIBRARY)
override TARGET_SHARED := $(OBJDIR)/$(LIB_VERSION)
override TARGET_STATIC := $(OBJDIR)/$(LIB_STATIC)
override TARGET_STATIC_JG := $(NAME)/lib$(NAME)-jg.a
override TARGET_STATIC_MK := $(NAME)/jg-static.mk

ifeq ($(DISABLE_MODULE), 0)
	override TARGET += $(TARGET_MODULE)
endif

ifneq ($(ENABLE_STATIC), 0)
	override TARGET += $(TARGET_STATIC)
	override OBJS_SHARED := $(TARGET_STATIC)
else
	override OBJS_SHARED = $(OBJS)
endif

ifneq ($(ENABLE_SHARED), 0)
	override TARGET += $(OBJDIR)/$(LIB_MAJOR) $(OBJDIR)/$(LIB_SHARED)
	override LIBS_MODULE := -L$(OBJDIR) -l$(NAME)
else
	override LIBS_MODULE = $(OBJS_SHARED)
endif

ifneq ($(ENABLE_STATIC_JG), 0)
	override TARGET += $(DESKTOP_TARGET) $(ICONS_TARGET) $(TARGET_STATIC_MK)
endif

ifneq ($(ENABLE_SHARED), 0)
	override OBJS_MODULE := $(OBJDIR)/$(LIB_MAJOR) $(OBJDIR)/$(LIB_SHARED)
else ifneq ($(ENABLE_STATIC), 0)
	override OBJS_MODULE := $(OBJS_SHARED)
else
	override OBJS_MODULE = $(OBJS)
endif

ifneq (,$(filter-out 0,$(ENABLE_SHARED) $(ENABLE_STATIC)))
	override ENABLE_INSTALL := 1
	override ENABLE_LIBRARY := 1
else ifeq ($(DISABLE_MODULE), 0)
	override ENABLE_INSTALL := 1
	override ENABLE_LIBRARY := 0
else
	override ENABLE_INSTALL := 0
	override ENABLE_LIBRARY := 0
endif

# Compiler commands
COMPILE = $(strip $(1) $(CPPFLAGS) $(PIC) $(2) -c $< -o $@)
COMPILE_C = $(call COMPILE, $(CC) $(CFLAGS), $(1))
COMPILE_CXX = $(call COMPILE, $(CXX) $(CXXFLAGS), $(1))
COMPILE_C_BUILD = $(strip $(CC_FOR_BUILD) $(1) $< -o $@)
COMPILE_CXX_BUILD = $(strip $(CXX_FOR_BUILD) $(1) $< -o $@)

# Info command
COMPILE_INFO = $(info $(subst $(SOURCEDIR)/,,$(1)))

override .DEFAULT_GOAL := all
override PHONY := all clean install install-strip uninstall $(TARGET_INSTALL)

install-data: all

install-library: all
ifeq ($(DISABLE_MODULE), 0)
	@mkdir -p $(DESTDIR)$(LIBPATH)
	cp $(TARGET_MODULE) $(DESTDIR)$(LIBPATH)/
endif
ifneq ($(ENABLE_LIBRARY), 0)
	@mkdir -p $(DESTDIR)$(LIBDIR)/pkgconfig
ifneq ($(ENABLE_SHARED), 0)
	cp $(TARGET_SHARED) $(DESTDIR)$(LIBDIR)/
	cp -P $(OBJDIR)/$(LIB_MAJOR) $(DESTDIR)$(LIBDIR)/
	cp -P $(OBJDIR)/$(LIB_SHARED) $(DESTDIR)$(LIBDIR)/
endif
ifneq ($(ENABLE_STATIC), 0)
	cp $(TARGET_STATIC) $(DESTDIR)$(LIBDIR)/
endif
	sed -e 's|@PREFIX@|$(PREFIX)|' \
		-e 's|@EXEC_PREFIX@|$(PKGCONFEXECDIR)|' \
		-e 's|@LIBDIR@|$(PKGCONFLIBDIR)|' \
		-e 's|@INCLUDEDIR@|$(PKGCONFINCDIR)|' \
		-e 's|@VERSION@|$(VERSION)|' \
		-e 's|@DESCRIPTION@|$(DESCRIPTION)|' \
		-e 's|@NAME@|$(NAME)|' -e 's|@JGNAME@|$(JGNAME)|' \
		-e '/Libs:/a\' -e '$(LIBS_PRIVATE)' \
		-e '/URL:/a\' -e '$(REQUIRES_PRIVATE)' \
		$(SOURCEDIR)/lib/pkgconf.pc.in \
		> $(DESTDIR)$(LIBDIR)/pkgconfig/$(LIB_PC)
endif

ifneq ($(ENABLE_INSTALL), 0)
install: $(TARGET_INSTALL)
else
install: all
	@echo 'Nothing to install'
endif

install-strip: install
ifeq ($(DISABLE_MODULE), 0)
	strip $(DESTDIR)$(LIBPATH)/$(LIBRARY)
endif
ifneq ($(ENABLE_SHARED), 0)
	strip $(DESTDIR)$(LIBDIR)/$(LIB_VERSION)
endif

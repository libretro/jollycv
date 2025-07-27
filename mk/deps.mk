-include $(SOURCEDIR)/mk/core.mk
-include $(SOURCEDIR)/mk/miniz.mk
-include $(SOURCEDIR)/mk/samplerate.mk
-include $(SOURCEDIR)/mk/speexdsp.mk

CFLAGS_JG = $(shell $(PKG_CONFIG) --cflags jg)

PKGCONF_EPOXY := epoxy
CFLAGS_EPOXY = $(shell $(PKG_CONFIG) --cflags $(PKGCONF_EPOXY))
LIBS_EPOXY = $(shell $(PKG_CONFIG) --libs $(PKGCONF_EPOXY))

PKGCONF_FLAC := flac
CFLAGS_FLAC = $(shell $(PKG_CONFIG) --cflags $(PKGCONF_FLAC))
LIBS_FLAC = $(shell $(PKG_CONFIG) --libs $(PKGCONF_FLAC))

PKGCONF_LZO := lzo2
CFLAGS_LZO = $(shell $(PKG_CONFIG) --cflags $(PKGCONF_LZO))
LIBS_LZO = $(shell $(PKG_CONFIG) --libs $(PKGCONF_LZO))

PKGCONF_SDL2 := sdl2
CFLAGS_SDL2 = $(shell $(PKG_CONFIG) --cflags $(PKGCONF_SDL2))
LIBS_SDL2 = $(shell $(PKG_CONFIG) --libs $(PKGCONF_SDL2))

PKGCONF_ZLIB := zlib
CFLAGS_ZLIB = $(shell $(PKG_CONFIG) --cflags $(PKGCONF_ZLIB))
LIBS_ZLIB = $(shell $(PKG_CONFIG) --libs $(PKGCONF_ZLIB))

PKGCONF_ZSTD := libzstd
CFLAGS_ZSTD = $(shell $(PKG_CONFIG) --cflags $(PKGCONF_ZSTD))
LIBS_ZSTD = $(shell $(PKG_CONFIG) --libs $(PKGCONF_ZSTD))

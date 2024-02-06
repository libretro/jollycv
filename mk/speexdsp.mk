DIR_SPEEXDSP := $(wildcard $(DEPDIR)/speex)

ifneq ($(DIR_SPEEXDSP),)
	USE_VENDORED_SPEEXDSP ?= 0
else
	override USE_VENDORED_SPEEXDSP := 0
endif

ifneq ($(USE_VENDORED_SPEEXDSP), 0)
	CFLAGS_SPEEXDSP := -I$(DEPDIR)
	LIBS_SPEEXDSP :=  $(if $(findstring -lm,$(LIBS)),,-lm)
	MKDIRS += deps/speex
	SRCS_SPEEXDSP := deps/speex/resample.c
	OBJS_SPEEXDSP := $(SRCS_SPEEXDSP:.c=.o)
else
	CFLAGS_SPEEXDSP = $(shell $(PKG_CONFIG) --cflags speexdsp)
	LIBS_SPEEXDSP = $(shell $(PKG_CONFIG) --libs speexdsp)
	OBJS_SPEEXDSP :=
endif

ifneq ($(findstring speexdsp,$(LIBS_REQUIRES)),)
	ifneq ($(USE_VENDORED_SPEEXDSP), 0)
		override LIBS_PRIVATE += $(LIBS_SPEEXDSP)
	else
		override REQUIRES_PRIVATE += speexdsp
	endif
endif

ifneq ($(DIR_SPEEXDSP),)
FLAGS_SPEEXDSP := -std=c99 $(WARNINGS_DEF_C)

BUILD_SPEEXDSP = $(call COMPILE_C, $(FLAGS_SPEEXDSP))

$(OBJDIR)/deps/speex/%.o: $(DIR_SPEEXDSP)/%.c $(OBJDIR)/.tag
	$(call COMPILE_INFO,$(BUILD_SPEEXDSP))
	@$(BUILD_SPEEXDSP)

ifneq ($(USE_VENDORED_SPEEXDSP), 0)
install-docs::
	cp $(DIR_SPEEXDSP)/COPYING $(DESTDIR)$(DOCDIR)/COPYING-speexdsp
endif
endif

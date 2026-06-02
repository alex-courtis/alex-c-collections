INCS = -Iinc

CPPFLAGS += $(INCS)

OFLAGS = -O3
WFLAGS = -pedantic \
		 -Wall \
		 -Wextra \
		 -Werror \
		 -Wimplicit-fallthrough \
		 -Wno-unused-parameter
DFLAGS = -g
MFLAGS =
COMPFLAGS = $(WFLAGS) $(OFLAGS) $(DFLAGS) $(MFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17 

LDFLAGS += $(MFLAGS)

CC = clang

PKGS += cmocka
PKG_CONFIG ?= pkg-config
CFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --libs $(p)))

ifneq (,$(findstring -m32,$(MFLAGS)))
	VG_SUPP = --suppressions=.vg.cmocka.32.supp
endif

INCS = -Iinc

CPPFLAGS += $(INCS)

OFLAGS = -O3
WFLAGS = -pedantic \
		 -Wall \
		 -Wextra \
		 -Werror \
		 -Wimplicit-fallthrough \
		 -Wold-style-definition \
		 -Wstrict-prototypes \
		 -Wno-unused-parameter
DFLAGS = -g
MFLAGS =
COMPFLAGS = $(WFLAGS) $(OFLAGS) $(DFLAGS) $(MFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17

LDFLAGS += $(MFLAGS)

PKGS += cmocka
PKG_CONFIG ?= pkg-config
CFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --libs $(p)))

CC = gcc

VALGRIND = valgrind \
		   --error-exitcode=1 \
		   --leak-check=full \
		   --show-leak-kinds=all \
		   --errors-for-leak-kinds=all \
		   --gen-suppressions=all

ifneq (,$(findstring -m32,$(MFLAGS)))
	VALGRIND += --suppressions=.vg.cmocka.32.supp
endif

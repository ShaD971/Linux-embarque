CC ?= gcc
PKG_CONFIG ?= pkg-config
BIN = dashboard

LVGL_PATH = lvgl

# override: Buildroot's TARGET_CONFIGURE_OPTS passes CFLAGS/LDFLAGS on the
# make command line, which a plain += would silently be discarded in favor
# of (command-line variables win over makefile assignments unless the
# makefile explicitly opts back in with `override`).
override CFLAGS += -O2 -g -Wall -Wextra -I. -I$(LVGL_PATH) -I$(LVGL_PATH)/include
override CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm)
override LDFLAGS += -lm -lpthread
override LDFLAGS += $(shell $(PKG_CONFIG) --libs libdrm)

include lvgl_sources.mk

CSRCS := $(LVGL_CSRCS) main.c dashboard.c sysinfo.c

OBJS = $(CSRCS:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BIN) $(OBJS)

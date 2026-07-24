CC ?= gcc
PKG_CONFIG ?= pkg-config
BIN = dashboard

LVGL_PATH = lvgl

CFLAGS += -O2 -g -Wall -Wextra -I. -I$(LVGL_PATH) -I$(LVGL_PATH)/include
CFLAGS += $(shell $(PKG_CONFIG) --cflags libdrm)
LDFLAGS += -lm -lpthread
LDFLAGS += $(shell $(PKG_CONFIG) --libs libdrm)

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

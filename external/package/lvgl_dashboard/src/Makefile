CC ?= gcc
PKG_CONFIG ?= pkg-config

LVGL_PATH = lvgl

# Backend d'affichage :
#   drm (defaut) -> cible Raspberry Pi 4, /dev/dri/card*, seule dep : libdrm
#   sdl          -> poste de developpement, fenetre SDL2, JAMAIS installe sur la cible
BACKEND ?= drm

BUILD_DIR = build/$(BACKEND)

# `override` est indispensable : Buildroot invoque ce Makefile avec
# CFLAGS="..." / LDFLAGS="..." en ligne de commande (TARGET_CONFIGURE_OPTS).
# Une affectation `+=` simple serait alors purement ignoree, et la
# cross-compilation echouerait faute de -I vers LVGL. `override` conserve les
# flags de la toolchain cible tout en ajoutant les notres.
override CFLAGS += -O2 -g -Wall -I. -I$(LVGL_PATH) -I$(LVGL_PATH)/include

# -Wextra n'est applique qu'a NOS sources : l'arbre LVGL est du code tiers
# (sous-module, non modifiable ici) qui emet des -Wunused-parameter propres a
# lui. On ne masque donc rien de notre cote.
STRICT_CFLAGS = -Wextra

ifeq ($(BACKEND),drm)
  BIN = dashboard
  override CFLAGS  += -DDASH_BACKEND_DRM=1
  override CFLAGS  += $(shell $(PKG_CONFIG) --cflags libdrm)
  override LDFLAGS += $(shell $(PKG_CONFIG) --libs libdrm)
else ifeq ($(BACKEND),sdl)
  BIN = dashboard-sdl
  # On neutralise le driver DRM et on active SDL + l'API snapshot (captures).
  override CFLAGS  += -DDASH_BACKEND_SDL=1 -DLV_USE_LINUX_DRM=0 -DLV_USE_SDL=1 -DLV_USE_SNAPSHOT=1
  # Tas LVGL elargi : lv_snapshot_take alloue une image plein ecran (8 Mo en
  # 1920x1080). Sans effet sur la cible, qui garde ses 4 Mo.
  override CFLAGS  += '-DLV_MEM_SIZE=(32U * 1024U * 1024U)'
  override CFLAGS  += $(shell $(PKG_CONFIG) --cflags sdl2)
  override LDFLAGS += $(shell $(PKG_CONFIG) --libs sdl2)
else
  $(error BACKEND invalide '$(BACKEND)' : valeurs attendues 'drm' ou 'sdl')
endif

override LDFLAGS += -lm -lpthread

include lvgl_sources.mk

# lvgl_sources.mk ne liste que les drivers de la cible (fb + drm). Les sources
# du driver SDL sont ajoutees ici, uniquement pour BACKEND=sdl, pour ne pas
# toucher a lvgl_sources.mk ni risquer de les embarquer dans le binaire cible.
ifeq ($(BACKEND),sdl)
LVGL_CSRCS += \
  $(LVGL_PATH)/src/drivers/sdl/lv_sdl_window.c \
  $(LVGL_PATH)/src/drivers/sdl/lv_sdl_sw.c \
  $(LVGL_PATH)/src/drivers/sdl/lv_sdl_texture.c \
  $(LVGL_PATH)/src/drivers/sdl/lv_sdl_mouse.c \
  $(LVGL_PATH)/src/drivers/sdl/lv_sdl_keyboard.c \
  $(LVGL_PATH)/src/drivers/sdl/lv_sdl_mousewheel.c
endif

CSRCS := $(LVGL_CSRCS) main.c dashboard.c sysinfo.c

OWN_CSRCS := main.c dashboard.c sysinfo.c
OBJS = $(addprefix $(BUILD_DIR)/,$(CSRCS:.c=.o))
# Depfiles pour nos seules sources. Les generer aussi pour l'arbre LVGL (400+
# fichiers, chacun tirant des centaines d'en-tetes) rendait le simple parcours
# des dependances par make plus long que la compilation elle-meme.
DEPS = $(addprefix $(BUILD_DIR)/,$(OWN_CSRCS:.c=.d))

.PHONY: all clean

all: $(BIN)

# Le sous-module LVGL est fige, mais sa compilation depend entierement de
# lv_conf.h et des -D poses ici. Cette dependance explicite couvre le vrai
# risque (config modifiee, objets perimes) sans le cout du suivi complet.
# A garder apres la cible `all`, sinon le premier .o devient le but par defaut.
$(OBJS): lv_conf.h Makefile

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Code tiers (sous-module LVGL) : -Wall seul.
$(BUILD_DIR)/$(LVGL_PATH)/%.o: $(LVGL_PATH)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Nos sources : -Wall -Wextra + suivi des dependances d'en-tetes.
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(STRICT_CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build dashboard dashboard-sdl

-include $(DEPS)

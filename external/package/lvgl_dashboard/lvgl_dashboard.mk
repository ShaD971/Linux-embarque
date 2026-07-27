################################################################################
# lvgl_dashboard
################################################################################

# LVGL_DASHBOARD_SITE_METHOD is intentionally left as the default (wget):
# `local` would set OVERRIDE_SRCDIR, which makes Buildroot skip the whole
# download stage (pkg-generic.mk) - EXTRA_DOWNLOADS would then never be
# fetched. LVGL itself (public repo) is therefore the package's normal,
# hash-checked download; the dashboard application code, which is
# versioned directly in this tree, is copied in during extraction instead.
LVGL_DASHBOARD_VERSION = c4424b27d63db752aa75f9fdffe30c6467b55ad1
LVGL_DASHBOARD_SITE = $(call github,lvgl,lvgl,$(LVGL_DASHBOARD_VERSION))
LVGL_DASHBOARD_SOURCE = lvgl-$(LVGL_DASHBOARD_VERSION).tar.gz
LVGL_DASHBOARD_DEPENDENCIES = libdrm

define LVGL_DASHBOARD_EXTRACT_CMDS
	mkdir -p $(@D)/lvgl
	$(INFLATE$(suffix $(LVGL_DASHBOARD_SOURCE))) $(LVGL_DASHBOARD_DL_DIR)/$(LVGL_DASHBOARD_SOURCE) | \
		$(TAR) --strip-components=1 -C $(@D)/lvgl $(TAR_OPTIONS) -
	cp -a $(LVGL_DASHBOARD_PKGDIR)/src/. $(@D)/
endef

define LVGL_DASHBOARD_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define LVGL_DASHBOARD_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/dashboard $(TARGET_DIR)/usr/bin/lvgl_dashboard
	$(INSTALL) -D -m 0755 $(LVGL_DASHBOARD_PKGDIR)/S99lvgl_dashboard \
		$(TARGET_DIR)/etc/init.d/S99lvgl_dashboard
endef

$(eval $(generic-package))

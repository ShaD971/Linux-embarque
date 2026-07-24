################################################################################
# lvgl_dashboard
################################################################################

LVGL_DASHBOARD_VERSION = 1.0
LVGL_DASHBOARD_SITE = $(LVGL_DASHBOARD_PKGDIR)/src
LVGL_DASHBOARD_SITE_METHOD = local
LVGL_DASHBOARD_DEPENDENCIES = libdrm

LVGL_DASHBOARD_LVGL_VERSION = c4424b27d63db752aa75f9fdffe30c6467b55ad1
LVGL_DASHBOARD_EXTRA_DOWNLOADS = \
	$(call github,lvgl,lvgl,$(LVGL_DASHBOARD_LVGL_VERSION))/lvgl-$(LVGL_DASHBOARD_LVGL_VERSION).tar.gz

define LVGL_DASHBOARD_EXTRACT_LVGL
	$(RM) -r $(@D)/lvgl
	mkdir -p $(@D)/lvgl
	$(TAR) -xzf \
		$(LVGL_DASHBOARD_DL_DIR)/lvgl-$(LVGL_DASHBOARD_LVGL_VERSION).tar.gz \
		-C $(@D)/lvgl --strip-components=1
endef
LVGL_DASHBOARD_POST_RSYNC_HOOKS += LVGL_DASHBOARD_EXTRACT_LVGL

define LVGL_DASHBOARD_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define LVGL_DASHBOARD_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/dashboard $(TARGET_DIR)/usr/bin/lvgl_dashboard
	$(INSTALL) -D -m 0755 $(LVGL_DASHBOARD_PKGDIR)/S99lvgl_dashboard \
		$(TARGET_DIR)/etc/init.d/S99lvgl_dashboard
endef

$(eval $(generic-package))

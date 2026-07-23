################################################################################
# lvgl_dashboard
################################################################################

LVGL_DASHBOARD_VERSION = 6141926be161ead29d34e1090dc2cfca535ae283
LVGL_DASHBOARD_SITE = https://github.com/ShaD971/lvgl_dashboard.git
LVGL_DASHBOARD_SITE_METHOD = git
LVGL_DASHBOARD_GIT_SUBMODULES = YES
LVGL_DASHBOARD_DEPENDENCIES = libdrm

define LVGL_DASHBOARD_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define LVGL_DASHBOARD_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/dashboard $(TARGET_DIR)/usr/bin/lvgl_dashboard
	$(INSTALL) -D -m 0755 $(LVGL_DASHBOARD_PKGDIR)/S99lvgl_dashboard \
		$(TARGET_DIR)/etc/init.d/S99lvgl_dashboard
endef

$(eval $(generic-package))

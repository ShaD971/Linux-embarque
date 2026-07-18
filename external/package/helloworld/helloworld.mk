################################################################################
# helloworld
################################################################################

HELLOWORLD_VERSION = 1.0
HELLOWORLD_SITE = $(BR2_EXTERNAL_LINUX_EMBARQUE_PATH)/package/helloworld/src
HELLOWORLD_SITE_METHOD = local
HELLOWORLD_LICENSE = GPL-2.0-or-later
HELLOWORLD_LICENSE_FILES = LICENSE

define HELLOWORLD_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define HELLOWORLD_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/helloworld $(TARGET_DIR)/usr/bin/helloworld
endef

$(eval $(generic-package))

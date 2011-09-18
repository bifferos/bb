#
# Copyright (C) 2011 Bifferos.com (sales@bifferos.com)
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk


PKG_NAME:=rtai-led-demo
PKG_RELEASE:=1
PKG_BUILD_DEPENDS:=liblxrt libpthread

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/rtai-led-demo
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=RTAI LED demo
  DEPENDS:=+liblxrt +libpthread
endef

define Package/rtai-led-demo/description
  Uses RTAI to demo changes to the LED state in realtime.
  Remove the watchdog module before running this example,
  and ensure the rtai modules rtai_hal.ko, rtai_lxrt.ko
  and rtai_fifos.ko are loaded (in that order)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(shell $(STAGING_DIR)/usr/bin/rtai-config --cc)" \
		LXRT_CFLAGS="-I$(STAGING_DIR)/usr/include -Wall -Wstrict-prototypes -pipe" \
		LXRT_LDFLAGS="-L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/lib -lpthread"
endef

define Package/rtai-led-demo/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rtai-led-demo $(1)/usr/bin
endef

$(eval $(call BuildPackage,rtai-led-demo))

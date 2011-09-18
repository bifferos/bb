#
# Copyright (C) 2011 Bifferos.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk


PKG_NAME:=biffconfig
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/biffconfig
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Utility for modifying the Biffboot environment settings
endef

define Package/biffconfig/description
  This utility allows you to change such things as kernel command-line, partition table 
  locations, output on serial console for Biffboot.  Use with caution as changing the
  wrong values will prevent your OpenWrt system from booting.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/biffconfig/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/biffconfig $(1)/sbin/
endef

$(eval $(call BuildPackage,biffconfig))

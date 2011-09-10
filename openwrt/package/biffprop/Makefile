#
# Copyright (C) 2011 Bifferos.com
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk


PKG_NAME:=biffprop
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/biffprop
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Utility programming Parallax Propeller chip
endef

define Package/biffprop/description
  Utility which uses the Bifferboard serial port and a single GPIO pin to
  upload a program to a propeller.  This is for cheapskates who just bought
  the Propeller and want to upload code to it, perhaps as a co-processor,
  there is no support for programming EEPROM, and indeed this is not needed
  to run programs on the Prop.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/biffprop/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/biffprop $(1)/usr/bin/
endef

$(eval $(call BuildPackage,biffprop))

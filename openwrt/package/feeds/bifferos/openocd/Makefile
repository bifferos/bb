#
# Makefile for OpenOCD by Nicolas Le Falher
#
# Contributions by Nathan & Bifferos
#

include $(TOPDIR)/rules.mk

PKG_NAME:=openocd
PKG_VERSION:=0.5.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/openocd
PKG_MD5SUM:=42999a8950b669780f27ec7484c86458

include $(INCLUDE_DIR)/package.mk

define Package/openocd
	SECTION:=openocd
	CATEGORY:=Utilities
	TITLE:=OpenOCD, The Open On-Chip Debugger
endef

define Package/openocd/description
	The Open On-Chip Debugger (OpenOCD) aims to provide debugging, 
	in-system programming and boundary-scan testing for embedded 
	target devices.
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
		./configure --enable-parport --enable-parport_ppdev \
	);
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)
endef

define Package/openocd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/openocd $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/openocd
	$(CP) -r $(PKG_BUILD_DIR)/tcl $(1)/etc/openocd 
endef

$(eval $(call BuildPackage,openocd))

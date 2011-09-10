#
# OpenWrt Makefile for RTAI, (c) Bifferos.com (sales@bifferos.com)
#
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rtai
PKG_VERSION:=cvs_20110827
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=@SF/bifferboard/

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)_$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk


define Package/liblxrt
 SECTION:=libs
 CATEGORY:=Libraries
 TITLE:= lxrt libraries from RTAI
 URL:=http://www.rtai.org/
endef

define Package/liblxrt/description
 This package contains the RTAI lxrt library and kernel modules
endef


define Build/Configure
	( cd $(PKG_BUILD_DIR); \
		CROSS_COMPILE=$(TARGET_CROSS) \
		./configure \
		--with-linux-dir=$(LINUX_DIR) \
		--host=i386 \
		--prefix=$(PKG_INSTALL_DIR) \
		--disable-math \
		--disable-fpu \
		--disable-math-c99 \
		--disable-leds \
		--disable-diag-tsc-sync \
		--disable-tune-tsc-sync \
		--disable-testsuite \
		CC=$(TARGET_CROSS)gcc \
		CXX=$(TARGET_CROSS)cxx \
		LD=$(TARGET_CROSS)ld \
		AR=$(TARGET_CROSS)ar \
		RANLIB=$(TARGET_CROSS)ranlib \
		STRIP=$(TARGET_CROSS)strip \
		NM=$(TARGET_CROSS)nm \
	)
endef


define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/base/include/*.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/*.h $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/include/asm-i386
	$(CP) $(PKG_BUILD_DIR)/base/include/asm-i386/*.h $(1)/usr/include/asm-i386
	$(LN) $(STAGING_DIR)/usr/include/asm-i386 $(STAGING_DIR)/usr/include/asm
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/base/scripts/rtai-{config,info,load} $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/base/sched/liblxrt/.libs/liblxrt.a $(1)/usr/lib
endef

define Package/liblxrt/install
	$(INSTALL_DIR) $(1)/lib/modules/$(LINUX_VERSION)
	$(CP) $(PKG_BUILD_DIR)/base/arch/i386/hal/rtai_hal.ko $(1)/lib/modules/$(LINUX_VERSION)/
	$(CP) $(PKG_BUILD_DIR)/base/sched/rtai_lxrt.ko $(1)/lib/modules/$(LINUX_VERSION)/
	$(CP) $(PKG_BUILD_DIR)/base/ipc/fifos/rtai_fifos.ko $(1)/lib/modules/$(LINUX_VERSION)/
endef

$(eval $(call BuildPackage,liblxrt))

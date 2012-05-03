#
# Makefile for xenomai by Nicolas Le Falher
#	chmod +x $(PKG_BUILD_DIR)/scripts/xeno-config; \
#		--includedir=$(PKG_BUILD_DIR)/include \

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=xenomai
PKG_VERSION:=2.6.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://download.gna.org/xenomai/stable/
PKG_MD5SUM:=360123b0616cc7499b4b3b9366711e08

XENOMAI_PATCH_DIR:=ksrc/arch/x86/patches/
ifeq ($(LINUX_VERSION), 2.6.37.6)
   XENOMAI_PATCH:=$(XENOMAI_PATCH_DIR)adeos-ipipe-2.6.37.6-x86-2.9-02.patch
endif
ifeq ($(LINUX_VERSION), 2.6.38.8)
   XENOMAI_PATCH:=$(XENOMAI_PATCH_DIR)adeos-ipipe-2.6.38.8-x86-2.10-01.patch
endif

PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/tmp_install/usr/xenomai

include $(INCLUDE_DIR)/package.mk

define Package/xenomai
   SECTION:=xenomai
   CATEGORY:=Utilities
   TITLE:=Xenomai: Real-Time Framework for Linux
   DEPENDS:=+librt
endef

define Package/xenomai/description
	Xenomai is a real-time development framework cooperating with the Linux kernel,
	in order to provide a pervasive, interface-agnostic, hard real-time support to user-space applications,
	seamlessly integrated into the GNU/Linux environment.
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
	./scripts/prepare-kernel.sh --adeos=$(XENOMAI_PATCH) --linux=$(BUILD_DIR_BASE)/linux-$(BOARD)/linux-$(LINUX_VERSION)/ --arch=$(ARCH);\
	./configure \
		--enable-debug \
		--program-prefix= \
		--disable-doc-install \
		--host=$(OPTIMIZE_FOR_CPU)-openwrt-linux-$(TARGET_SUFFIX) \
		--target=$(OPTIMIZE_FOR_CPU)-openwrt-linux-$(TARGET_SUFFIX) \
		--enable-smp \
		--enable-x86-sep \
		--enable-x86-tsc \
		;\
	);
endef

define Build/Compile
	(cd $(PKG_BUILD_DIR); \
	mkdir $(PKG_BUILD_DIR)/tmp_install; \
	$(MAKE); \
	$(MAKE) DESTDIR=$(PKG_BUILD_DIR)/tmp_install install-user; \
	);
endef

define Package/xenomai/install
	$(INSTALL_DIR) $(1)/usr/xenomai
	$(INSTALL_DIR) $(1)/usr/xenomai/include
	$(CP) $(PKG_INSTALL_DIR)/include/* $(1)/usr/xenomai/include
	$(INSTALL_DIR) $(1)/usr/xenomai/bin
	$(CP) $(PKG_INSTALL_DIR)/bin/* $(1)/usr/xenomai/bin
	$(INSTALL_DIR) $(1)/usr/xenomai/lib
	$(CP) $(PKG_INSTALL_DIR)/lib/* $(1)/usr/xenomai/lib
	$(INSTALL_DIR) $(1)/usr/xenomai/sbin
	$(CP) $(PKG_INSTALL_DIR)/sbin/* $(1)/usr/xenomai/sbin
	$(INSTALL_DIR) $(1)/usr/xenomai/share
	$(CP) $(PKG_INSTALL_DIR)/share/* $(1)/usr/xenomai/share
endef

$(eval $(call BuildPackage,xenomai))

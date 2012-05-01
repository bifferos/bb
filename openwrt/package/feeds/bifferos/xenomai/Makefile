#
# Makefile for xenomai by Nicolas Le Falher
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=xenomai
PKG_VERSION:=2.6.0
PKG_RELEASE:=1

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

include $(INCLUDE_DIR)/package.mk

define Package/xenomai
	SECTION:=xenomai
	CATEGORY:=Utilities
	TITLE:=Xenomai: Real-Time Framework for Linux
endef

define Package/xenomai/description
	Xenomai is a real-time development framework cooperating with the Linux kernel,
	in order to provide a pervasive, interface-agnostic, hard real-time support to user-space applications,
	seamlessly integrated into the GNU/Linux environment.
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
		./scripts/prepare-kernel.sh --adeos=$(XENOMAI_PATCH) --linux=$(BUILD_DIR_BASE)/linux-$(BOARD)/linux-$(LINUX_VERSION)/ --arch=$(ARCH)\
	);
endef

define Build/Compile
endef

define Package/xenomai/install
endef

$(eval $(call BuildPackage,xenomai))

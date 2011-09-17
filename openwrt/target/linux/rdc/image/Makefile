#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Image/Prepare
	$(CP) $(LINUX_DIR)/arch/x86/boot/bzImage $(KDIR)/bzImage
endef

define Image/Build/bifferboard
	$(TOPDIR)/target/linux/rdc/image/mkimg_bifferboard.py $(KDIR)/bzImage $(KDIR)/root.$(1) $(BIN_DIR)/$(IMG_PREFIX)-$(1)-$(2).img  \
		$(CONFIG_TARGET_BIFFERBOARD_KMAX)
endef

define Image/Build/Initramfs
	$(CP) $(KDIR)/bzImage $(BIN_DIR)/$(IMG_PREFIX)-ramfs.bzImage
endef

define Image/Build
	$(CP) $(KDIR)/bzImage $(BIN_DIR)/$(IMG_PREFIX).bzImage
	$(call Image/Build/$(PROFILE),$(1),$(PROFILE),$(patsubst jffs2-%k,%,$(1)))
ifeq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),y)
	$(call Image/Build/Initramfs)
endif
endef

$(eval $(call BuildImage))

#
# Copyright (C) 2007-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

autoconf_bool = $(patsubst %,$(if $($(1)),--enable,--disable)-%,$(2))

# delete *.la-files from staging_dir - we can not yet remove respective lines within all package
# Makefiles, since backfire still uses libtool v1.5.x which (may) require those files
define libtool_remove_files
	find $(1) -name '*.la' | $(XARGS) rm -f;
endef


AM_TOOL_PATHS:= \
	AUTOM4TE=$(STAGING_DIR_HOST)/bin/autom4te \
	AUTOCONF=$(STAGING_DIR_HOST)/bin/autoconf \
	AUTOMAKE=$(STAGING_DIR_HOST)/bin/automake \
	ACLOCAL=$(STAGING_DIR_HOST)/bin/aclocal \
	AUTOHEADER=$(STAGING_DIR_HOST)/bin/autoheader \
	LIBTOOLIZE=$(STAGING_DIR_HOST)/bin/libtoolize \
	LIBTOOL=$(STAGING_DIR_HOST)/bin/libtool \
	M4=$(STAGING_DIR_HOST)/bin/m4 \
	AUTOPOINT=true

# 1: build dir
# 2: remove files
# 3: automake paths
# 4: libtool paths
# 5: extra m4 dirs
define autoreconf
	(cd $(1); \
		$(patsubst %,rm -f %;,$(2)) \
		$(foreach p,$(3), \
			if [ -f $(p)/configure.ac ] || [ -f $(p)/configure.in ]; then \
				[ -d $(p)/autom4te.cache ] && rm -rf autom4te.cache; \
				touch NEWS AUTHORS COPYING ChangeLog; \
				$(AM_TOOL_PATHS) $(STAGING_DIR_HOST)/bin/autoreconf -v -f -i -s \
					$(if $(word 2,$(3)),--no-recursive) \
					-B $(STAGING_DIR_HOST)/share/aclocal \
					$(patsubst %,-I %,$(5)) \
					$(patsubst %,-I %,$(4)) $(4) || true; \
				[ -e $(p)/config.rpath ] || \
						ln -s $(SCRIPT_DIR)/config.rpath $(p)/config.rpath;
			fi; \
		) \
	);
endef

# 1: build dir
define patch_libtool
	(cd $(1); \
		for lt in $$$$($$(STAGING_DIR_HOST)/bin/find . -name ltmain.sh); do \
			lt_version="$$$$($$(STAGING_DIR_HOST)/bin/sed -ne 's,^[[:space:]]*VERSION=\([0-9]\.[0-9]\+\).*,\1,p' $$$$lt)"; \
			case "$$$$lt_version" in \
				1.5|2.2|2.4) echo "autotools.mk: Found libtool v$$$$lt_version - applying patch to $$$$lt"; \
					(cd $$$$(dirname $$$$lt) && $$(PATCH) -s -p1 < $$(TOPDIR)/tools/libtool/files/libtool-v$$$$lt_version.patch) ;; \
				*) echo "autotools.mk: error: Unsupported libtool version v$$$$lt_version - cannot patch $$$$lt"; exit 1 ;; \
			esac; \
		done; \
	);
endef


PKG_LIBTOOL_PATHS?=$(CONFIGURE_PATH)
PKG_AUTOMAKE_PATHS?=$(CONFIGURE_PATH)
PKG_MACRO_PATHS?=m4
PKG_REMOVE_FILES?=aclocal.m4

Hooks/InstallDev/Post += libtool_remove_files

define autoreconf_target
  $(strip $(call autoreconf, \
    $(PKG_BUILD_DIR), $(PKG_REMOVE_FILES), \
    $(PKG_AUTOMAKE_PATHS), $(PKG_LIBTOOL_PATHS), \
    $(STAGING_DIR)/host/share/aclocal $(STAGING_DIR)/usr/share/aclocal $(PKG_MACRO_PATHS)))
endef

define patch_libtool_target
  $(strip $(call patch_libtool, \
    $(PKG_BUILD_DIR)))
endef

ifneq ($(filter patch-libtool,$(PKG_FIXUP)),)
  Hooks/Configure/Pre += patch_libtool_target
endif

ifneq ($(filter libtool,$(PKG_FIXUP)),)
  PKG_BUILD_DEPENDS += libtool libintl libiconv
 ifeq ($(filter no-autoreconf,$(PKG_FIXUP)),)
  Hooks/Configure/Pre += autoreconf_target
 endif
endif

ifneq ($(filter libtool-ucxx,$(PKG_FIXUP)),)
  PKG_BUILD_DEPENDS += libtool libintl libiconv
 ifeq ($(filter no-autoreconf,$(PKG_FIXUP)),)
  Hooks/Configure/Pre += autoreconf_target
 endif
endif

ifneq ($(filter autoreconf,$(PKG_FIXUP)),)
  ifeq ($(filter autoreconf,$(Hooks/Configure/Pre)),)
    Hooks/Configure/Pre += autoreconf_target
  endif
endif


HOST_FIXUP?=$(PKG_FIXUP)
HOST_LIBTOOL_PATHS?=$(if $(PKG_LIBTOOL_PATHS),$(PKG_LIBTOOL_PATHS),.)
HOST_AUTOMAKE_PATHS?=$(if $(PKG_AUTOMAKE_PATHS),$(PKG_AUTOMAKE_PATHS),.)
HOST_MACRO_PATHS?=$(if $(PKG_MACRO_PATHS),$(PKG_MACRO_PATHS),m4)
HOST_REMOVE_FILES?=$(PKG_REMOVE_FILES)

define autoreconf_host
  $(strip $(call autoreconf, \
    $(HOST_BUILD_DIR), $(HOST_REMOVE_FILES), \
    $(HOST_AUTOMAKE_PATHS), $(HOST_LIBTOOL_PATHS), \
    $(HOST_MACRO_PATHS)))
endef

define patch_libtool_host
  $(strip $(call patch_libtool, \
    $(HOST_BUILD_DIR)))
endef

ifneq ($(filter patch-libtool,$(PKG_FIXUP)),)
  Hooks/HostConfigure/Pre += patch_libtool_host
endif

ifneq ($(filter patch-libtool,$(HOST_FIXUP)),)
  Hooks/HostConfigure/Pre += $(strip $(call patch_libtool,$(HOST_BUILD_DIR)))
endif

ifneq ($(filter libtool,$(HOST_FIXUP)),)
 ifeq ($(filter no-autoreconf,$(HOST_FIXUP)),)
  Hooks/HostConfigure/Pre += autoreconf_host
 endif
endif

ifneq ($(filter libtool-ucxx,$(HOST_FIXUP)),)
 ifeq ($(filter no-autoreconf,$(HOST_FIXUP)),)
  Hooks/HostConfigure/Pre += autoreconf_host
 endif
endif

ifneq ($(filter autoreconf,$(HOST_FIXUP)),)
  ifeq ($(filter autoreconf,$(Hooks/HostConfigure/Pre)),)
    Hooks/HostConfigure/Pre += autoreconf_host
  endif
endif
PKG_NAME=stsafea
PKG_URL=https://www.st.com/content/st_com/en/products/embedded-software/mcu-mpu-embedded-software/stm32-embedded-software/stm32cube-expansion-packages/x-cube-safea1.html
PKG_VERSION=3.3.1
PKG_LICENSE=N/A

PKG_SOURCE_LOCAL=STSAFE_Axx0/
MIDDLEWARE_CONF=include/stsafea_conf.h

include $(RIOTBASE)/pkg/pkg.mk

all: $(PKG_SOURCE_LOCAL) $(MIDDLEWARE_CONF)
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR)/CoreModules/Src -f $(CURDIR)/$(PKG_NAME).mk

$(PKG_SOURCE_LOCAL):
	$(error $(PKG_SOURCE_DIR) has not been populated by the user! See $(PKG_SOURCE_DIR)/doc.txt )

$(MIDDLEWARE_CONF):
	cp $(PKG_SOURCE_DIR)/Interface/stsafea_conf_template.h $@

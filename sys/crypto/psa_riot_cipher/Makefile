BASE_MODULE := psa_riot_cipher
SUBMODULES := 1

ifneq (,$(filter psa_riot_cipher_aes_%,$(USEMODULE)))
  USEMODULE += psa_riot_cipher_aes_common
endif

include $(RIOTBASE)/Makefile.base

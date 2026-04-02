GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/generated/fonts/*.c))

DEPPATH += --dep-path $(PRJ_DIR)/generated/fonts
VPATH += :$(PRJ_DIR)/generated/fonts

CFLAGS += "-I$(PRJ_DIR)/generated/fonts"

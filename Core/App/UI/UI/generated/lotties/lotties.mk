GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/generated/lotties/*.c))

DEPPATH += --dep-path $(PRJ_DIR)/generated/lotties
VPATH += :$(PRJ_DIR)/generated/lotties

CFLAGS += "-I$(PRJ_DIR)/generated/lotties"

# @file sdk.mk
#
# This file is created and owned by anyui.
#
# COPYRIGHT 2025 anyui Team
# All rights reserved.
#
# https://anyui.tech/
#
# Author: anyui Team

GEN_CSRCS += $(notdir $(wildcard $(PRJ_DIR)/sdk/*.c))

DEPPATH += --dep-path $(PRJ_DIR)/sdk
VPATH += :$(PRJ_DIR)/sdk

CFLAGS += "-I$(PRJ_DIR)/sdk"

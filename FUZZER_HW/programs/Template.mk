# === DO NOT MODIFY THIS FILE ===
# This file is a template file which should be included in every
# Makefile in the subdirectories of the programs directory.
# 
# Modifying this file may break the build system. During grading,
# we will be replacing this file with our own copy. You have been
# warned.
# === DO NOT MODIFY THIS FILE ===

# See $PROJECT_DIR/programs/README.md for details on how to
# use this file.

include ../Makefile.variables

DIR_NM := $(notdir $(CURDIR))

EXEC-NM ?= $(DIR_NM)
BIN ?= bin/
BLD ?= build/
INC ?= include/
SRC ?= src/
COVSAN_SRC ?= $(CURDIR)/../coverage.c

define remove_slashes
  $(shell echo $(subst /,.,$1) | sed 's/\./\//')
endef

SRC_FILES := $(shell find $(SRC) -type f -name '*.c')
OBJ_FILES := $(foreach OBJ, $(patsubst $(SRC)%,$(BLD)%,$(SRC_FILES:.c=.o)), $(call remove_slashes, $(OBJ)))

.PHONY: all debug prod setup clean start end cclean

all: start prod end
debug: CFLAGS += $(DFLAGS)
debug: start prod end

start:
	@echo " DESCEND  $(EXEC-NM)"

end:
	@echo " ASCEND   $(EXEC-NM)"

prod: setup $(BIN)$(EXEC-NM)

setup: $(BIN) $(BLD)

$(BIN)$(EXEC-NM): $(OBJ_FILES)
	-@$(CC) $^ $(COVSAN_SRC) -o $@ $(LIBS); \
	if [ $$? -eq 0 ]; then \
		echo " LD       $(EXEC-NM)"; \
	fi

define BUILD_RECIPE_TEMPLATE
$1: $2
	-@$$(CC) $$(CFLAGS) -I $$(INC) -c $$< -o $$@; \
	if [ $$$$? -eq 0 ]; then \
		echo " CC       $$(notdir $$<)"; \
	fi
endef

$(foreach i, $(shell seq $(words $(OBJ_FILES))), \
	$(eval $(call BUILD_RECIPE_TEMPLATE, $(word $(i), $(OBJ_FILES)), $(word $(i), $(SRC_FILES))) ))

$(BIN):
	@mkdir $(BIN)

$(BLD):
	@mkdir $(BLD)

clean:
	@if [ -d $(BIN) ]; then 				\
		rm -rf $(BIN); 						\
	fi
	@if [ -d $(BLD) ]; then 				\
		rm -rf $(BLD); 						\
	fi
	@echo " CLEAN    $(EXEC-NM)"

-include $(wildcard $(BLD)*.d)
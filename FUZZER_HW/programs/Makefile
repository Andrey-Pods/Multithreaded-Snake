# === NOTE ===
# During grading, we will replace the entire programs directory with
# our own. Feel free to add additional programs to test your fuzzer
# against here using the build system. See 
# $PROJECT_DIR/programs/README.md for how to use the build system.
# === NOTE ===

# Add program subdirectories here to add to build script

PROGRAM-DIR := cascade hello-world

# ------------------------------------------------------------ #
# === DO NOT MODIFY BEYOND THIS POINT ===
# Modifying the file beyond this point may break the build
# system. 
# === DO NOT MODIFY BEYOND THIS POINT ===

MAKEFLAGS += --no-print-directory

MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
DIR_NM := $(shell basename $(MK_DIR))
BIN ?= $(MK_DIR)/bin/

.PHONY: all debug clean

all: $(BIN)
	@echo " DESCEND  $(DIR_NM)"
	@for DIR in $(PROGRAM-DIR); do 						\
		$(MAKE) -C $(MK_DIR)$$DIR all BIN=$(BIN);		\
	done
	@echo " ASCEND   $(DIR_NM)"

debug: $(BIN)
	@echo " DESCEND  $(DIR_NM)"
	@for DIR in $(PROGRAM-DIR); do 						\
		$(MAKE) -C $(MK_DIR)$$DIR debug BIN=$(BIN);		\
	done
	@echo " ASCEND   $(DIR_NM)"

clean:
	@for DIR in $(PROGRAM-DIR); do						\
		$(MAKE) -C $(MK_DIR)$$DIR clean BIN=$(BIN);		\
	done
	@rm -rf $(BIN)
	@echo " CLEAN    $(DIR_NM)"

$(BIN):
	@mkdir $(BIN)

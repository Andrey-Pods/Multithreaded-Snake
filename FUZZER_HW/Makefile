# === DO NOT MODIFY THIS FILE ===
# Modifying this file may break the build system. During grading,
# we will be replacing this file with our own copy. You have been
# warned.
# === DO NOT MODIFY THIS FILE ===

CC := clang
EXEC := fuzzer
TEST_EXEC := $(EXEC)_tests

SOURCE := src/
INCLUDE := include/
BUILD := build/
BIN := bin/
TEST := test/
LIB := lib/
PROGRAMS := programs/

CFLAGS := -std=c11 -Wall -Werror -Wno-unused-function
DFLAGS := -g -DDEBUG -DCOLOR
PRINT_STATEMENTS := -DERROR -DSUCCESS -DWARN -DINFO
DEPFLAGS := -MMD -MP

POSIX := -D_POSIX_SOURCE
BSD := -D_DEFAULT_SOURCE
GNU := -D_GNU_SOURCE

INC := -I$(INCLUDE)
LIBS := 
TEST_LIBS := -lcriterion

CFLAGS += $(STD) $(POSIX) $(BSD) $(GNU)

define remove_slashes
  $(shell echo $(subst /,.,$1) | sed 's/\./\//')
endef

SRCF := $(shell find $(SOURCE) -type f -name '*.c')
OBJF := $(patsubst $(SOURCE)%,$(BUILD)%,$(SRCF:.c=.o))
OBJF := $(foreach OBJ, $(OBJF), $(call remove_slashes, $(OBJ)))
FUNCF := $(filter-out $(BUILD)main.o, $(OBJF))

TEST_SRCF := $(shell find $(TEST) -maxdepth 1 -type f -name '*.c')

.PHONY: clean all debug setup test_programs

all: CFLAGS += $(OFLAGS)
all: prod

prod: setup $(BIN)$(EXEC) $(BIN)$(TEST_EXEC) test_programs

debug: CFLAGS += $(DFLAGS) $(PRINT_STATEMENTS)
debug: prod

setup: $(BIN) $(BUILD)

$(BIN):
	mkdir -p $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BIN)$(EXEC): $(OBJF)
	$(CC) $^ -o $@ $(LIB)fzl.a $(LIBS)

$(BIN)$(TEST_EXEC): $(FUNCF) $(TEST_SRCF)
	$(CC) $(CFLAGS) $(INC) $^ $(LIB)fzl.a $(TEST_LIBS) $(LIBS) -o $@


define BUILD_RECIPE_TEMPLATE
$1: $2
	$$(CC) $$(CFLAGS) $(DEPFLAGS) $$(INC) -c $$< -o $$@
endef

$(foreach i, $(shell seq $(words $(OBJF))), \
		$(eval $(call BUILD_RECIPE_TEMPLATE, $(word $(i), $(OBJF)), $(word $(i), $(SRCF))) ))

clean:
	rm -rfv $(BUILD) $(BIN)
	$(MAKE) -C $(PROGRAMS) clean

test_programs:
	$(MAKE) -C $(PROGRAMS) all

.PRECIOUS: $(BUILD)*.d

-include $(wildcard $(BUILD)*.d)
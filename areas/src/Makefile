SUBDIRS := mob obj qst shp wld zon

ROOT_DIR          := $(abspath ../..)
BIN_ROOT          ?= $(ROOT_DIR)/bin
AREA_TOOL_BIN_DIR ?= $(BIN_ROOT)/areas/tools
AREA_TOOL_OBJ_DIR ?= $(BIN_ROOT)/objects/areas/tools

export BIN_ROOT AREA_TOOL_BIN_DIR AREA_TOOL_OBJ_DIR

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	@for directory in $(SUBDIRS) areas; do \
		$(MAKE) -C "$$directory" clean || exit; \
	done

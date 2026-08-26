BIN_ROOT ?= $(abspath ../../..)/bin
AREA_TOOL_BIN_DIR ?= $(BIN_ROOT)/areas/tools
AREA_TOOL_OBJ_DIR ?= $(BIN_ROOT)/objects/areas/tools

MAKE_NEW_TARGET := $(AREA_TOOL_BIN_DIR)/make_new
MAKE_AREAS_TARGET := $(AREA_TOOL_BIN_DIR)/make_areas
MAKE_NEW_OBJECT := $(AREA_TOOL_OBJ_DIR)/areas/make_new.o
MAKE_AREAS_OBJECT := $(AREA_TOOL_OBJ_DIR)/areas/make_areas.o

.PHONY: all make_new make_areas clean

all: $(MAKE_NEW_TARGET) $(MAKE_AREAS_TARGET)
make_new: $(MAKE_NEW_TARGET)
make_areas: $(MAKE_AREAS_TARGET)

$(MAKE_NEW_TARGET): $(MAKE_NEW_OBJECT)
	@mkdir -p $(dir $@)
	gcc -o $@ $<

$(MAKE_AREAS_TARGET): $(MAKE_AREAS_OBJECT)
	@mkdir -p $(dir $@)
	gcc -o $@ $<

$(MAKE_NEW_OBJECT): make_new.c
	@mkdir -p $(dir $@)
	gcc -c -g $< -o $@

$(MAKE_AREAS_OBJECT): make_areas.c
	@mkdir -p $(dir $@)
	gcc -c -g $< -o $@

clean:
	rm -f $(MAKE_NEW_TARGET) $(MAKE_AREAS_TARGET) \
		$(MAKE_NEW_OBJECT) $(MAKE_AREAS_OBJECT)

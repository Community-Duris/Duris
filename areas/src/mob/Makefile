TOOL := make_mob
BIN_ROOT ?= $(abspath ../../..)/bin
AREA_TOOL_BIN_DIR ?= $(BIN_ROOT)/areas/tools
AREA_TOOL_OBJ_DIR ?= $(BIN_ROOT)/objects/areas/tools
TARGET := $(AREA_TOOL_BIN_DIR)/$(TOOL)
OBJECT := $(AREA_TOOL_OBJ_DIR)/mob/$(TOOL).o

.PHONY: all $(TOOL) clean

all: $(TARGET)
$(TOOL): $(TARGET)

$(TARGET): $(OBJECT)
	@mkdir -p $(dir $@)
	gcc -o $@ $<

$(OBJECT): $(TOOL).c ../area_file.h
	@mkdir -p $(dir $@)
	gcc -c -g $< -o $@

clean:
	rm -f $(TARGET) $(OBJECT)

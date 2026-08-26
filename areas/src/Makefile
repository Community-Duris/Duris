SUBDIRS := mob obj qst shp wld zon

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	@for directory in $(SUBDIRS); do \
		$(MAKE) -C "$$directory" clean || exit; \
	done

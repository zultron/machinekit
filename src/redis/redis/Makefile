# Top level makefile, the real shit is at src/Makefile

default: depends all

depends:
	$(MAKE) -C deps/

.DEFAULT:
	cd src && $(MAKE) $@

install:
	cd src && $(MAKE) $@

clean:
	@for dir in $(DEPENDS); do \
	$(MAKE) -C deps/$$dir clean; done

.PHONY: install

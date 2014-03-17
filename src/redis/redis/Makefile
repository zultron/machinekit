# Top level makefile, the real shit is at src/Makefile

default: depends all

DEPENDS := \
	jemalloc \
	hiredis \
	linenoise

.PHONY: depends
depends: $(DEPENDS)

$(DEPENDS):
	$(MAKE) -C deps/$@

.DEFAULT:
	cd src && $(MAKE) $@

install:
	cd src && $(MAKE) $@

clean:
	@for dir in $(DEPENDS); do \
	$(MAKE) -C deps/$$dir clean; done
	
.PHONY: install

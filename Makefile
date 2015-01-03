# Top level makefile, the real shit is at src/Makefile

default: all


.DEFAULT:
	cd src && $(MAKE) $@

deb:        
	dpkg-buildpackage

.PHONY: test
test:
	@prove -I t/

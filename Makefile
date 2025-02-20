.PHONY: all build_tools test

all: build_tools test

build_tools: 
	bash Tools/*.sh

test: 
	$(MAKE) -C Tests

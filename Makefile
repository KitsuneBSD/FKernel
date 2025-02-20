.PHONY: all tools_build test

all: tools_build test

tools_build: 
	bash Tools/*.sh

test: 
	$(MAKE) -C Tests

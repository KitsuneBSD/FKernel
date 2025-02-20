.PHONY: all build_tools test

all: build_tools test

build_tools:
	@if [ ! -f "Tools/bin/x86_64-tcc" ]; then \
		bash ./Tools/BuildTcc.sh; \
	fi

test: 
	$(MAKE) -C Tests

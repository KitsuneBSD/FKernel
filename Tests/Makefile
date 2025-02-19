.PHONY: test

tests_dir := $(wildcard */)

test:
	@for dir in $(tests_dir); do \
		$(MAKE) -C $$dir; \
	done

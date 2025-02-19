.PHONY: all clean run

CC := tcc
CFLAGS := -I Include -Wall -Werror -Wpedantic -Wextras -O2

SOURCES := $(wildcard *.c)
EXES := $(SOURCES:.c=)

all: $(EXES)
	@$(MAKE) run
	@$(MAKE) clean

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

run:
	@for exe in $(EXES); do \
		echo "Running $$exe:"; \
		./$$exe || exit 1; \
	done

clean:
	@rm -f $(EXES)

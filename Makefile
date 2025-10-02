src := $(wildcard src/*.c)
obj := $(src:src/%.c=out/%.o)
dep := $(obj:.o=.d)
headers := $(wildcard src/*.h)

CFLAGS := -Wall -Wextra -std=gnu99

MODE ?= debug
ifeq ($(MODE), debug)
CFLAGS += -O0 -g
else ifeq ($(MODE), release)
CFLAGS += -O2
endif

LDFLAGS += -lmnl

# Rules

mkdir_p = mkdir -p $(@D)
check_options := out/.options.$(shell echo $(CC) $(CFLAGS) $(LDFLAGS) | sha256sum | awk '{ print $$1 }')

out/.options.%:
	$(mkdir_p)
	rm -f out/.options.*
	touch $@

.PHONY: .FORCE
.FORCE:

all: build

.PHONY: build
build: out/everycast

out/everycast: $(obj)
	@$(mkdir_p)
	$(CC) $(CFLAGS) $(obj) -o $@ $(LDFLAGS)

-include $(dep)
$(obj): out/%.o: src/%.c $(check_options)
	@$(mkdir_p)
	$(CC) $(CFLAGS) -MMD -c $(filter %.c,$<) -o $@

.PHONY: clean
clean:
	rm -rf out
	find . -type f -name *.o -delete
	find . -type f -name *.d -delete

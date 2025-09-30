src := $(wildcard src/*.c)
obj := $(src:.c=.o)
headers := $(wildcard src/*.h)

CFLAGS += -Wall -Wextra -std=gnu99

MODE ?=
ifeq ($(MODE), debug)
CFLAGS += -O0 -g
else ifeq ($(MODE), release)
CFLAGS += -O2
endif

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

$(filter src/%.o, $(obj)): src/%.o: $(headers) $(check_options)

out/everycast: $(obj)
	$(mkdir_p)
	$(CC) $(CFLAGS) $(obj) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf out
	find . -type f -name *.o -delete
